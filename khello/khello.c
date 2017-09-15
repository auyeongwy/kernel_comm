/** @file khello.c
 * 
 * Simple kernel driver that creates a character device in /dev/ and allows receiving and sending data to-and-from userland.
 * This example uses the older driver creation mechanics.
 * 
 * Usage:
 * Load the module: "insmod khello.ko"
 * See module messages: "tail -f /var/log/messages"
 * Unload the module: "rmmod khello.ko"
 * Write data to the module: "echo hello > /dev/khello"
 * Read data from the module: "cat /dev/khello" or open using an application.
 */

#include <linux/init.h> 
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "khello" /**< Name of the device in /dev */
#define CLASS_NAME "khello_class" /**< Device class. */


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Au Yeong Wing Yau");
MODULE_DESCRIPTION("Simple module with character device");
MODULE_VERSION("0.1");


static int g_major_num; /**< Device major number. */
static struct class *g_dev_class  = NULL; /**< Device driver class struc. */
static struct device *g_device = NULL; /**< Device driver device struc. */
static unsigned char g_data[32]; /**< A simple data buffer to hold data sent to this device. */
static size_t g_data_size; /**< Size of data in g_data. */
static DEFINE_MUTEX(g_mutex); /**< Mutex for thread-safety. */

/**Opens the device. Implements the open function defined in linux/fs.h */
static int dev_open(struct inode *p_inode, struct file *p_file);

/**Releases the device. Implements the release function defined in linux/fs.h */
static int dev_release(struct inode *p_inode, struct file *p_file);

/**Reads data from the device. Implements the read function defined in linux/fs.h */
static ssize_t dev_read(struct file *p_file, char *p_buf, size_t p_size, loff_t *p_off);

/**Writes data to the device. Implements the write function defined in linux/fs.h */
static ssize_t dev_write(struct file *p_file, const char *p_buf, size_t p_size, loff_t *p_off);


/** Defines the file_operations structure for device operations.
 */
static struct file_operations g_fops =
{
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};


/** Kernel module init funciton.
 * @return 0 if success, else non-zero value.
 */
static int __init hello_init(void)
{
	int result = 0;
	memset(g_data, 0 ,32);
	g_data_size = 0;
	mutex_init(&g_mutex);
    printk(KERN_INFO "khello: Init\n");
	
	/* Register major number. */
	g_major_num = register_chrdev(0, DEVICE_NAME, &g_fops);
	if (g_major_num < 0){
		printk(KERN_ALERT "khello: register major num failed\n");
		result = g_major_num;
		goto do_exit;
	}
	printk(KERN_INFO "khello: registered major num %d\n", g_major_num);
	
	/* Register device class */
	g_dev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(g_dev_class)){
		printk(KERN_ALERT "khello: failed to register device class\n");
		result = PTR_ERR(g_dev_class); /* Correct way to return an error on a pointer */
		goto do_exit;
   }
   printk(KERN_INFO "khello: device class registered\n");
   
    /* Register device driver */
	g_device = device_create(g_dev_class, NULL, MKDEV(g_major_num, 0), NULL, DEVICE_NAME);
	if(IS_ERR(g_device)){
		printk(KERN_ALERT "khello: failed to create device\n");
		result = PTR_ERR(g_device);
		goto do_exit;
	}
	printk(KERN_INFO "khello: device created\n");
	
	
do_exit:
	if(result < 0) {
		if(IS_ERR(g_device))
			class_destroy(g_dev_class);
		unregister_chrdev(g_major_num, DEVICE_NAME);
		mutex_destroy(&g_mutex);
	}
    return result;
}



/** Module exit cleanup function.
 */
static void __exit hello_cleanup(void)
{
	device_destroy(g_dev_class, MKDEV(g_major_num, 0));
	class_unregister(g_dev_class);
	class_destroy(g_dev_class);
	unregister_chrdev(g_major_num, DEVICE_NAME);
	mutex_destroy(&g_mutex);
    printk(KERN_INFO "khello: Exit\n");
}



static int dev_open(struct inode *p_inode, struct file *p_file)
{
	return 0;
}



static int dev_release(struct inode *p_inode, struct file *p_file)
{
	return 0;
}



static ssize_t dev_read(struct file *p_file, char *p_buf, size_t p_size, loff_t *p_off)
{
	unsigned long result;
	
	mutex_lock(&g_mutex); /* Mutex lock to access shared data. */
	result = copy_to_user(p_buf, g_data, g_data_size);
	if(result == 0)
		printk(KERN_INFO "khello: sent %zu bytes\n", g_data_size);
	else
		printk(KERN_ALERT "khello: failed to send %lu bytes\n", result);
	mutex_unlock(&g_mutex); /* Mutex unlock */ 
	return result;
}



static ssize_t dev_write(struct file *p_file, const char *p_buf, size_t p_size, loff_t *p_off)
{
	if(p_size > 31)
		p_size = 31;
	mutex_lock(&g_mutex); /* Mutex lock to access shared data. */
	memcpy(g_data, p_buf, p_size);
	g_data[p_size] = 0;
	g_data_size = p_size;
	printk(KERN_INFO "khello: Received from user:%s\n", g_data);
	mutex_unlock(&g_mutex); /* Mutex unlock. */
	return p_size;
}




module_init(hello_init);
module_exit(hello_cleanup);