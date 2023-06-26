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

#define GPIO_LED_CNT 1
#define GPIO_LED_NAME "gpio_led"
#define LED_ON    1
#define LED_OFF   0

struct gpio_led_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;
    int dev_state;
    spinlock_t lock;
};

static struct gpio_led_dev gpio_led;


static int led_open(struct inode *inode, struct file *filep)
{
    int ret;
    const char* str;
    unsigned long flags;

    spin_lock_irqsave(&gpio_led.lock, flags);
    if(gpio_led.dev_state){
        spin_unlock_irqrestore(&gpio_led.lock, flags);
        return -EBUSY;
    }
    gpio_led.dev_state++;
    spin_unlock_irqrestore(&gpio_led.lock, flags);

    gpio_led.nd = of_find_node_by_path("/gpio_led");
    if(gpio_led.nd == NULL){
        printk("stm32mp1_led node not find!\r\n");
        return -EINVAL;
    }else{
        printk("stm32mp1_led node find!\r\n");
    }
    ret = of_property_read_string(gpio_led.nd, "compatible", &str);
    if(ret < 0){
        printk("compatible property find failed\r\n");
        return -EINVAL;
    }
    printk("compatible=%s!\r\n", str);
    if(strcmp(str, "atk,led")){
        return -EINVAL;
    }
    ret = of_property_read_string(gpio_led.nd, "status", &str);
    if(ret < 0){
        printk("status read failed\r\n");
        return -EINVAL;
    }
    printk("status = %s\r\n", str);
    if(strcmp(str,"okay")){
        return -EINVAL;
    }
    gpio_led.led_gpio = of_get_named_gpio(gpio_led.nd, "led_gpio", 0);
    if(gpio_led.led_gpio < 0){
        printk("can't get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", gpio_led.led_gpio);
    ret = gpio_request(gpio_led.led_gpio, "LED-GPIO");
    if(ret){
        printk(KERN_ERR "gpio_led:failed to request led-gpio:%d\r\n",ret);
        return ret;
    }
    ret = gpio_direction_output(gpio_led.led_gpio, 1);
    if(ret < 0){
        printk("can't set gpio!\r\n");
    }

    filep->private_data = &gpio_led;
    printk("led open success");
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int ret;
    unsigned char data_buf[1];
    unsigned char led_state;
    struct gpio_led_dev *dev = filp->private_data;

    ret = copy_from_user(data_buf, buf, cnt);
    if(ret < 0){
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    led_state = data_buf[0];
    printk("led state: %d\r\n", led_state);
    if(led_state == LED_ON){
        gpio_set_value(dev->led_gpio, 0);
    }else if(led_state == LED_OFF){
        gpio_set_value(dev->led_gpio, 1);
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    struct gpio_led_dev *dev = filp->private_data;
    unsigned long flags;

    gpio_free(gpio_led.led_gpio);

    spin_lock_irqsave(&dev->lock, flags);
    if(dev->dev_state){
        dev->dev_state --;
    }
    spin_unlock_irqrestore(&dev->lock, flags);
    return 0;
}


static struct file_operations gpio_led_fops={
    .owner = THIS_MODULE,
    .open  = led_open,
    .read  = led_read,
    .write = led_write,
    .release = led_release,
};


static int __init led_init(void)
{
    int ret = 0;

    spin_lock_init(&gpio_led.lock);
    
    if(gpio_led.major){
        gpio_led.devid = MKDEV(gpio_led.major, 0);
        ret = register_chrdev_region(gpio_led.devid,GPIO_LED_CNT, GPIO_LED_NAME);
        if(ret < 0){
            pr_err("%s register char dev failed,ret=%d\r\n",GPIO_LED_NAME,ret);
            goto free_gpio;
        }
    }else{
        ret = alloc_chrdev_region(&gpio_led.devid,0,GPIO_LED_CNT,GPIO_LED_NAME);
        if(ret <0){
            pr_err("%s couldn't alloc_chrdev_region,ret=%d\r\n",GPIO_LED_NAME,ret);
            goto free_gpio;
        }
        gpio_led.major = MAJOR(gpio_led.devid);
        gpio_led.minor = MINOR(gpio_led.devid);
    }
    printk("dts_led major=%d,minor=%d\r\n",gpio_led.major,gpio_led.minor);
    gpio_led.cdev.owner = THIS_MODULE;
    cdev_init(&gpio_led.cdev, &gpio_led_fops);
    ret = cdev_add(&gpio_led.cdev, gpio_led.devid, GPIO_LED_CNT);
    if(ret < 0){
        goto del_unregister;
    }

    gpio_led.class = class_create(THIS_MODULE,GPIO_LED_NAME);
    if(IS_ERR(gpio_led.class)){
        goto del_cdev;
    }
    gpio_led.device = device_create(gpio_led.class, NULL, gpio_led.devid, NULL, GPIO_LED_NAME);
    if(IS_ERR(gpio_led.device)){
        goto destroy_class;
    }
    return 0;
    destroy_class:
        class_destroy(gpio_led.class);
    del_cdev:
        cdev_del(&gpio_led.cdev);
    del_unregister:
        unregister_chrdev_region(gpio_led.devid, GPIO_LED_CNT);
    free_gpio:
        return -EIO;

}

static void __exit led_exit(void)
{
    cdev_del(&gpio_led.cdev);
    unregister_chrdev_region(gpio_led.devid, GPIO_LED_CNT);
    device_destroy(gpio_led.class, gpio_led.devid);
    class_destroy(gpio_led.class);
    printk("led exit\r\n");
    return;
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
