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
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_CNT 1
#define LED_NAME  "dts_plat_led"
#define LED_ON    1
#define LED_OFF   0

struct led_dev{
    dev_t dev_id;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int gpio_led;
};
struct led_dev led_device;

void led_switch(uint8_t state)
{
    if(state ==LED_ON){
        gpio_set_value(led_device.gpio_led, 0);
    }else if(state ==LED_OFF){
        gpio_set_value(led_device.gpio_led, 1);
    }
}


static int led_gpio_init(struct device_node *nd)
{
    int ret;

    led_device.gpio_led = of_get_named_gpio(nd, "led_gpio", 0);
    if(led_device.gpio_led < 0){
        printk(KERN_ERR "can't get led-gpio");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", led_device.gpio_led);
    ret = gpio_request(led_device.gpio_led, "LED0");
    if(ret){
        printk(KERN_ERR "gpio_led:failed to request led-gpio:%d\r\n",ret);
        return ret;
    }
    ret = gpio_direction_output(led_device.gpio_led, 1);
    if(ret < 0){
        printk("can't set gpio!\r\n");
    }
    return 0;
}

static int led_open(struct inode *inode, struct file *filep)
{
    printk("led drv open success");
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int ret_value;
    unsigned char data_buf[1];
    unsigned char led_state;

    ret_value = copy_from_user(data_buf, buf, cnt);
    if(ret_value < 0){
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }
    led_state = data_buf[0];
    printk("led state: %d\r\n", led_state);
    if(led_state == LED_ON){
        led_switch(LED_ON);
    }else if(led_state == LED_OFF){
        led_switch(LED_OFF);
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{

    printk("led drv release");
    return 0;
}


static struct file_operations led_fops={
    .owner = THIS_MODULE,
    .open  = led_open,
    .read  = led_read,
    .write = led_write,
    .release = led_release,
};

static int led_probe(struct platform_device *dev)
{
    int ret;

    printk("led driver and device has matched!\r\n");

    ret = led_gpio_init(dev->dev.of_node);
    if(ret < 0){
        return ret;
    }

    ret = alloc_chrdev_region(&led_device.dev_id, 0, LED_CNT, LED_NAME);
    if(ret < 0){
        goto fail_map;        
    }
    led_device.cdev.owner = THIS_MODULE;
    cdev_init(&led_device.cdev, &led_fops);
    ret = cdev_add(&led_device.cdev, led_device.dev_id, LED_CNT);
    if(ret < 0){
        goto del_unregister;
    }
    led_device.class = class_create(THIS_MODULE, LED_NAME);
    if(IS_ERR(led_device.class)){
        goto del_cdev;
    }
    led_device.device = device_create(led_device.class, NULL, led_device.dev_id, NULL, LED_NAME);
    if(IS_ERR(led_device.device)){
        goto destroy_class;
    }
    printk("led init ok\r\n");
    return 0;
destroy_class:
    class_destroy(led_device.class);
del_cdev:
    cdev_del(&led_device.cdev);
del_unregister:
    unregister_chrdev_region(led_device.dev_id, LED_CNT);
fail_map:
    gpio_free(led_device.gpio_led);
    return -EIO;

}

static int led_remove(struct platform_device *dev)
{
    gpio_set_value(led_device.gpio_led, 1);
    gpio_free(led_device.gpio_led);

    cdev_del(&led_device.cdev);
    unregister_chrdev_region(led_device.dev_id, LED_CNT);
    device_destroy(led_device.class, led_device.dev_id);
    class_destroy(led_device.class);
    printk("led exit\r\n");
    return 0;

}

static const struct of_device_id led_of_match[] ={
    {.compatible = "atk,led"},
    {/* sentinel */},
};

MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_driver = {
    .driver = {
        .name = "stm32mp1-led",    
        .of_match_table = led_of_match,    
    },
    .probe = led_probe,
    .remove = led_remove,
};

static int __init led_drv_init(void)
{
    return platform_driver_register(&led_driver);
}

static void __exit led_drv_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
