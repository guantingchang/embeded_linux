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
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216c_drv.h"

#define AP_3216C_CNT  1
#define AP_3216C_NAME "ap3216c"

struct ap_3216c_dev {
	struct i2c_client *client;
	dev_t dev_id;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
    unsigned short ir,als,ps;
};
//实测有功能bug
static int ap3216c_read_regs(struct ap_3216c_dev *dev, u8 reg, void *val, int len) 
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->client;

	msg[0].addr = client ->addr;
	msg[0].flags = 0;
	msg[0].buf = &reg;
	msg[0].len = 1;

	msg[1].addr = client ->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = val;
	msg[1].len = len;

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret ==2){
		ret = 0;
	}else{
		printk("i2c rd failed=%d, reg=%06x len=%d\n", ret, reg, len);
		ret = -EREMOTEIO;
	}

	return ret;
}

static s32 ap3216c_write_regs(struct ap_3216c_dev *dev, u8 reg, u8 *buf, u8 len) 
{
	u8 temp[256];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->client;

	temp[0] = reg;
	memcpy(&temp[1], buf, len);

	msg.addr = client ->addr;
	msg.flags = 0;
	msg.buf = temp;
	msg.len = len +1;

	return  i2c_transfer(client->adapter, &msg, 1);
}

static unsigned char ap3216c_read_reg(struct ap_3216c_dev *dev, u8 reg)
{
	u8 data = 0;
	ap3216c_read_regs(dev,reg, &data,1);
	return data;
}

static void ap3216c_write_reg(struct ap_3216c_dev *dev,u8 reg, u8 data)
{
	u8 buf = 0;
	buf = data;
	ap3216c_write_regs(dev,reg,&buf ,1);
}

void ap3216c_readdata(struct ap_3216c_dev *dev)
{
	unsigned char i = 0;
	unsigned char buf[6];

	for(i = 0; i <6;i++){
		buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW +1);	
	}

	if(buf[0] & 0x80){
		dev ->ir = 0;
	}else{
		dev->ir = ((unsigned short)buf[1] <<2) |(buf[0]&0x03);
	}
	dev->als = ((unsigned short)buf[3] <<8) |(buf[2]);
	if(buf[4] & 0x40){
		dev ->ps = 0;
	}else{
		dev->ps = ((unsigned short)(buf[5]&0x3f) <<4) |(buf[4] & 0X0F);
	}
}

static int ap3216c_open(struct inode *inode, struct file *filp)
{
	struct cdev *cdev = filp ->f_path.dentry->d_inode->i_cdev;
	struct ap_3216c_dev *ap3216cdev = container_of(cdev, struct ap_3216c_dev, cdev);

	ap3216c_write_reg(ap3216cdev, AP3216C_SYSTEMCONG, 0x04); 
	mdelay(50);
	ap3216c_write_reg(ap3216cdev, AP3216C_SYSTEMCONG, 0x03); 
	return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	short data[3];
	long err = 0;

	struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;

	struct ap_3216c_dev *dev = container_of(cdev, struct ap_3216c_dev, cdev);
	ap3216c_readdata(dev);
	data[0] = dev->ir;
	data[1] = dev->als;
	data[2] = dev->ps;

	err = copy_to_user(buf, data, sizeof(data));
	return 0;
}

static int ap3216c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations ap3216c_ops = {
	.owner = THIS_MODULE,
	.open = ap3216c_open,
	.read = ap3216c_read,
	.release = ap3216c_release,
};

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct ap_3216c_dev *ap3216c_dev;

	ap3216c_dev = devm_kzalloc(&client->dev, sizeof(*ap3216c_dev), GFP_KERNEL);
	if(!ap3216c_dev){
		return -ENOMEM;
	}
	ret = alloc_chrdev_region(&ap3216c_dev->dev_id, 0, AP_3216C_CNT, AP_3216C_NAME);
	if(ret < 0){
		printk("%s could't alloc chrdev_region, ret=%d\r\n", AP_3216C_NAME,ret);
		return -ENOMEM;
	}
	ap3216c_dev ->cdev.owner = THIS_MODULE;
	cdev_init(&ap3216c_dev->cdev, &ap3216c_ops);
	ret = cdev_add(&ap3216c_dev->cdev, ap3216c_dev->dev_id, AP_3216C_CNT);
	if(ret < 0){
		goto del_unregister;
	}
	ap3216c_dev->class = class_create(THIS_MODULE, AP_3216C_NAME);
	if(IS_ERR(ap3216c_dev->class)){
		goto del_cdev;
	}
	ap3216c_dev->device = device_create(ap3216c_dev->class, NULL, ap3216c_dev->dev_id, NULL, AP_3216C_NAME);
	if(IS_ERR(ap3216c_dev->device)){
		goto destroy_class;
	}
	ap3216c_dev->client = client;
	i2c_set_clientdata(client, ap3216c_dev);
	return 0;
destroy_class:
	device_destroy(ap3216c_dev->class, ap3216c_dev->dev_id);
del_cdev:
	cdev_del(&ap3216c_dev->cdev);
del_unregister:
	unregister_chrdev_region(ap3216c_dev->dev_id, AP_3216C_CNT);
	return -EIO;
}

static int ap3216c_remove(struct i2c_client *client)
{
	struct ap_3216c_dev *ap3216c_dev = i2c_get_clientdata(client);

	cdev_del(&ap3216c_dev->cdev);
	unregister_chrdev_region(ap3216c_dev->dev_id, AP_3216C_CNT);
	device_destroy(ap3216c_dev->class, ap3216c_dev->dev_id);
	class_destroy(ap3216c_dev->class);
	return 0;
}

static const struct i2c_device_id ap3216c_id[] = {
	{"LiteOn,ap3216c", 0},  
	{}
};

static const struct of_device_id ap3216c_of_match[]={
	{.compatible = "LiteOn,ap3216c"},
	{/*sentinel */},
};

static struct i2c_driver ap3216c_driver = {
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ap3216c",
		.of_match_table = ap3216c_of_match,
	},
	.id_table = ap3216c_id,
};

static int __init ap3216c_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&ap3216c_driver);
	return ret;
}

static void __exit ap3216c_exit(void)
{
	i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("GTC");
MODULE_INFO(intree, "Y");