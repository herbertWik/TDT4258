//////////  Custom gamepad driver for TDT4258 ////////// 


////////// Includes ////////// 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/signal.h>
#include <asm/siginfo.h>
#include <asm/io.h>
#include "efm32gg.h"


////////// Defines ////////// 
#define DRIVER_NAME "Gamepad_Driver"
#define GPIO_EVEN_IRQ_LINE 17
#define GPIO_ODD_IRQ_LINE 18


////////// Variable Declarations //////////  
struct fasync_struct* Queue;
static dev_t Dev_Nr;
struct cdev Driver_cdev;
struct class* Class;


////////// Function Declarations //////////  
static irqreturn_t gpio_interrupt_handler(int, void*, struct pt_regs*);
static int __init Init_Driver(void);
static void __exit Exit_Driver(void);
static ssize_t Write_Driver(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
static ssize_t Read_Driver(struct file *filp, char __user *buff, size_t count, loff_t *offp);
static int Open_Driver(struct inode *inode, struct file *filp);
static int Release_Driver(struct inode* inode, struct file* filp);
static int Fasync_Driver(int fd, struct file *filp, int mode);


//////////  Maps the file operations to local functions ////////// 
static struct file_operations Driver_fops = 
{
    .owner = THIS_MODULE,
    .read = Read_Driver,
    .write = Write_Driver,
    .open = Open_Driver,
    .release = Release_Driver,
    .fasync = Fasync_Driver
};


////////// Interrupt Handler //////////  
irqreturn_t gpio_interrupt_handler(int irq, void* dev_id, struct pt_regs* regs)
{
    iowrite32(ioread32(GPIO_IF), GPIO_IFC);
    if (Queue)
    {
        kill_fasync(&Queue, SIGIO, POLL_IN);
    }
    return IRQ_HANDLED;
}


////////// Initializing Driver //////////  
static int __init Init_Driver(void)
{
	printk("Initializing gamepad driver. \n");
	
	
	////////// Allocate device number //////////
	if (alloc_chrdev_region(&Dev_Nr,0,1,DRIVER_NAME) < 0)
	{
		printk(KERN_ALERT "Error: Failed to allocate device number. \n");
		return -1;
	}
	
	
	////////// Allocate IO Ports //////////
	if (request_mem_region(GPIO_PC_MODEL,1,DRIVER_NAME) == NULL)
	{
		printk(KERN_ALERT "Error: Failed to allocate GPIO_PC_MODEL memory region \n");
		return -1;
	}
	
	if (request_mem_region(GPIO_PC_DIN,1,DRIVER_NAME) == NULL)
	{
		printk(KERN_ALERT "Error: Failed to allocate GPIO_PC_IN memory region \n");
		return -1;
	}
	
	if (request_mem_region(GPIO_PC_DOUT,1,DRIVER_NAME) == NULL)
	{
		printk(KERN_ALERT "Error: Failed to allocate GPIO_PC_OUT memory region \n");
		return -1;
	}
	
	
	////////// Setting up buttons //////////
	iowrite32(0x33333333, GPIO_PC_MODEL);
    iowrite32(0xFF, GPIO_PC_DOUT);
	
	
	////////// Setting up interrupt //////////
    request_irq(GPIO_EVEN_IRQ_LINE, (irq_handler_t)gpio_interrupt_handler, 0, DRIVER_NAME, &Driver_cdev);
    request_irq(GPIO_ODD_IRQ_LINE, (irq_handler_t)gpio_interrupt_handler, 0, DRIVER_NAME, &Driver_cdev);
    
    iowrite32(0x22222222, GPIO_EXTIPSELL);
	iowrite32(0xFF, GPIO_EXTIFALL);
	iowrite32(0xFF, GPIO_IEN);
	
	////////// Add device to kernal //////////
    cdev_init(&Driver_cdev, &Driver_fops);
    Driver_cdev.owner = THIS_MODULE;
    cdev_add(&Driver_cdev, Dev_Nr, 1);
    
    Class = class_create(THIS_MODULE, DRIVER_NAME);
    device_create(Class, NULL, Dev_Nr, NULL, DRIVER_NAME);
    
    printk(KERN_INFO "Initializing of gamepad driver successful. \n");
    
    return 0;
}


////////// Exit Driver //////////  
static void __exit Exit_Driver(void)
{
	printk(KERN_INFO "Exiting gamepad driver \n");
}


////////// Write to Driver //////////  
static ssize_t Write_Driver(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	printk(KERN_INFO "Warning: Trying to write to buttons. \n");
	return 1;
}


////////// Read from Driver //////////  
static ssize_t Read_Driver(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	int Val = ioread32(GPIO_PC_DIN);
	copy_to_user(buff, &Val, 4);
	return 1;
}


////////// Open Driver //////////  
static int Open_Driver(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "Opening driver. \n");
	return 0;
}


////////// Release Driver //////////  
static int Release_Driver(struct inode* inode, struct file* filp)
{
    printk(KERN_INFO "Releasing driver. \n");
    return 0;
}


//////////  Fasync Helper //////////  
static int Fasync_Driver(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &Queue);
}


////////// Module configs ////////// 
module_init(Init_Driver);
module_exit(Exit_Driver);
MODULE_DESCRIPTION("Gamepad Driver for TDT4258");
MODULE_LICENSE("GPL");
