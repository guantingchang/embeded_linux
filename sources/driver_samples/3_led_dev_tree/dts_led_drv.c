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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define DTS_LED_CNT 1
#define DTS_LED_NAME "dts_led"
#define LED_ON    1
#define LED_OFF   0

/*映射后的寄存器虚拟地址指针*/
static void __iomem *mpu_ahb4_periph_rcc_pi;
static void __iomem *gpioi_moder_pi;
static void __iomem *gpioi_otyper_pi;
static void __iomem *gpioi_ospeedr_pi;
static void __iomem *gpioi_pupdr_pi;
static void __iomem *gpioi_bsrr_pi;

struct dts_led_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
};

struct dts_led_dev dts_led;

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
    uint32_t value;
    uint32_t reg_data[12];
    int ret;
    struct property *proper;
    const char* str;

    dts_led.nd = of_find_node_by_path("/stm32mp1_led");
    if(dts_led.nd == NULL){
        printk("stm32mp1_led node not find!\r\n");
        return -EINVAL;
    }else{
        printk("stm32mp1_led node fine!\r\n");
    }
    proper = of_find_property(dts_led.nd, "compatible", NULL);
    if(proper == NULL){
        printk("compatible property find failed\r\n");
    }else{
        printk("compatible=%s!\r\n", (char*)proper->value);
    }
    ret = of_property_read_string(dts_led.nd, "status", &str);
    if(ret < 0){
        printk("status read failed\r\n");
    }else{
        printk("status = %s\r\n", str);
    }
    ret = of_property_read_u32_array(dts_led.nd, "reg", reg_data, 12);
    if(ret < 0){
        printk("reg property read failed!\r\n");
    }else{
        uint8_t i=0;
        printk("reg data:\r\n");
        for(i=0;i<12;i++){
            printk("%x ",reg_data[i]);
        }
        printk("\r\n");
    }

    mpu_ahb4_periph_rcc_pi = of_iomap(dts_led.nd,0);
    gpioi_moder_pi = of_iomap(dts_led.nd,1);
    gpioi_otyper_pi = of_iomap(dts_led.nd,2);
    gpioi_ospeedr_pi = of_iomap(dts_led.nd,3);
    gpioi_pupdr_pi = of_iomap(dts_led.nd,4);
    gpioi_bsrr_pi = of_iomap(dts_led.nd,5);

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
    filep->private_data = &dts_led;
    printk("led open success");
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
    led_unmap();
    return 0;
}


static struct file_operations dts_led_fops={
    .owner = THIS_MODULE,
    .open  = led_open,
    .read  = led_read,
    .write = led_write,
    .release = led_release,
};


static int __init led_init(void)
{
    int ret = 0;
    if(dts_led.major){
        dts_led.devid = MKDEV(dts_led.major, 0);
        ret = register_chrdev_region(dts_led.devid,DTS_LED_CNT, DTS_LED_NAME);
        if(ret < 0){
            pr_err("%s register char dev failed,ret=%d\r\n",DTS_LED_NAME,ret);
            goto fail_map;
        }
    }else{
        ret = alloc_chrdev_region(&dts_led.devid,0,DTS_LED_CNT,DTS_LED_NAME);
        if(ret <0){
            pr_err("%s couldn't alloc_chrdev_region,ret=%d\r\n",DTS_LED_NAME,ret);
            goto fail_map;
        }
        dts_led.major = MAJOR(dts_led.devid);
        dts_led.minor = MINOR(dts_led.devid);
    }
    printk("dts_led major=%d,minor=%d\r\n",dts_led.major,dts_led.minor);
    dts_led.cdev.owner = THIS_MODULE;
    cdev_init(&dts_led.cdev, &dts_led_fops);
    ret = cdev_add(&dts_led.cdev, dts_led.devid, DTS_LED_CNT);
    if(ret < 0){
        goto del_unregister;
    }

    dts_led.class = class_create(THIS_MODULE,DTS_LED_NAME);
    if(IS_ERR(dts_led.class)){
        goto del_cdev;
    }
    dts_led.device = device_create(dts_led.class, NULL, dts_led.devid, NULL, DTS_LED_NAME);
    if(IS_ERR(dts_led.device)){
        goto destroy_class;
    }
    return 0;
    destroy_class:
        class_destroy(dts_led.class);
    del_cdev:
        cdev_del(&dts_led.cdev);
    del_unregister:
        unregister_chrdev_region(dts_led.devid, DTS_LED_CNT);
    fail_map:
        return -EIO;

}

static void __exit led_exit(void)
{
    cdev_del(&dts_led.cdev);
    unregister_chrdev_region(dts_led.devid, DTS_LED_CNT);
    device_destroy(dts_led.class, dts_led.devid);
    class_destroy(dts_led.class);
    printk("led exit\r\n");
    return;
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
