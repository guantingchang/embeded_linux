#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#define FT5426_DEVICE_MODE_REG 0x00
#define FT5426_TD_STATUS_REG   0x02
#define FT5426_TOUCH_DATA_REG  0x03
#define FT5426_ID_G_MODE_REG   0xA4

#define MAX_SUPPORT_POINTS   5
#define TOUCH_EVENT_DOWN     0x00
#define TOUCH_EVENT_UP       0x01
#define TOUCH_EVENT_ON       0x02
#define TOUCH_EVENT_RESERRVED 0x03

struct edt_ft5426_dev {
	struct i2c_client *client;
	struct input_dev *input;
	int reset_gpio;
	int irq_gpio;
};

static int edt_ft5426_ts_write(struct edt_ft5426_dev *ft5426, u8 addr, u8 *buf, u16 len)
{
	struct i2c_client *client = ft5426->client;
	struct i2c_msg msg;
	u8 send_buf[6] ={0};
	int ret;

	send_buf[0] = addr;
	memcpy(&send_buf[1], buf, len);

	msg.flags = 0;
	msg.addr = client->addr;
	msg.buf = send_buf;
	msg.len = len + 1;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if(1 == ret){
		return 0;
	}else{
		dev_err(&client->dev, "%s:write error, addr=0x%x len=%d.\n", __func__, addr, len);
		return -1;
	}
}

static int edt_ft5426_ts_read(struct edt_ft5426_dev *ft5426, u8 addr, u8 *buf, u16 len)
{
	struct i2c_client *client = ft5426->client;
	struct i2c_msg msg[2];
	int ret;

	msg[0].flags = 0;
	msg[0].addr = clent->addr;
	msg[0].buf = &addr;
	msg[0].len = 1;

	msg[1].flags = I2C_M_RD;
	msg[1].addr = client->addr;
	msg[1].buf = buf;
	msg[1].len = len;

	ret = i2c_transfer(client->adapter, msg, 2);
	if(2== ret){
		return 0;
	}else{
		dev_err(&client->dev, "%s: read err,addr=0x%x len=%d.\n", __func__, addr, len);
		return -1;
	}
}

static int edt_ft5426_ts_reset(struct edt_ft5426_dev *ft5426)
{
	struct i2c_client *client = ft5426->client;
	int ret;

	ft5426->reset_gpio = of_get_named_gpio(client->dev.of_node, "reset-gpios", 0);
	if(gpio_is_valid(ft5426->reset_gpio) ==0){
		dev_err(&client->dev, "failed to get ts reset gpio\n");
		return ft5426->reset_gpio;
	}

	ret = devm_gpio_request_one(&client->dev, ft5426->reset_gpio, GPIO_OUT_INIT_HIGH, "ft5426 reset");
	if(ret < 0){
		return ret;
	}

	msleep(20);
	gpio_set_value_cansleep(ft5426->reset_gpio, 0);
	msleep(5);
	gpio_set_value_cansleep(ft5426->reset_gpio, 1);
	return 0;
}

static irqreturn_t edt_ft5426_ts_isr(int irq, void *dev_id)
{
	struct edt_ft5426_dev *ft6426 = dev_id;
	u8 rdbuf[30] = {0};
	int i, type, x,y, id;
	bool down;
	int ret;

	ret = edt_ft5426_read(ft6426, FT5426_TD_STATUS_REG, rdbuf, 29);
	if(ret!=0){
		goto out;
	}

	for(i=0;i<MAX_SUPPORT_POINTS;i++){
		u8 *buf = &rdbuf[i*6 + i];
		type = buf[0] >> 6;
		if(type == TOUCH_EVENT_RESERRVED){
			continue;
		}
		x = ((buf[2] << 8) | (buf[3])) & 0x0fff;
		y= ((buf[0] << 8) | (buf[1])) & 0x0fff;
		id = (buf[2] >> 4) & 0x0f;
		down = (type !=TOUCH_EVENT_UP);
		input_mt_slot(ft5426->input, id);
		input_mt_report_slot_state(ft5426->input, MT_TOOL_FINGER, down);
		if(down ==0){
			continue;
		}
		input_report_abs(ft5426->input, ABS_MT_POSITION_X, x);
		input_report_abs(ft5426->input, ABS_MT_POSITION_Y, y);
	}
	input_mt_report_pointer_emulation(ft5426->input, true);
	input_sync(ft5426->input);
out:
	return IRQ_HANDLED;
}

