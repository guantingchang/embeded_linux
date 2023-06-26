#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_MAJOR 200
#define LED_NAME  "led"
#define LED_ON    1
#define LED_OFF   0

/*寄存器物理地址*/
#define PERIPH_BASE    (0x40000000)
#define MPU_AHB4_PERIPH_BASE (PERIPH_BASE+0x10000000)
#define RCC_BASE     (MPU_AHB4_PERIPH_BASE+0x0000)
#define RCC_MP_AHB4ENSETR   (RCC_BASE+0x0A28)
#define GPIOI_BASE     (MPU_AHB4_PERIPH_BASE+0xA000)
#define GPIOI_MODER    (GPIOI_BASE+0x0000)
#define GPIOI_OTYPER   (GPIOI_BASE+0x0004)
#define GPIOI_OSPEEDR  (GPIOI_BASE+0x0008)
#define GPIOI_PUPDR    (GPIOI_BASE+0x000C)
#define GPIOI_BSRR     (GPIOI_BASE+0x0018)

/*映射后的寄存器虚拟地址指针*/
static void __iomem *mpu_ahb4_periph_rcc_pi;
static void __iomem *gpioi_moder_pi;
static void __iomem *gpioi_otyper_pi;
static void __iomem *gpioi_ospeedr_pi;
static void __iomem *gpioi_pupdr_pi;
static void __iomem *gpioi_bsrr_pi;


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

    mpu_ahb4_periph_rcc_pi = ioremap(RCC_MP_AHB4ENSETR,4);
    gpioi_moder_pi = ioremap(GPIOI_MODER,4);
    gpioi_otyper_pi = ioremap(GPIOI_OTYPER,4);
    gpioi_ospeedr_pi = ioremap(GPIOI_OSPEEDR,4);
    gpioi_pupdr_pi = ioremap(GPIOI_PUPDR,4);
    gpioi_bsrr_pi = ioremap(GPIOI_BSRR,4);

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
    printk("led unmap");
    return 0;
}


static struct file_operations led_fops={
    .owner = THIS_MODULE,
    .open  = led_open,
    .read  = led_read,
    .write = led_write,
    .release = led_release,
};


static int __init led_init(void)
{
    int ret_value = 0;
    ret_value = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    if (ret_value < 0) {
        printk("led driver register failed\r\n");
        return -EIO;
    }else{
        printk("led init ok\r\n");
        return 0;
    }

}

static void __exit led_exit(void)
{
    unregister_chrdev(LED_MAJOR, LED_NAME);
    printk("led exit\r\n");
    return;
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guantc");
MODULE_INFO(intree, "Y");
