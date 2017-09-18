/// <summary>
///  Create a character device driver inside of Linux
///  Writting/reading character by character
///  Synchronization with operations
/// </summary>


#include <linux/module.h>			// Allow to create module
#include <linux/kernel.h>			// Allow to access kernel library
#include <linux/fs.h> 				// Allow to use file_operations
#include <linux/cdev.h>				// Allow to registrate your device to the Kernel
#include <linux/semaphore.h>		// Allow synchronization
#include <asm/uaccess.h>			// Map data from user space to Kernel space

// create myDevice structure and
struct my_device
{
	char data[100];
	struct semaphore sem;
} virtual_device;

/// <summary>
/// Global space where we declare variables
/// </summary>

struct cdev *mcdev;					// My character device driver
int major_number;					// The major number that will optain from dev_num structure
int ret;							// The totaly amount of bytes that we actually writen/readen 

dev_t dev_num;						// The name of our device drive

#define DEVICE_NAME "adinadevice"

struct file_operations fops = {
	.owner = THIS_MODULE,			// Prevent unloading of this module when operations are user
	.open = device_open,			// Points to the method to call when we open the device
	.release = device_close,		// Points to the method to call when we close the device
	.write = device_write,			// Points to the method to call when we write the device
	.read = device_read				// Points to the method to call when we read the device
}
///<summary>
// Caled on device file open
// Inode reference to the file on disk
// And contrains information about that file
// The struct file is represent an abstract open file
///</succes>
int device_open(struct inode *inode,struct file *filp)
{
	// Only allow one process to open this device by using a semaphore as mutex = mutual exclusive looks
	if(down_interruptible(&virtual_device.sem) !=0)
	{
		printk(KERN_ALERT "adinacode:could not lock device during open");
		return -1;
	}
	printk(KERN_INFO "adinacode:opened device");
	return 0;
}

///<summary>
// Called when user wants to get information from the device
///</summary>
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset)
{
	// Take data from Kernel space(device) to user space (process)
	// Copy_to_user(destination, source, sizeToTransfer)
	printk(KERN_INFO "adinacode:Reading from device");
	ret = copy_to_user(bufStoreData, virtual_device.data, bufCount);	
	return ret;	
}

///<summary>
// Called when user wants to sent information to the device
///</summary>
ssize_t device_write(struct file* filp,const char* bufSourceData, size_t bufCount, loff_t* curOffset)
{
	// Sent data from user to Kernel
	// Copy_from_user(dest,source,count)
	printk(KERN_INFO "adinacode:writing to device");
	ret=copy_from_user(virtual_device.data, bufSourceData, bufCount);
	return ret;
}

///<summary>
// Called upon user close
///</summary>
int device_close(struct *inode,struct file *filp)
{
	// By calling up , we release the mutex 
	// Effect: allowing other process to use the device now
	up(&virtual_device.sem);
	printk(KERN_INFO "adinacode:closed device");
	return 0;
}

///<summary>
// Register our device with the system: 2 steps process
///</summary>
static int driver_entry(void)
{	
	// Step1 : use dynamic allocation to asign our device
	// 		  a major number -- alloc chrdev_region(dev_t*, uint fminor, uint count, char* name)
	ret=alloc_chrdev_region(&dev_num,0,1,DEVICE_NAME);
	if(ret<0)
	{	// If kernel functionsa return negative => this means it is an error
		printk(KERN_ALERT "adinacode: failled to alocate a major number");
		// Propagate the error
		return ret;
	}
	// Extract the major number and store in our variable (MACRO)
	major_number = MAJOR(dev_num);
	printk(KERN_INFO "adinacode:major number is %d",major_number);
	// dmesg
	printk(KERN_INFO "\tuse\"mknod /dev/%s c %d o\" for device file",DEVICE_NAME,major_number);
	
	// Step 2: alocate the character device structure and to initialize it 
	mcdev = cdev_alloc();
	// fops = struct file operations
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;
	
	// Now that we created cdev, we have to add it to the Kernel
	// int cdev_add(struct cdev*, dev_t num, unsigned int count);
	ret=cdev_add(mcdev,dev_num,1);
	// Check for errors
	if(ret<0)
	{
		printk(KERN_ALERT "adinacode:unable to add cdev to Kernel");
		// Propagating the error
		return ret;
	}
	
	// Initialize our semaphore
	sema_init(&virtual_device.sem,1);
	return 0;
}

///<summary>
// Unregister everithing in revers order
///</summary>
static void driver_exit(void)
{
	// Remove the character device from the system
	cdev_del(mcdev);
	
	// Unregister the character device that originaly we register
	unregister_chrdev_region(dev_num,1);
	printk(KERN_ALERT "adinacode:unloaded module");
}

// Inform the Kernel where to start and where to stop
module_init(driver_entry);
module_exit(driver_exit);