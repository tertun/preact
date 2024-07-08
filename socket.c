#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/uio.h> // For struct iovec and related operations

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bhhoang");
MODULE_DESCRIPTION("Server Module");

#define SERVER_PORT 3000
#define ATTEMPT 3

static struct socket *sock_listen = NULL;
static bool server_running = true; // Global flag to control server loop

static int __init server_init(void)
{
    struct sockaddr_in server;
    int ret;
    static int count = 0;
    char *recv_buf;
    struct socket *sock_client = NULL;
    struct msghdr msg;
    struct kvec vec;
    int len;

    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock_listen);
    if (ret < 0) {
        printk(KERN_ERR "Server: Socket creation failed with error %d\n", ret);
        return ret;
    }

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
        if (ret < 0 && count < ATTEMPT) { 
            printk(KERN_ERR "Server: Accept failed with error %d\n", ret);
            count++;
            continue; // Try to accept the next connection
        }

        memset(recv_buf, 0, 1024); // Clear the buffer

        vec.iov_len = 1024; // Length of the receive buffer
        vec.iov_base = recv_buf; // The receive buffer itself

        // Setup message header for receiving
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;

        // Receive the message
        len = kernel_recvmsg(sock_client, &msg, &vec, 1, 1024, msg.msg_flags);

        if (len > 0) {
            recv_buf[len] = '\0'; // Ensure the received message is null-terminated
            printk(KERN_INFO "Server: Received message with size of %d\n", len);
            printk(KERN_INFO "Server: Message is %s\n", recv_buf);
            // If message is "exit", stop the server
            char *e_command = "exit";
            for (int i = 0; i < 4; i++) {
                if (recv_buf[i] != e_command[i]) {
                    break;
                }
                if (i == 3) {
                    printk(KERN_INFO "Server: Received exit command\n");
                    server_running = false;
                }
            }
        } else {
            printk(KERN_ERR "Server: Receive failed or connection closed\n");
        }

        if (sock_client) {
            sock_release(sock_client);
            sock_client = NULL; // Prevent double release
        }
    }

    kfree(recv_buf);

out_sock_release:
    if (sock_listen) {
        sock_release(sock_listen);
    }
    printk(KERN_INFO "Server module exited with code %d\n", ret);
    return ret;
}

static void __exit server_exit(void)
{
    printk(KERN_INFO "Server module exited\n");
    if (sock_listen) {
        sock_release(sock_listen);
        sock_listen = NULL;
    }
    printk(KERN_INFO "Server module exited\n");
}

module_init(server_init);
module_exit(server_exit);
