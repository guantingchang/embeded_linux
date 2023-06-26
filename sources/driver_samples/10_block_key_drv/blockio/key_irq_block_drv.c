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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEY_CNT 1
#define KEY_NAME "key"
#define KEY0_VALUE    0xF0
#define INVALID_KEY      0x00

enum key_status{
    KEY_PRESS = 0,
    KEY_RELEASE,
    KEY_KEEP,
};

struct key_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int key_gpio;
    struct timer_list timer;
    int irq_num;
    atomic_t status;
    wait_queue_head_t r_wait;
};

struct key_dev key0_dev;

static irqreturn_t key_interrupt_cb(int irq, void *dev_id)
{
    mod_timer(&key0_dev.timer, jiffies + msecs_to_jiffies(15));
    return IRQ_HANDLED;
}

static int keyio_init(void)
{
    int ret;
    const char* str;
    unsigned long irq_flags;

    key0_dev.nd = of_find_node_by_path("/key");
    if(key0_dev.nd == NULL){
        printk("key node not find!\r\n");
        return -EINVAL;
    }else{
        printk("key node find!\r\n");
    }
    ret = of_property_read_string(key0_dev.nd, "compatible", &str);
    if(ret < 0){
        printk("compatible property find failed\r\n");
        return -EINVAL;
    }
    printk("compatible=%s!\r\n", str);
    if(strcmp(str, "atk,key")){
        return -EINVAL;
    }
    ret = of_property_read_string(key0_dev.nd, "status", &str);
    if(ret < 0){
        printk("status read failed\r\n");
        return -EINVAL;
    }
    printk("status = %s\r\n", str);
    if(strcmp(str,"okay")){
        return -EINVAL;
    }
    key0_dev.key_gpio = of_get_named_gpio(key0_dev.nd, "key-gpio", 0);
    if(key0_dev.key_gpio < 0){
        printk("can't get key-gpio");
        return -EINVAL;
    }
    key0_dev.irq_num = irq_of_parse_and_map(key0_dev.nd, 0);
    if(!key0_dev.irq_num){
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", key0_dev.key_gpio);

    ret = gpio_request(key0_dev.key_gpio, "KEY-GPIO");
    if(ret){
        printk(KERN_ERR "key0_dev:failed to request key-gpio:%d\r\n",ret);
        return ret;
    }
    ret = gpio_direction_input(key0_dev.key_gpio);
    if(ret < 0){
        printk("can't set gpio!\r\n");
    }
    irq_flags = irq_get_trigger_type(key0_dev.irq_num);
    if(irq_flags == IRQF_TRIGGER_NONE){
        irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING;
    }
    ret = request_irq(key0_dev.irq_num, key_interrupt_cb, irq_flags, "key0_irq", NULL);
    if(ret){
        gpio_free(key0_dev.key_gpio);
        return ret;
    }
    return 0;
}

static void key_timer_cb(struct timer_list *arg)
{
    static int last_val = 1;
    int current_val;

    current_val = gpio_get_value(key0_dev.key_gpio);
    if( 0 == current_val && last_val){
        atomic_set(&key0_dev.status, KEY_PRESS);
        wake_up_interruptible(&key0_dev.r_wait);
    }else if(1== current_val && !last_val){
        atomic_set(&key0_dev.status, KEY_RELEASE);
        wake_up_interruptible(&key0_dev.r_wait);
    }else{
        atomic_set(&key0_dev.status, KEY_KEEP);
    }
    last_val = current_val;

}
static int key_open(struct inode *inode, struct file *filep)
{
    int ret;

    filep->private_data = &key0_dev;
    ret = keyio_init();
    if(ret < 0){
        return ret;
    }
    timer_setup(&key0_dev.timer, key_timer_cb, 0);
    printk("key open success");
    return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    int ret = 0;
    struct key_dev *dev = filp->private_data;
    
    ret = wait_event_interruptible(dev->r_wait, KEY_KEEP !=atomic_read(&dev->status));
    if(ret){
        return ret;
    }
    ret = copy_to_user(buf, &dev->status, sizeof(int));
    atomic_set(&dev->status, KEY_KEEP);
    return ret;
}

static ssize_t key_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static int key_release(struct inode *inode, struct file *filp)
{
    struct key_dev *dev = filp->private_data;
    gpio_free(dev->key_gpio);
    free_irq(dev->irq_num, NULL);
    del_timer_sync(&dev->timer);
    printk("close key irq ok");
    return 0;
}


static struct file_operations key_fops={
    .owner = THIS_MODULE,
    .open  = key_open,
    .read  = key_read,
    .write = key_write,
    .release = key_release,
};


static int __init gpio_key_init(void)
{
    int ret = 0;

    init_waitqueue_head(&key0_dev.r_wait);
    atomic_set(&key0_dev.status, KEY_KEEP);
    
    if(key0_dev.major){
        key0_dev.devid = MKDEV(key0_dev.major, 0);
        ret = register_chrdev_region(key0_dev.devid,KEY_CNT, KEY_NAME);
        if(ret < 0){
            pr_err("%s register char dev failed,ret=%d\r\n",KEY_NAME,ret);
            return -EIO;
        }
    }else{
        ret = alloc_chrdev_region(&key0_dev.devid,0,KEY_CNT,KEY_NAME);
        if(ret <0){
            pr_err("%s couldn't alloc_chrdev_region,ret=%d\r\n",KEY_NAME,ret);
            return -EIO;
        }
        key0_dev.major = MAJOR(key0_dev.devid);
        key0_dev.minor = MINOR(key0_dev.devid);
    }
    printk("key major=%d,minor=%d\r\n",key0_dev.major,key0_dev.minor);
    key0_dev.cdev.owner = THIS_MODULE;
    cdev_init(&key0_dev.cdev, &key_fops);
    ret = cdev_add(&key0_dev.cdev, key0_dev.devid, KEY_CNT);
    if(ret < 0){
        goto del_unregister;
    }

    key0_dev.class = class_create(THIS_MODULE,KEY_NAME);
    if(IS_ERR(key0_dev.class)){
        goto del_cdev;
    }
    key0_dev.device = device_create(key0_dev.class, NULL, key0_dev.devid, NULL, KEY_NAME);
    if(IS_ERR(key0_dev.device)){
        goto destroy_class;
    }
    return 0;
    destroy_class:
        device_destroy(key0_dev.class, key0_dev.devid);
    del_cdev:
        cdev_del(&key0_dev.cdev);
    del_unregister:
        unregister_chrdev_region(key0_dev.devid, KEY_CNT);
        return -EIO;

}

static void __exit gpio_key_exit(void)
{
    cdev_del(&key0_dev.cdev);
    unregister_chrdev_region(key0_dev.devid, KEY_CNT);
    device_destroy(key0_dev.class, key0_dev.devid);
    class_destroy(key0_dev.class);
    printk("key exit\r\n");
    return;
}

module_init(gpio_key_init);
module_exit(gpio_key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
