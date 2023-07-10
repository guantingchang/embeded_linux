#include <linux/module.h>
#include <linux/genhd.h>
#include <linux/blk-mq.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>

#define RAMDISK_SIZE (2*1024*1024)
#define RAMDISK_NAME "ramdisk"
#define RAMDISK_MINOR 3

struct ramdisk_dev {
	int major;
	unsigned char *ramdiskbuf;
	struct gendisk *gendisk;
	struct request_queue *queue;
	struct blk_mq_tag_set tag_set;
	spinlock_t lock;
};

struct ramdisk_dev *ramdisk = NULL;

static int __init ramdisk_init(void)
{
	int ret = 0;
	struct ramdisk_dev *dev;
	printk("ramdisk init\r\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if(dev == NULL){
		return -ENOMEM;
	}

	dev->ramdiskbuf = kmalloc(RAMDISK_SIZE, GFP_KERNEL);
	if(dev->ramdiskbuf == NULL){
		printk(KERN_WARNING "dev->ramdiskbuf:vmalloc failure.\n");
		return -ENOMEM;
	}
	ramdisk = dev;
	
}