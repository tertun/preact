#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/atomic.h>
#include <asm/errno.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "virtualdevice"
#define CLASS_NAME "virtual"
#define SUCCESS 0 
#define BUF_LEN 80 /* Max length of the message from the device */ 


MODULE_AUTHOR("bhhoang");
MODULE_DESCRIPTION("A simple Linux char driver");
MODULE_LICENSE("GPL");
// Reference: https://sysprog21.github.io/

// enum for state of the device
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
};

// static variables
static int majorNumber;
static struct class* virtualClass = NULL;
static struct device* virtualDev = NULL;
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 

static char msg[BUF_LEN + 1]; 

// static function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
extern void monitor_callback(const char *data, size_t length);

// file operations
static struct file_operations fops =
{
   .open = dev_open,
   .write = dev_write,
   .release = dev_release,
};

static int __init virtualdevice_init(void) {
  printk(KERN_INFO "Initializing the Example LKM\n");

  // Try to dynamically allocate a major number for the device
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if (majorNumber < 0) {
    printk(KERN_ALERT "Example failed to register a major number\n");
    return majorNumber;
  }
  
  printk(KERN_INFO "Registered correctly with major number %d\n", majorNumber);

// Register the device class
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
    virtualClass = class_create(DEVICE_NAME); 
#else 
    virtualClass = class_create(THIS_MODULE, DEVICE_NAME); 
#endif 

  if (IS_ERR(virtualClass)) {
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "Failed to register device class\n");
    return PTR_ERR(virtualClass);
  }
  printk(KERN_INFO "Device class registered correctly\n");
 // Register the device driver
  virtualDev = device_create(virtualClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
  if (IS_ERR(virtualDev)) {
    class_destroy(virtualClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "Failed to create the device\n");
    return PTR_ERR(virtualDev);
  }
  printk(KERN_INFO "Device class created correctly\n");
  return 0;
}

// Cleanup
static void __exit virtualdevice_exit(void) {
  device_destroy(virtualClass, MKDEV(majorNumber, 0));
  class_unregister(virtualClass);
  
  class_destroy(virtualClass);
  unregister_chrdev(majorNumber, DEVICE_NAME);
  printk(KERN_INFO "Goodbye from the LKM!\n");
}
// Called when a process tries to open the device file
static int dev_open(struct inode *inodep, struct file *filep) {
   // Allow only one process to open this device by using atomic_cmpxchg
   if (atomic_cmpxchg(&already_open, 0, 1) != 0) {
      printk(KERN_ALERT "Device is in use by another process");
      return -EBUSY;
   }
   printk(KERN_INFO "Device has been opened\n");
   return SUCCESS;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    size_t bytes_to_copy = min(len, (size_t)BUF_LEN);
    memset(msg, 0, BUF_LEN + 1);
    if (copy_from_user(msg, buffer, bytes_to_copy) != 0) {
      printk(KERN_ALERT "Failed to copy data from user\n");
      return -EFAULT;
    }
    monitor_callback(msg, bytes_to_copy);
   //printk(KERN_INFO "Received %zu characters from the user\n", len);
   //monitor_callback(buffer, len);
   return bytes_to_copy;
}

static int dev_release(struct inode *inodep, struct file *filep) {
   atomic_set(&already_open, CDEV_NOT_USED); // Reset the atomic variable to allow next open
   printk(KERN_INFO "Device successfully closed\n");
   return 0;
}

module_init(virtualdevice_init);
module_exit(virtualdevice_exit);
