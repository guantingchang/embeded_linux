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

static blk_qc_t ramdisk_make_request_function(struct request_queue *queu, struct bio* bio)
{
	int offset;
	struct bio_vec bvec;
	struct bvec_iter iter;
	unsigned long len = 0;
	struct ramdisk_dev *dev = queue->queuedata;

	offset = (bio->bi_iter.bi_sector) << 9;
	spin_lock(&dev->lock);
	bio_for_each_segment(bvec, bio, iter){
		char *ptr = page_address(bvec.bv_page) + bvec.bv_offset;
		len = bvec.bv_len;

		if(bio_data_dir(bio) == READ){
			memcpy(ptr, dev->ramdiskbuf + offset, len);
		}else if(bio_data_dir(bio)== WRITE){
			memcpy(dev->ramdiskbuf + offset, ptr, len);
		}
		offset += len;
	}
	spin_unlock(&dev->lock);
	bio_endio(bio);
	return BLK_QC_T_NONE;
}

static struct request_queue *create_req_queue(struct ramdisk_dev *dev)
{
	struct request_queue *queue;

	queue = blk_alloc_queue(GFP_KERNEL);
	blk_queue_make_request(queue, ramdisk_make_request_function);
	queue ->queuedata = dev;
	return queue;
}

static int create_req_gendisk(struct ramdisk_dev *dev)
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