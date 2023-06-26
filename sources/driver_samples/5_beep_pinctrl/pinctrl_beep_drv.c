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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define GPIO_BEEP_CNT 1
#define GPIO_BEEP_NAME "gpio_beep"
#define BEEP_ON    1
#define BEEP_OFF   0

struct beep_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int beep_gpio;
};

struct beep_dev gpio_beep;


static int beep_open(struct inode *inode, struct file *filep)
{
    int ret;
    const char* str;

    gpio_beep.nd = of_find_node_by_path("/beep");
    if(gpio_beep.nd == NULL){
        printk("stm32mp1_beep node not find!\r\n");
        return -EINVAL;
    }else{
        printk("stm32mp1_beep node find!\r\n");
    }
    ret = of_property_read_string(gpio_beep.nd, "compatible", &str);
    if(ret < 0){
        printk("compatible property find failed\r\n");
        return -EINVAL;
    }
    printk("compatible=%s!\r\n", str);
    if(strcmp(str, "atk,beep")){
        return -EINVAL;
    }
    ret = of_property_read_string(gpio_beep.nd, "status", &str);
    if(ret < 0){
        printk("status read failed\r\n");
        return -EINVAL;
    }
    printk("status = %s\r\n", str);
    if(strcmp(str,"okay")){
        return -EINVAL;
    }
    gpio_beep.beep_gpio = of_get_named_gpio(gpio_beep.nd, "beep-gpio", 0);
    if(gpio_beep.beep_gpio < 0){
        printk("can't get beep-gpio");
        return -EINVAL;
    }
    printk("beep-gpio num = %d\r\n", gpio_beep.beep_gpio);
    ret = gpio_request(gpio_beep.beep_gpio, "BEEP-GPIO");
    if(ret){
        printk(KERN_ERR "gpio_beep:failed to request beep-gpio:%d\r\n",ret);
        return ret;
    }
    ret = gpio_direction_output(gpio_beep.beep_gpio, 1);
    if(ret < 0){
        printk("can't set gpio!\r\n");
    }

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
    gpio_free(gpio_beep.beep_gpio);
    return 0;
}


static struct file_operations gpio_beep_fops={
    .owner = THIS_MODULE,
    .open  = beep_open,
    .read  = beep_read,
    .write = beep_write,
    .release = beep_release,
};


static int __init beep_init(void)
{
    int ret = 0;
    if(gpio_beep.major){
        gpio_beep.devid = MKDEV(gpio_beep.major, 0);
        ret = register_chrdev_region(gpio_beep.devid,GPIO_BEEP_CNT, GPIO_BEEP_NAME);
        if(ret < 0){
            pr_err("%s register char dev failed,ret=%d\r\n",GPIO_BEEP_NAME,ret);
            goto free_gpio;
        }
    }else{
        ret = alloc_chrdev_region(&gpio_beep.devid,0,GPIO_BEEP_CNT,GPIO_BEEP_NAME);
        if(ret <0){
            pr_err("%s couldn't alloc_chrdev_region,ret=%d\r\n",GPIO_BEEP_NAME,ret);
            goto free_gpio;
        }
        gpio_beep.major = MAJOR(gpio_beep.devid);
        gpio_beep.minor = MINOR(gpio_beep.devid);
    }
    printk("dts_beep major=%d,minor=%d\r\n",gpio_beep.major,gpio_beep.minor);
    gpio_beep.cdev.owner = THIS_MODULE;
    cdev_init(&gpio_beep.cdev, &gpio_beep_fops);
    ret = cdev_add(&gpio_beep.cdev, gpio_beep.devid, GPIO_BEEP_CNT);
    if(ret < 0){
        goto del_unregister;
    }

    gpio_beep.class = class_create(THIS_MODULE,GPIO_BEEP_NAME);
    if(IS_ERR(gpio_beep.class)){
        goto del_cdev;
    }
    gpio_beep.device = device_create(gpio_beep.class, NULL, gpio_beep.devid, NULL, GPIO_BEEP_NAME);
    if(IS_ERR(gpio_beep.device)){
        goto destroy_class;
    }
    return 0;
    destroy_class:
        class_destroy(gpio_beep.class);
    del_cdev:
        cdev_del(&gpio_beep.cdev);
    del_unregister:
        unregister_chrdev_region(gpio_beep.devid, GPIO_BEEP_CNT);
    free_gpio:
        return -EIO;

}

static void __exit beep_exit(void)
{
    cdev_del(&gpio_beep.cdev);
    unregister_chrdev_region(gpio_beep.devid, GPIO_BEEP_CNT);
    device_destroy(gpio_beep.class, gpio_beep.devid);
    class_destroy(gpio_beep.class);
    printk("beep exit\r\n");
    return;
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
