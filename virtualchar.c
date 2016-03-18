#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include<linux/miscdevice.h>

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

static loff_t virtualchar_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret;
	switch (orig)
	{
		/*Offset start from the beginning of the file*/
		case 0:
			/*Error: Offset value is nagative*/
			if (offset < 0)
			{
				ret = -EINVAL;
				break;
			}
			/*Error: Offset value out of bound*/
			if ((unsigned int)offset > MEM_SIZE)
			{
				ret = -EINVAL;
				break;
			}
			filp->f_pos = (unsigned int) offset;
			ret = filp->f_pos;
			break;
		/*Offset start from current position*/
		case 1:
			if ((filp->f_pos + offset) > MEM_SIZE)
			{
				ret = -EINVAL;
				break;
			}
			if ((filp->f_pos + offset) < 0)
			{
				ret = -EINVAL;
				break;
			}
			filp->f_pos += offset;
			ret = filp->f_pos;
			break;
		default:
			ret = -EINVAL;
	}
	return ret;
}

static long virtualchar_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct virtualchar_dev *dev = filp->private_data;

	switch (cmd)
	{
		/*Command to clear global memory*/
		case MEM_CLEAR:
			memset(dev->mem, 0, MEM_SIZE);
			printk(KERN_INFO "virtualchar memory is set to zero\n");
			break;

		default:
			return -EINVAL;
	}
	return 0;
}


/*Assign file_operation functions*/
static const struct file_operations virtualchar_fops = {
	.owner = THIS_MODULE,
	.read = virtualchar_read,
	.write = virtualchar_write,
	.open = virtualchar_open,
	.llseek = virtualchar_llseek,
	.unlocked_ioctl = virtualchar_ioctl,
	.release = virtualchar_release,
};

static struct miscdevice misc={
	.minor = MISC_DYNAMIC_MINOR,
	.name = "virtualchar",
	.fops = &virtualchar_fops,
};

int virtualchar_init (void)
{
	int result;

	/*Dynamically allocate device struct memory*/
	virtualchar_devp = kmalloc(1*sizeof(struct virtualchar_dev), GFP_KERNEL);
	if (! virtualchar_devp)
	{
		result = -ENOMEM;
		return result;
	}
	memset(virtualchar_devp, 0, 1*sizeof(struct virtualchar_dev));
	result = misc_register(&misc);

	return 0;


}

void virtualchar_exit (void)
{
	kfree(virtualchar_devp);
	misc_deregister(&misc);
}

MODULE_AUTHOR("Ji Bo");
MODULE_LICENSE("Dual BSD/GPL");

module_param(virtualchar_major, int, S_IRUGO);
MODULE_PARM_DESC(virtualchar_major, "This is the device major number,148 is the initialization number");
module_init(virtualchar_init);
module_exit(virtualchar_exit);
