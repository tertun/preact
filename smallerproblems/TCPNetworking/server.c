#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bhhoang");
MODULE_DESCRIPTION("Server Module");

#define SERVER_PORT 3000

static struct socket *sock_listen = NULL;
static bool server_running = true; // Global flag to control server loop

static int __init server_init(void)
{
    struct sockaddr_in server;
    int ret;
    char *recv_buf;
    struct socket *sock_client = NULL;

    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock_listen);
    if (ret < 0) {
        printk(KERN_ERR "Server: Socket creation failed with error %d\n", ret);
        return ret;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(SERVER_PORT);

    ret = kernel_bind(sock_listen, (struct sockaddr *)&server, sizeof(server));
    if (ret < 0) {
        printk(KERN_ERR "Server: Bind failed with error %d\n", ret);
        goto out_sock_release;
    }

    ret = kernel_listen(sock_listen, 5);
    if (ret < 0) {
        printk(KERN_ERR "Server: Listen failed with error %d\n", ret);
        goto out_sock_release;
    }

    recv_buf = kmalloc(1024, GFP_KERNEL);
    if (!recv_buf) {
        printk(KERN_ERR "Server: Memory allocation for receive buffer failed\n");
        ret = -ENOMEM;
        goto out_sock_release;
    }

    while (server_running) {
        ret = kernel_accept(sock_listen, &sock_client, 0);
        if (ret < 0) { 
            printk(KERN_ERR "Server: Accept failed with error %d\n", ret);
            continue; // Try to accept the next connection
        }
        
        memset(recv_buf, 0, sizeof(recv_buf));
        // Receive message from client
        ret = kernel_recvmsg(sock_client, (struct msghdr *)recv_buf, (struct kvec *)recv_buf, 1, 1024, 0);
        if (ret > 0) {
            printk(KERN_INFO "Server: Received message with size of %d", ret);
            // If message is "exit", stop the server
            if (strcmp(recv_buf, "exit") == 0) {
                printk(KERN_INFO "Server: Received exit command\n");
                server_running = false;
                // Exit the server loop
                break;
            }
        } else {
            printk(KERN_ERR "Server: Receive failed or connection closed\n");
        }

        sock_release(sock_client);
    }

    kfree(recv_buf);

out_sock_release:
    if (sock_listen) {
        sock_release(sock_listen);
    }
    return ret;
}

static void __exit server_exit(void)
{
    printk(KERN_INFO "Server module exited\n");
}

module_init(server_init);
module_exit(server_exit);