#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#define CHRDEVBASE_MAJOR 200
#define CHRDEVBASE_NAME  "chrdevbase"

static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"kernel data!"};

static struct file_operations chrdevbase_fops;

static int chrdevbase_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = 0;
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    retvalue = copy_to_user(buf, readbuf, cnt);
    if (retvalue == 0) {
        printk("kernel sendata ok!\r\n");
    } else {
        printk("kernel sendata failed!\r\n");
    }
    return 0;
}

static ssize_t chrtest_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue = copy_from_user(writebuf, buf, cnt);
    if (retvalue == 0) {
        printk("kernel recevdata:%s ok!\r\n", writebuf);
    } else {
        printk("kernel recevdata failed!\r\n");
    }
    return 0;
}

static int chrtest_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrtest_write,
    .release = chrtest_release,

};

static int __init chrdev_init(void)
{
    int retvalue = 0;
    retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (retvalue < 0) {
        printk("chrdevbase driver register failed\r\n");
    }
    printk("chrdevbase init\r\n");
    return 0;
}

static void __exit chrdev_exit(void)
{
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
    printk("chrdevbase exit\r\n");
    return;
}

module_init(chrdev_init);
module_exit(chrdev_exit);
MODULE_LICENSE("GPL v2.0");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
