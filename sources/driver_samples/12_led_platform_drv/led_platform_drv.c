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
#define LED_NAME  "plat_led"
#define LED_ON    1
#define LED_OFF   0

/*映射后的寄存器虚拟地址指针*/
static void __iomem *mpu_ahb4_periph_rcc_pi;
static void __iomem *gpioi_moder_pi;
static void __iomem *gpioi_otyper_pi;
static void __iomem *gpioi_ospeedr_pi;
static void __iomem *gpioi_pupdr_pi;
static void __iomem *gpioi_bsrr_pi;

struct led_dev{
    dev_t dev_id;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};
struct led_dev led_device;

void led_switch(uint8_t state)
{
    uint32_t value = 0;

    if(state ==LED_ON){
        value = readl(gpioi_bsrr_pi);
        value |= (1 << 16);
        writel(value,gpioi_bsrr_pi);
    }else if(state ==LED_OFF){
        value = readl(gpioi_bsrr_pi);
        value |= (1 << 0);
        writel(value,gpioi_bsrr_pi);
    }
}

void led_unmap(void){
    iounmap(mpu_ahb4_periph_rcc_pi);
    iounmap(gpioi_moder_pi);
    iounmap(gpioi_otyper_pi);
    iounmap(gpioi_ospeedr_pi);
    iounmap(gpioi_pupdr_pi);
    iounmap(gpioi_bsrr_pi);
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
    int i = 0,ret;
    int ressize[6];
    u32 value = 0;
    struct resource *led_source[6];

    printk("led driver and device has matched!\r\n");

    for(i=0;i<6;i++){
        led_source[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
        if(led_source[i] == 0){
            dev_err(&dev->dev, "No mem resource for always on\n");
            return -ENXIO;
        }
        ressize[i] = resource_size(led_source[i]);
    }

    mpu_ahb4_periph_rcc_pi = ioremap(led_source[0]->start,ressize[0]);
    gpioi_moder_pi = ioremap(led_source[1]->start,ressize[1]);
    gpioi_otyper_pi = ioremap(led_source[2]->start,ressize[2]);
    gpioi_ospeedr_pi = ioremap(led_source[3]->start,ressize[3]);
    gpioi_pupdr_pi = ioremap(led_source[4]->start,ressize[4]);
    gpioi_bsrr_pi = ioremap(led_source[5]->start,ressize[5]);

    value = readl(mpu_ahb4_periph_rcc_pi);
    value &=~(0x01 << 8);
    value |= (0x01 << 8);
    writel(value, mpu_ahb4_periph_rcc_pi);
 
    value = readl(gpioi_moder_pi);
    value &=~(0x03 << 0);
    value |= (0x01 << 0);
    writel(value, gpioi_moder_pi);

    value = readl(gpioi_otyper_pi);
    value &=~(0x01 << 0);
    writel(value, gpioi_otyper_pi);

    value = readl(gpioi_ospeedr_pi);
    value &=~(0x03 << 0);
    value |= (0x02 << 0);
    writel(value, gpioi_ospeedr_pi);

    value = readl(gpioi_pupdr_pi);
    value &=~(0x03 << 0);
    value |= (0x01 << 0);
    writel(value, gpioi_pupdr_pi);

    value = readl(gpioi_bsrr_pi);
    value |= (0x01 << 0);
    writel(value, gpioi_bsrr_pi);

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
    led_unmap();
    return -EIO;

}

static int led_remove(struct platform_device *dev)
{
    led_unmap();
    cdev_del(&led_device.cdev);
    unregister_chrdev_region(led_device.dev_id, LED_CNT);
    device_destroy(led_device.class, led_device.dev_id);
    class_destroy(led_device.class);
    printk("led exit\r\n");
    return 0;

}

static struct platform_driver led_driver = {
    .driver = {
        .name = "stm32mp1-led",        
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
