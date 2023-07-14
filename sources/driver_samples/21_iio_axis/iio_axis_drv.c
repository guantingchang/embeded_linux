#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/platform_device.h>

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
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/spi/spi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "icm20608_drv.h"

#define ICM20608_CNT  1
#define ICM20608_NAME "icm20608"

#define ICM20608_CHAN(_type, _channel2, _index) \
{
	.type = _type,
	.modified = 1,
	.channel2 = _channel2,
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_RAW),
	.scan_index = _index,
	.scan_type = {
		.sign = 's',
		.realbits = 16,
		.storagebits = 16,
		.shift = 0,
		.endianness = IIO_BE,
	},
}

enum inv_icm20608_scan {
	INV_ICM20608_SCAN_ACCL_X,
	INV_ICM20608_SCAN_ACCL_Y,
	INV_ICM20608_SCAN_ACCL_Z,
	INV_ICM20608_SCAN_TEMP,
	INV_ICM20608_SCAN_GYRO_X,
	INV_ICM20608_SCAN_GYRO_Y,
	INV_ICM20608_SCAN_GYRO_Z,
	INV_ICM20608_SCAN_TIMESTAMP,
};


struct icm20608_dev {
	struct spi_device *spi;
	struct regmap *regmap;
	struct regmap_config regmap_config;
	struct mutex lock;
};

static const struct iio_chan_spec icm20608_channels[] ={
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_OFFSET) | BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = INV_ICM20608_SCAN_TEMP,
		.scan_type = {
			.sign = 's',
			.realbits = 16,
			.storagebits = 16,
			.shift = 0,
			.endianness = IIO_BE,
		},
	},

	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_ICM20608_SCAN_GYRO_X),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_ICM20608_SCAN_GYRO_Y),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_ICM20608_SCAN_GYRO_Z),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_ICM20608_SCAN_ACCL_X),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_ICM20608_SCAN_ACCL_Y),
	ICM20608_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_ICM20608_SCAN_ACCL_Z),
};

//实测有功能bug
static int icm20608_read_regs(struct icm20608_dev *dev, u8 reg, void *buf, int len) 
{
	int ret = -1;
	unsigned char txdata[1];
	unsigned char *rxdata;
	struct spi_message msg;
	struct spi_transfer *transfer;
	struct spi_device *spi = (struct spi_device *)dev->spi;

	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if(transfer == NULL){
		return -ENOMEM;
	}
	rxdata = kzalloc(sizeof(char)* len, GFP_KERNEL);
	if(rxdata == NULL){
		goto out1;
	}

	txdata[0] = reg | 0x80;
	transfer->tx_buf = txdata;
	transfer->rx_buf = rxdata;
	transfer->len = len+1;
	spi_message_init(&msg);
	spi_message_add_tail(transfer, &msg);
	ret = spi_sync(spi, &msg);
	if(ret){
		goto out2;
	}
	memcpy(buf,rxdata+1, len);
out2:
	kfree(rxdata);
out1:
	kfree(transfer);
	return ret;
}

static s32 icm20608_write_regs(struct icm20608_dev *dev, u8 reg, u8 *buf, u8 len) 
{
	int ret = -1;
	unsigned char *txdata;
	struct spi_message msg;
	struct spi_transfer *transfer;
	struct spi_device *spi = (struct spi_device *)dev->spi;
	transfer = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if(transfer == NULL){
		return -ENOMEM;
	}

	txdata = kzalloc(sizeof(char)+len, GFP_KERNEL);
	if(txdata == NULL){
		goto out1;
	}

	*txdata = reg & ~0x80;
	memcpy(txdata+1,buf,len);
	transfer->tx_buf = txdata;
	transfer ->len = len +1;
	spi_message_init(&msg);
	spi_message_add_tail(transfer, &msg);
	ret = spi_sync(spi, &msg);
	if(ret!=0){
		goto out2;
	}
out2:
	kfree(txdata);
out1:
	kfree(transfer);
	return ret;	
}

static unsigned char icm20608_read_one_reg(struct icm20608_dev *dev, u8 reg)
{
	u8 ret = 0;
	unsigned int data;

	ret = regmap_read(dev->regmap, reg, &data);
	return (u8)data;
}

static void icm20608_write_one_reg(struct icm20608_dev *dev,u8 reg, u8 value)
{
	regmap_write(dev->regmap, reg, value);
}

void icm20608_readdata(struct icm20608_dev *dev)
{
	unsigned char data[14];

	//icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);
	u8 ret;
	ret = regmap_bulk_read(dev->regmap, ICM20_ACCEL_XOUT_H, data, 14);
	dev->accel_x_adc = (signed short)((data[0] << 8) | (data[1]));
	dev->accel_y_adc = (signed short)((data[2]<<8) | (data[3]));
	dev->accel_z_adc = (signed short)((data[4]<<8) | (data[5]));
	dev->temp_adc = (signed short)((data[6]<<8) | (data[7]));
	dev->gyro_x_adc = (signed short)((data[8]<< 8) |(data[9]));
	dev->gyro_y_adc = (signed short)((data[10]<< 8)| (data[11]));
	dev->gyro_z_adc = (signed short)((data[12]<<8)| data[13]);	
}

void icm20608_reginit(struct icm20608_dev *dev)
{
	u8 value = 0;

	icm20608_write_one_reg(dev, ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	icm20608_write_one_reg(dev,ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);
	value = icm20608_read_one_reg(dev, ICM20_WHO_AM_I);
	printk("icm20608 id=%x\r\n", value);
	icm20608_write_one_reg(dev, ICM20_SMPLRT_DIV, 0x00);
    icm20608_write_one_reg(dev, ICM20_GYRO_CONFIG, 0x18);
    icm20608_write_one_reg(dev, ICM20_ACCEL_CONFIG, 0x18);
    icm20608_write_one_reg(dev, ICM20_CONFIG, 0x04);
    icm20608_write_one_reg(dev, ICM20_ACCEL_CONFIG2, 0x04);
    icm20608_write_one_reg(dev, ICM20_PWR_MGMT_2, 0x00);
    icm20608_write_one_reg(dev, ICM20_LP_MODE_CFG, 0x00);
    icm20608_write_one_reg(dev, ICM20_FIFO_EN, 0x00);
}

#define ICM20608_TEMP_OFFSET  0
#define ICM20608_TEMP_SCALE   326800000

static const int gyro_scale_icm20608[] = {7629,15258,30517, 61035};
static const int accel_scale_icm20608 = {61035,122070, 244140,488281};

static int icm20608_sensor_show(struct icm20608_dev *dev, int reg, int axis, int *value)
{
	int ind, result;
	__bel6 data;

	ind = (axis -IIO_MOD_X) *2;
	result = regmap_bulk_read(dev->regmap, reg + ind, (u8*)&data, 2);
	if(result != 0){
		return -EINVAL;
	}
	*val = (short)be16_to_cpup(&data);
	return IIO_VAL_INT;
}

static int icm20608_read_channel_data(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val)
{
	struct icm20608_dev *dev = iio_priv(indio_dev);
	int ret = 0;

	switch(chan->type){
		case IIO_ANGL_VEL:
			ret = icm20608_sensor_show(dev, ICM20_GYRO_XOUT_H, chan->channel2, val);
			break;
		case IIO_ACCEL:
			ret = icm20608_sensor_show(dev, ICM20_ACCEL_XOUT_H, chan->channel2, val);
			break;
		case IIO_TEMP:
			ret = icm20608_sensor_show(dev, ICM20_TEMP_OUT_H, IIO_MOD_X, val);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int icm20608_read_raw(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask)
{
	struct icm20608_dev *dev = iio_priv(indio_dev);
	int ret = 0;
	unsigned char regdata = 0;
	printk("icm20608_read_raw\r\n");

	if(mask == IIO_CHAN_INFO_RAW){			
			printk("read raw data\r\n");
			iio_device_claim_direct_mode(indio_dev);
			mutex_lock(&dev->lock);
			ret = icm20608_read_channel_data(indio_dev, chan, val);
			mutex_unlock(&dev->lock);
			iio_device_release_direct_mode(indio_dev);
			return ret;
	}else if(mask == IIO_CHAN_INFO_SCALE){
		switch(chan->type){
			case IIO_ANGL_VEL:
				printk("read gyro scale\r\n");
				mutex_lock(&dev->lock);
				regdata = (icm20608_read_onereg(dev, ICM20_GYRO_CONFIG) & 0x18) >>3 ;
				*val = 0;
				*val2 = gyro_scale_icm20608[regdata];
				mutex_unlock(&dev->lock);
				return IIO_VAL_INT_PLUS_MICRO;
			case IIO_ACCEL:
				printk("read acc\r\n");
				mutex_lock(&dev->lock);
				regdata = (icm20608_read_onereg(dev, ICM20_ACCEL_CONFIG) & 0x18) >>3;
				*val = 0;
				*val2 = accel_scale_icm20608[regdata];
				mutex_unlock(&dev->lock);
				return IIO_VAL_INT_PLUS_NANO;
			case IIO_TEMP:
				printk("read temp\r\n");
				*val = ICM20608_TEMP_SCALE / 1000000;
				*val2 = ICM20608_TEMP_SCALE % 1000000;
				return IIO_VAL_INT_PLUS_MICRO;
			default:
				return -EINVAL;
		}
	}else if(mask == IIO_CHAN_INFO_OFFSET) {
		switch(chan->type) {
			case IIO_TEMP:
				printk("read temp offset\r\n");
				*val = ICM20608_TEMP_OFFSET;
				return IIO_VAL_INT;
			default:
				return -EINVAL;
		}
	}else if(mask == IIO_CHAN_INFO_CALIBBIAS){
		switch(chan->type){
			case IIO_ANGL_VEL:
				printk("read gyro calibbias\r\n");
				mutex_lock(&dev->lock);
				ret = icm20608_sensor_show(dev, ICM20_XG_OFFS_USRH, chan->channel2,val);
				mutex_unlock(&dev->lock);
				return ret;
			case IIO_ACCEL:
				printk("read accl calibbias\r\n");
				mutex_lock(&dev->lock);
				ret = icm20608_sensor_show(dev, ICM20_XA_OFFSET_H, chan->channel2, val);
				mutex_unlock(&dev->lock);
				return ret;
			default:
				return -EINVAL;
		}
	}else{
		return -EINVAL;
	}
}

static int icm20608_sensor_set(struct icm20608_dev *dev, int reg, int axis, int val)
{
	int ind,result;
	__be16 data = cpu_to_be16(val);

	ind = (axis - IIO_MOD_X) *2;
	result = regmap_bulk_write(dev->regmap, reg + ind, (u8*)&data, 2);
	if(result != 0){
		return -EINVAL;
	}
	return 0;
}

static int icm20608_write_gyro_scale(struct icm20608_dev *dev, int val)
{
	int result, i;
	u8 data;

	for(i=0;i< ARRAY_SIZE(gyro_scale_icm20608); ++i){
		if(gyro_scale_icm20608[i] == val){
			data = (i <<3);
			result = regmap_write(dev->regmap, ICM20_CYRO_CONFIG, data);
			if(result){
				return result;
			}
			return 0;
		}
	}
	return -EINVAL;
}

static int icm20608_write_accel_scale(struct icm20608_dev *dev, int val)
{
	int result, i;
	u8 data;

	for(i=0;i< ARRAY_SIZE(accel_scale_icm20608); ++i){
		if(accel_scale_icm20608[i] == val){
			data = (i <<3);
			result = regmap_write(dev->regmap, ICM20_ACCEL_CONFIG, data);
			if(result){
				return result;
			}
			return 0;
		}
	}
	return -EINVAL;
}

static int icm20608_write_raw(struct iio_dev* indio_dev, struct iio_chan_spec const *chan, int val, int val2, long mask)
{
	printk("icm20608_write_raw\r\n");
	struct icm20608_dev *dev = iio_priv(indio_dev);
	int ret = 0;

	iio_device_claim_direct_mode(indio_dev); 
	switch (mask) {
		case IIO_CHAN_INFO_SCALE: /* 设置陀螺仪和加速度计的分辨率 */
			switch (chan->type) {
				case IIO_ANGL_VEL: /* 设置陀螺仪 */
					mutex_lock(&dev->lock);
					ret = icm20608_write_gyro_scale(dev, val2);
					mutex_unlock(&dev->lock);
					break;
				case IIO_ACCEL: /* 设置加速度计 */
					mutex_lock(&dev->lock);
					ret = icm20608_write_accel_scale(dev, val2);
					mutex_unlock(&dev->lock);
					break;
				default:
					ret = -EINVAL;
					break;
			}
			break;
		case IIO_CHAN_INFO_CALIBBIAS: /* 设置陀螺仪和加速度计的校准值 */
			switch (chan->type) {
				case IIO_ANGL_VEL: /* 设置陀螺仪校准值 */
					mutex_lock(&dev->lock);
					ret = icm20608_sensor_set(dev, ICM20_XG_OFFS_USRH,
					chan->channel2, val);
					mutex_unlock(&dev->lock);
					break;
				case IIO_ACCEL: /* 加速度计校准值 */
					mutex_lock(&dev->lock);
					ret = icm20608_sensor_set(dev, ICM20_XA_OFFSET_H,
					chan->channel2, val);
					mutex_unlock(&dev->lock);
					break;
				default:
					ret = -EINVAL;
					break;
				}
			break;
		default:
			ret = -EINVAL;
			break;
	}
	iio_device_release_direct_mode(indio_dev);
	return ret;
}

static int icm20608_write_raw_get_fmt(struct iio_dev* indio_dev, struct iio_chan_spec const *chan, long mask)
{
	printk("icm20608_write_raw_get_fmt\r\n");
	switch (mask) {
		case IIO_CHAN_INFO_SCALE:
			switch (chan->type) {
				case IIO_ANGL_VEL: 
					return IIO_VAL_INT_PLUS_MICRO;
				default: 
					return IIO_VAL_INT_PLUS_NANO;
			}
		default:
			return IIO_VAL_INT_PLUS_MICRO;
	}
	return -EINVAL;

	return 0;
}

static const struct iio_info icm20608_info = {
	.read_raw = icm20608_read_raw,
	.write_raw = icm20608_write_raw,
	.write_raw_get_fmt = &icm20608_write_raw_get_fmt,
};


static int icm20608_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t icm20608_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	signed int data[7];
	long err = 0;
	struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
	struct icm20608_dev *dev = container_of(cdev, struct icm20608_dev, cdev);

	icm20608_readdata(dev);
	data[0] = dev->gyro_x_adc;
	data[1] = dev->gyro_y_adc;
	data[2] = dev->gyro_z_adc;
	data[3] = dev->accel_x_adc;
	data[4] = dev->accel_y_adc;
	data[5] = dev->accel_z_adc;
	data[6] = dev->temp_adc;
	err = copy_to_user(buf, data, sizeof(data));
	return 0;
}

static int icm20608_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations imc20608_ops = {
	.owner = THIS_MODULE,
	.open = icm20608_open,
	.read = icm20608_read,
	.release = icm20608_release,
};

static int icm20608_probe(struct spi_device *spi)
{
	int ret;
	struct icm20608_dev *data;
	struct iio_dev *indio_dev;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*data));
	if(!indio_dev){
		return -ENOMEM;
	}

	data = iio_priv(iio_dev);
	data->spi = spi;
	spi_set_drvdata(spi, indio_dev);
	mutex_init(&data->lock);

	indio_dev->dev.parent = &spi>dev;
	indio_dev->info = &icm20608_info;
	indio_dev->name = ICM20608_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = icm20608_channels;
	indio_dev->num_channels = ARRAY_SIZE(icm20608_channels);
	ret = iio_device_register(indio_dev);
	if(ret < 0){
		dev_err(&spi->dev, "iio_device_register failed");
		goto err_iio_register;
	}

	data->regmap_config.reg_bits = 8;
	data->regmap_config.val_bits = 8;
	data->regmap_config.read_flag_mask = 0x80;
	data->regmap = regmap_init_spi(spi, &data->regmap_config);
	if(IS_ERR(data->regmap)){
		ret = PTR_ERR(data->regmap);
		goto err_regmap_init;
	}
	spi->mode = SPI_MODE_0;
	spi_setup(spi);
	icm20608_reginit(data);

	return 0;
err_regmap_init:
	iio_device_unregister(indio_dev);
err_iio_register;
	return ret;
}

static int icm20608_remove(struct spi_device *spi)
{
	struct iio_dev *indio_device = spi_get_drvdata(spi);
	struct icm20608_dev *data;

	data = iio_priv(indio_device);
	regmap_exit(data->regmap);
	iio_device_unregister(indio_device);
	return 0;
}

static const struct spi_device_id icm20608_id[] = {
	{"alientek,icm20608", 0},  
	{}
};

static const struct of_device_id icm20608_of_match[]={
	{.compatible = "alientek,icm20608"},
	{/*sentinel */},
};

static struct spi_driver icm20608_driver = {
	.probe = icm20608_probe,
	.remove = icm20608_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "icm20608",
		.of_match_table = icm20608_of_match,
	},
	.id_table = icm20608_id,
};

static int __init icm20608_init(void)
{
	int ret = 0;
	ret = spi_register_driver(&icm20608_driver);
	return ret;
}

static void __exit icm20608_exit(void)
{
	spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("GTC");
MODULE_INFO(intree, "Y");