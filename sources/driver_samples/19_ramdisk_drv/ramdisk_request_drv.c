#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>	
#include <linux/slab.h>		
#include <linux/fs.h>		
#include <linux/errno.h>	
#include <linux/types.h>	
#include <linux/fcntl.h>	
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
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


int ramdisk_open(struct block_device *dev, fmode_t mode)
{
	printk("ramdisk open\r\n");
	return 0;
}

void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
	printk("ramdisk release\r\n");
}

int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
	geo->heads = 2;
	geo->cylinders = 32;
	geo->sectors = RAMDISK_SIZE/(2*32*512);
	return 0;
}

static struct  block_device_operations ramdisk_fops = {
	.owner = THIS_MODULE,
	.open = ramdisk_open,
	.release = ramdisk_release,
	.getgeo = ramdisk_getgeo,
};

static int ramdisk_transfer(struct request *req)
{
	unsigned long start = blk_rq_pos(req) << 9;
	unsigned long len = blk_rq_cur_bytes(req);

	void *buffer = bio_data(req->bio);
	if(rq_data_dir(req) == READ){
		memcpy(buffer, ramdisk->ramdiskbuf + start, len);
	}else if(rq_data_dir(req) == WRITE){
		memcpy(ramdisk->ramdiskbuf + start, buffer, len);
	}
	return 0;
}

static blk_status_t __queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)
{
	struct request *req = bd->rq;
	struct ramdisk_dev *dev = req->rq_disk->private_data;
	int ret;

	blk_mq_start_request(req);
	spin_lock(&dev->lock);
	ret = ramdisk_transfer(req);
	blk_mq_end_request(req, ret);
	spin_unlock(&dev->lock);
	return BLK_STS_OK;
}

static struct blk_mq_ops mq_ops = {
	.queue_rq = __queue_rq,
};

static struct request_queue *create_req_queue(struct ramdisk_dev *dev)
{
	struct request_queue *queue;
	struct blk_mq_tag_set *set = &dev->tag_set;

	#if 0
		queue = blk_mq_init_sq_queue(set, &mq_ops, 2, BLK_MQ_F_SHOULD_MERGE);
	#else
		int ret;
		memset(set, 0, sizeof(*set));
		set->ops = &mq_ops;
		set->nr_hw_queues =2;
		set->queue_depth = 2;
		set->numa_node = NUMA_NO_NODE;
		set->flags = BLK_MQ_F_SHOULD_MERGE;
		
		ret = blk_mq_alloc_tag_set(set);
		if(ret){
			printk(KERN_WARNING "sblkdev:unable to allocate tag set\n");
			return ERR_PTR(ret);
		}
		queue = blk_mq_init_queue(set);
		if(IS_ERR(queue)){
			blk_mq_free_tag_set(set);
			return queue;
		}
	#endif
		return queue;

}

static int create_req_gendisk(struct ramdisk_dev *set)
{
	struct ramdisk_dev *dev = set;

	dev->gendisk = alloc_disk(RAMDISK_MINOR);
	if(dev == NULL){
		return -ENOMEM;
	}
	dev ->gendisk->major = ramdisk->major;
	dev->gendisk->first_minor = 0;
	dev->gendisk->fops = &ramdisk_fops;
	dev->gendisk->private_data = set;
	dev->gendisk->queue = dev->queue;
	sprintf(dev->gendisk->disk_name, RAMDISK_NAME);
	set_capacity(dev->gendisk, RAMDISK_SIZE/512);
	add_disk(dev->gendisk);
	return 0;
}


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
	spin_lock_init(&dev->lock);
	dev->major = register_blkdev(0, RAMDISK_NAME);
	if(dev->major < 0){
		goto register_blkdev_fail;
	}
	dev->queue = create_req_queue(dev);
	if(dev->queue == NULL){
		goto create_queue_fail;
	}
	ret = create_req_gendisk(dev);
	if(ret < 0){
		goto create_gendisk_fail;
	}
	return 0;
create_gendisk_fail:
	blk_cleanup_queue(dev->queue);
	blk_mq_free_tag_set(&dev->tag_set);
create_queue_fail:
	unregister_blkdev(dev->major, RAMDISK_NAME);	
register_blkdev_fail:
	kfree(dev->ramdiskbuf);
	kfree(dev);
	return -ENOMEM;
}

static void __exit ramdisk_exit(void)
{
	printk("ramdisk exit\r\n");
	del_gendisk(ramdisk->gendisk);
	put_disk(ramdisk->gendisk);

	blk_cleanup_queue(ramdisk->queue);
	blk_mq_free_tag_set(&ramdisk->tag_set);

	unregister_blkdev(ramdisk->major, RAMDISK_NAME);	

	kfree(ramdisk->ramdiskbuf);
	kfree(ramdisk);
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("GTC");
MODULE_INFO(intree, "Y");