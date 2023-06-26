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
#include <linux/semaphore.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEY_NAME "input_key"
#define KEY0_VALUE    0xF0
#define INVALID_KEY      0x00

enum key_status{
    KEY_PRESS = 0,
    KEY_RELEASE,
    KEY_KEEP,
};

struct key_dev{
    struct input_dev *idev;
    int key_gpio;
    int key_irq;
    struct timer_list timer;
};

struct key_dev key0_dev;

static irqreturn_t key_interrupt_cb(int irq, void *dev_id)
{
    if(key0_dev.key_irq !=irq){
        return IRQ_NONE;
    }
    disable_irq_nosync(irq);
    mod_timer(&key0_dev.timer, jiffies + msecs_to_jiffies(15));
    return IRQ_HANDLED;
}

static int key_gpio_init(struct device_node *nd)
{
    int ret;
    unsigned long irq_flags;

    key0_dev.key_gpio = of_get_named_gpio(nd, "key-gpio", 0);
    if(gpio_is_valid(key0_dev.key_gpio) == 0){
        printk("can't get key-gpio");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", key0_dev.key_gpio);

    ret = gpio_request(key0_dev.key_gpio, "KEY0");
    if(ret){
        printk(KERN_ERR "key0_dev:failed to request key-gpio:%d\r\n",ret);
        return ret;
    }
    ret = gpio_direction_input(key0_dev.key_gpio);
    if(ret < 0){
        printk("can't set gpio!\r\n");
    }
    
    key0_dev.key_irq = irq_of_parse_and_map(nd,0);
    if(key0_dev.key_irq ==0){
        return -EINVAL;
    }
    irq_flags = irq_get_trigger_type(key0_dev.key_irq);
    if(irq_flags == IRQF_TRIGGER_NONE){
        irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;
    }
    ret = request_irq(key0_dev.key_irq, key_interrupt_cb, irq_flags, "key0_irq", NULL);
    if(ret){
        gpio_free(key0_dev.key_gpio);
        printk("key irq request err:%d!\r\n", ret);
        return ret;
    }
    return 0;
}

static void key_timer_cb(struct timer_list *arg)
{
    int current_val;

    current_val = gpio_get_value(key0_dev.key_gpio);
    input_report_key(key0_dev.idev, KEY_0, !current_val);
    input_sync(key0_dev.idev);
    enable_irq(key0_dev.key_irq);
}


static int key_probe(struct platform_device *pdev)
{
    int ret;

    ret = key_gpio_init(pdev->dev.of_node);
    if(ret < 0){
        return ret;
    }
    timer_setup(&key0_dev.timer, key_timer_cb, 0);
    key0_dev.idev = input_allocate_device();
    key0_dev.idev->name = KEY_NAME;

    key0_dev.idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
    input_set_capability(key0_dev.idev, EV_KEY, KEY_0);

    ret = input_register_device(key0_dev.idev);
    if(ret){
        printk("register input device failed: %d\n", ret);
        free_irq(key0_dev.key_irq, NULL);
        gpio_free(key0_dev.key_gpio);
        del_timer_sync(&key0_dev.timer);
        return -EIO;
    }
    printk("key probe ok\r\n");
    return 0;
}

static int key_remove(struct platform_device *pdev)
{
    free_irq(key0_dev.key_irq, NULL);
    gpio_free(key0_dev.key_gpio);
    del_timer_sync(&key0_dev.timer);
    input_unregister_device(key0_dev.idev);
    return 0;
}

static const struct of_device_id key_of_match[] = {
    {.compatible = "atk,key"},
    {/* sentinel */},
};

static struct platform_driver key_driver = {
    .driver = {
        .name = "stm32mp1-key",
        .of_match_table = key_of_match
    },
    .probe = key_probe,
    .remove = key_remove,
};

module_platform_driver(key_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
