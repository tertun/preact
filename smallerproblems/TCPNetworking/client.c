#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/inet.h>
#include <linux/in.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bhhoang");


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 3000

static int __init client_init(void)
{
    struct socket *sock;
    struct sockaddr_in sin;
    struct msghdr msg;
    struct kvec vec;
    // 
    // char *data = "Hello, from the client kernel!"; // Message = "exit" will stop the server
    char *data = "exit"; // Message = "exit" will stop the server
    
    int len, ret;

    // Create a socket
    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret < 0) {
        printk(KERN_ERR "Client: Socket creation failed\n");
        return ret;
    }

    // Specify server address
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton(SERVER_IP); // Server IP
    sin.sin_port = htons(SERVER_PORT); // Server port

    // Connect to server
    ret = kernel_connect(sock, (struct sockaddr *)&sin, sizeof(sin), 0);
    if (ret < 0) {
        printk(KERN_ERR "Client: Connect failed\n");
        sock_release(sock);
        return ret;
    }

    // Prepare message
    memset(&msg, 0, sizeof(char));
    vec.iov_base = data;
    vec.iov_len = strlen(data);

    // Send message
    len = kernel_sendmsg(sock, &msg, &vec, 1, sizeof(msg));
    (len > 0) ? printk(KERN_INFO "Client: Message sent with content: %s\n and length: %d\n", data, len) : printk(KERN_ERR "Client: Send failed with error %d\n", len);
    // Cleanup
    sock_release(sock);
    return 0;
}

static void __exit client_exit(void)
{
    printk(KERN_INFO "Client module exited\n");
}

module_init(client_init);
module_exit(client_exit);
