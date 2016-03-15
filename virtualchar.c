#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

/*Memory size is 4096 bytes*/
#define MEM_SIZE 0x1000
#define MEM_CLEAR 0x1

#ifndef VIRTUALCHAR_MAJOR 
#define VIRTUALCHAR_MAJOR 148
#endif

static int virtualchar_major = VIRTUALCHAR_MAJOR;
struct virtualchar_dev
{
	struct cdev cdev;
	unsigned char mem[MEM_SIZE];
};

struct virtualchar_dev *virtualchar_devp;

int virtualchar_open(struct inode *inode, struct file * filp)
{
	/*Get device struct pointer from passed-in inode*/
	struct virtualchar_dev *dev;
	dev = container_of(inode->i_cdev, struct virtualchar_dev, cdev);

	/*Assign device struct pointer to private data pointer*/
	filp->private_data = dev;
	return 0;
}

int virtualchar_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*Device read functions, referenced in virtualchar_fops*/
static ssize_t virtualchar_read (struct file * filp, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct virtualchar_dev *dev = filp->private_data;

	/*Error: Offset value out of bound*/
	if (p >= MEM_SIZE)
		return 0;

	/*Error: Read count larger than available*/
	if (count > MEM_SIZE - p)
		count = MEM_SIZE - p;

	/*Read data from kernel space to user space*/
	if (copy_to_user(buf, (void *)(dev->mem + p), count))
		ret = -EFAULT;
	else
	{
		*ppos += count;
		ret = count;
		printk(KERN_INFO "Read %d bytes(s) from %li\n", count, p);
	}

	return ret;
}

static ssize_t virtualchar_write (struct file * filp, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct virtualchar_dev *dev = filp->private_data;

	/*Error: Offset value out of bound*/
	if (p >= MEM_SIZE)
		return 0;

	/*Error: Write count larger than available*/
	if (count > MEM_SIZE - p)
		count = MEM_SIZE - p;

	if (copy_from_user(dev->mem + p, buf, count))
		ret = -EFAULT;
	else
	{
		*ppos += count;
		ret = count;
		printk(KERN_INFO "Write %d bytes(s) from %li\n", count, p);
	}
	return ret;
}

/*Assign file_operation functions*/
static const struct file_operations virtualchar_fops = {
	.owner = THIS_MODULE,
	.read = virtualchar_read,
	.write = virtualchar_write,
	.open = virtualchar_open,
	.release = virtualchar_release,
};

/*Part of init function*/
static void virtualchar_setup_cdev(struct virtualchar_dev *dev, int index)
{
	int err, devno = MKDEV(virtualchar_major, index);

	/*Init global memory, associate file_fops struct*/
	cdev_init(&dev->cdev, &virtualchar_fops);
	dev->cdev.owner = THIS_MODULE;
	/*Add cdev to system, associate with device number*/
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding globalmem", err);
}

int virtualchar_init (void)
{
	int result;
	dev_t devno = MKDEV(virtualchar_major, 0);

	/*Apply char device driver region, specify display names at /proc/devices and sysfs*/
	if (virtualchar_major)
		result = register_chrdev_region(devno, 1, "virtualchar");
	else
	/*Dynamically allocate major/minor numbers*/
	{
		result = alloc_chrdev_region(&devno, 0, 1, "virtualchar");
		virtualchar_major = MAJOR(devno);
	}

	if (result < 0)
		return result;

	/*Dynamically allocate device struct memory*/
	virtualchar_devp = kmalloc(1*sizeof(struct virtualchar_dev), GFP_KERNEL);
	if (! virtualchar_devp)
	{
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(virtualchar_devp, 0, 1*sizeof(struct virtualchar_dev));

	/*Initialize and add cdev*/
	virtualchar_setup_cdev(&virtualchar_devp[0], 0);
	return 0;

fail_malloc: unregister_chrdev_region(devno, 1);
			 return result;
}

void virtualchar_exit (void)
{
	/*Delete cdev node*/
	cdev_del(&(virtualchar_devp[0].cdev));

	/*Release the allocated memory*/
	kfree(virtualchar_devp);
	/*Release device number*/
	unregister_chrdev_region(MKDEV(virtualchar_major, 0), 1);
}

MODULE_AUTHOR("Ji Bo");
MODULE_LICENSE("Dual BSD/GPL");

module_param(virtualchar_major, int, S_IRUGO);
MODULE_PARM_DESC(virtualchar_major, "This is the device major number,148 is the initialization number");
module_init(virtualchar_init);
module_exit(virtualchar_exit);
