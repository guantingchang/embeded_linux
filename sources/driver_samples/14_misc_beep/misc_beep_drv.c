#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define GPIO_BEEP_CNT 1
#define GPIO_BEEP_NAME "misc_beep"
#define MISC_BEEP_MINOR  144
#define BEEP_ON    1
#define BEEP_OFF   0

struct beep_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int beep_gpio;
};

struct beep_dev gpio_beep;

static int beep_gpio_init(struct device_node *nd)
{
    int ret;

    gpio_beep.beep_gpio = of_get_named_gpio(nd, "beep-gpio", 0);
    if(gpio_is_valid(gpio_beep.beep_gpio)== 0){
        printk("can't get beep-gpio");
        return -EINVAL;
    }
    printk("beep-gpio num = %d\r\n", gpio_beep.beep_gpio);
    ret = gpio_request(gpio_beep.beep_gpio, "BEEP");
    if(ret){
        printk(KERN_ERR "gpio_beep:failed to request beep-gpio:%d\r\n",ret);
        return ret;
    }
    ret = gpio_direction_output(gpio_beep.beep_gpio, 1);
    if(ret < 0){
        printk("can't set gpio!\r\n");
    }
    return 0;
}


static int beep_open(struct inode *inode, struct file *filep)
{
    filep->private_data = &gpio_beep;
    printk("beep open success");
    return 0;
}

static ssize_t beep_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int ret;
    unsigned char data_buf[1];
    unsigned char beep_state;
    struct beep_dev *dev = filp->private_data;

    ret = copy_from_user(data_buf, buf, cnt);
    if(ret < 0){
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    beep_state = data_buf[0];
    printk("beep state: %d\r\n", beep_state);
    if(beep_state == BEEP_ON){
        gpio_set_value(dev->beep_gpio, 0);
    }else if(beep_state == BEEP_OFF){
        gpio_set_value(dev->beep_gpio, 1);
    }
    return 0;
}

static int beep_release(struct inode *inode, struct file *filp)
{

    return 0;
}


static struct file_operations gpio_beep_fops={
    .owner = THIS_MODULE,
    .open  = beep_open,
    .read  = beep_read,
    .write = beep_write,
    .release = beep_release,
};

static struct miscdevice beep_miscdev = {
    .minor = MISC_BEEP_MINOR,
    .name = GPIO_BEEP_NAME,
    .fops = &gpio_beep_fops,
};

static int misc_beep_probe(struct platform_device *pdev)
{
    int ret = 0;
    printk("beep driver and device was matched\r\n");
    ret = beep_gpio_init(pdev->dev.of_node);
    if(ret < 0){
        return ret;
    }
    ret = misc_register(&beep_miscdev);
    if(ret < 0){
        printk("misc device register failed\r\n");
        goto free_gpio;
    }
    return 0;
free_gpio:
        gpio_free(gpio_beep.beep_gpio);
        return -EINVAL;
}

static int misc_beep_remove(struct platform_device *pdev)
{
    gpio_set_value(gpio_beep.beep_gpio, 1);
    gpio_free(gpio_beep.beep_gpio);
    misc_deregister(&beep_miscdev);
    printk("beep exit\r\n");
    return 0;
}

static const struct of_device_id beep_of_match[] = {
    {.compatible = "atk,beep"},
    {/* sentinel */},
};

static struct platform_driver beep_driver = {
    .driver = {
        .name = "stm32mp1-beep",
        .of_match_table = beep_of_match,
    },
    .probe = misc_beep_probe,
    .remove =  misc_beep_remove,
};

static int __init misc_beep_init(void)
{
    return platform_driver_register(&beep_driver);
}

static void __exit misc_beep_exit(void)
{
     platform_driver_unregister(&beep_driver);
}


module_init(misc_beep_init);
module_exit(misc_beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
