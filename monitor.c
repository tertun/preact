#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/inet.h>
#include <linux/in.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 3000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("bhhoang");
MODULE_DESCRIPTION("Monitor Module send via kernel socket");

// Callback function for receiving messages from the device module
void monitor_callback(const char *data, size_t length) {
    printk(KERN_INFO "Monitor: Received %zu bytes of data.\n", length);
    printk(KERN_INFO "Received data: %s\n", data);

    // Send the received data to the server
    struct socket *sock;
    struct sockaddr_in sin;
    struct msghdr msg;
    struct kvec vec;

    int len, ret;

    // Create a socket
    ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret < 0) {
        printk(KERN_ERR "Client: Socket creation failed\n");
        return;
    }

    // Specify server address
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton(SERVER_IP);
    sin.sin_port = htons(SERVER_PORT);

    // Connect to server
    ret = kernel_connect(sock, (struct sockaddr *)&sin, sizeof(sin), 0);
    if (ret < 0) {
        printk(KERN_ERR "Client: Connect failed\n");
        sock_release(sock);
        return;
    }

    // Prepare message
    memset(&msg, 0, sizeof(msg));
    vec.iov_base = (void *)data;
    vec.iov_len = length;

    // Send message
    len = kernel_sendmsg(sock, &msg, &vec, 1, length);
    (len > 0) ? printk(KERN_INFO "Client: Message sent with content: %s and length: %d\n", data, len) : printk(KERN_ERR "Client: Send failed with error %d\n", len);
    // Cleanup
    sock_release(sock);

    return;    
}
EXPORT_SYMBOL(monitor_callback);

static int __init monitor_init(void) {
    printk(KERN_INFO "Monitor: Initializing the monitor module\n");
    return 0;
}

static void __exit monitor_exit(void) {
    printk(KERN_INFO "Monitor: Exiting monitor module\n");
}



module_init(monitor_init);
module_exit(monitor_exit);
