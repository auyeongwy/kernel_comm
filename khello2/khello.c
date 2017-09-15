/** @file khello.c
 * 
 * Simple kernel driver that creates a character device in /dev/ and allows receiving and sending data to-and-from userland.
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
#include <linux/cdev.h>
#include <linux/poll.h>

#define DEVICE_NAME "khello" /**< Name of the device in /dev */
#define CLASS_NAME "khello_class" /**< Device class. */


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Au Yeong Wing Yau");
MODULE_DESCRIPTION("Simple module with character device");
MODULE_VERSION("0.1");


static dev_t g_dev_num=0; /**< The dev number. */
static struct cdev g_c_device; /**< Character device. */
static struct class *g_class = NULL; /**< Device class. */
static struct device *g_device = NULL; /**< The device itself. */
static unsigned char g_data[32]; /**< A simple data buffer to hold data sent to this device. */
static size_t g_data_size; /**< Size of data in g_data. */
static DEFINE_MUTEX(g_mutex); /**< Mutex for thread-safety. */
static int g_lock=0;

/**Opens the device. Implements the open function defined in linux/fs.h */
static int dev_open(struct inode *p_inode, struct file *p_file);

/**Releases the device. Implements the release function defined in linux/fs.h */
static int dev_release(struct inode *p_inode, struct file *p_file);

/**Reads data from the device. Implements the read function defined in linux/fs.h */
static ssize_t dev_read(struct file *p_file, char *p_buf, size_t p_size, loff_t *p_off);

/**Writes data to the device. Implements the write function defined in linux/fs.h */
static ssize_t dev_write(struct file *p_file, const char *p_buf, size_t p_size, loff_t *p_off);

/** Returns results to a poll or select call. Implements the funciton defined in linux/fs.h  */
unsigned int dev_poll(struct file *p_file, struct poll_table_struct *p_table);


/** Defines the file_operations structure for device operations.
 */
static struct file_operations g_fops =
{
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.poll = dev_poll,
	.release = dev_release,
};


/** Kernel module init funciton.
 * @return 0 if success, else non-zero value.
 */
static int __init hello_init(void)
{
	int result = 0, progress = 0; 
	memset(g_data, 0 ,32);
	g_data_size = 0;
	mutex_init(&g_mutex);
    printk(KERN_INFO "khello: Init\n");

	/* Request major number from kernel. */
	if((result = alloc_chrdev_region(&g_dev_num, 0, 1, DEVICE_NAME)) < 0) {
		printk(KERN_ALERT "khello: request device number failed\n");
		goto do_exit;
	}
	
	/* Create character device. */
	cdev_init(&g_c_device, &g_fops);
	g_c_device.owner = THIS_MODULE;
	if((result = cdev_add(&g_c_device, g_dev_num, 1)) < 0) {
		printk(KERN_ALERT "khello: character device creation failed\n");
		goto do_exit;
	}
	++progress;
	
	/* Create device class */
	g_class = class_create(THIS_MODULE, CLASS_NAME);
	if(IS_ERR(g_class)) {
		printk(KERN_ALERT "khello: device class creation failed\n");
		goto do_exit;
	}
	++progress;
	
	/* Create device itself. */
	g_device = device_create(g_class, NULL, g_dev_num, NULL, DEVICE_NAME);
	if(IS_ERR(g_device)) {
		printk(KERN_ALERT "khello: device creation failed\n");
		goto do_exit;		
	}
	++progress;
	
	printk(KERN_INFO "khello: device created\n");
	
	
do_exit:
	/* Device creation failure so clean up. */
	if(result < 0) {
		if(progress == 2) {
			class_destroy(g_class);
			progress = 1;
		}
		if(progress == 1) {
			cdev_del(&g_c_device);
			progress = 0;
		}
		if(progress==0)
			unregister_chrdev_region(g_dev_num, 1);
		mutex_destroy(&g_mutex);
	}
	
    return result;
}



/** Module exit cleanup function.
 */
static void __exit hello_cleanup(void)
{
	device_destroy(g_class, g_dev_num);
	class_destroy(g_class);
	cdev_del(&g_c_device);
	unregister_chrdev_region(g_dev_num, 1);
	mutex_destroy(&g_mutex);
    printk(KERN_INFO "khello: Cleanup and exit\n");
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
	g_lock = 1;
	result = copy_to_user(p_buf, g_data, g_data_size);
	if(result == 0)
		printk(KERN_INFO "khello: sent %zu bytes\n", g_data_size);
	else
		printk(KERN_ALERT "khello: failed to send %lu bytes\n", result);
	g_lock = 0;
	mutex_unlock(&g_mutex); /* Mutex unlock */ 
	return result;
}



static ssize_t dev_write(struct file *p_file, const char *p_buf, size_t p_size, loff_t *p_off)
{
	if(p_size > 31)
		p_size = 31;
	mutex_lock(&g_mutex); /* Mutex lock to access shared data. */
	g_lock = 1;
	memcpy(g_data, p_buf, p_size);
	g_data[p_size] = 0;
	g_data_size = p_size;
	printk(KERN_INFO "khello: Received from user:%s\n", g_data);
	g_lock = 0;
	mutex_unlock(&g_mutex); /* Mutex unlock. */
	return p_size;
}



unsigned int dev_poll(struct file *p_file, struct poll_table_struct *p_table)
{
	unsigned int result;

	if(g_data_size > 0) /* Data is availalable for reading. */
		result = POLLIN;
	if(g_lock == 0) /* Reading will not block */
		result = POLLOUT;
	
	return result;
}



module_init(hello_init);
module_exit(hello_cleanup);