#include <linux/init.h>//module_init/exit
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE等
#include <linux/genhd.h>//alloc_disk
#include <linux/blkdev.h>//blk_init_queue
#include <linux/fs.h>//register_blkdev,unregister_blkdev
#include <linux/types.h>//u_char,u_short
#include <linux/vmalloc.h>
#include <linux/hdreg.h>
//1MB大小空间
#define RAMBLK_SIZE (1024*1024*32)
#define RAM_CHUNKSIZE		(0x00001000)
#define RAM_CHUNKMASK		(0x00000fff)
#define RAM_CHUNKSHIFT	(12)

/*
//定义驱动私有数据结构
struct ramblk_info{
	u_char heads;//磁头数
	u_short cylinders;//柱面数
	u_char sectors;//扇区数
	u_char control;
	int unit;
};
*/

static struct gendisk * ramblk_disk = NULL;
static struct request_queue * ramblk_request_queue = NULL;
static int major = 0;//块设备的主设备号
//static struct ramblk_info *pinfo = NULL;
static DEFINE_SPINLOCK(ramblk_spinlock);//定义并初始化一个自旋锁
static char * ramblk_buf = NULL;//申请的内存起始地址

//ramdisk_info初始化函数
/*
static int ramblk_info_init(struct ramblk_info *p)
{
	if(!p){
		printk("ramblk_info_init p==NULL.\n");
		return -1;
	}
	p->heads = 4;//4个磁头
	p->cylinders = 4;//4个柱面
	p->sectors = 128;//128个扇区
	return 0;
}
*/
int ramblk_getgeo(struct block_device * blk_Dev, struct hd_geometry * hg)
{
	hg->cylinders = 64;
	hg->heads = 8;
	hg->sectors = (RAMBLK_SIZE/8/64/512);
	return 0;
}



static const struct block_device_operations ramblk_fops = {
	.owner	= THIS_MODULE,
	.getgeo = ramblk_getgeo,
};

static void do_ramblk_request(struct request_queue *q )
{
	struct request *req;
//	static volatile int r_cnt = 0;
//	static volatile int w_cnt = 0;
	//printk("ramblk_request_fn %d.\n",cnt++);
	req = blk_fetch_request(q);
	while (req) {
			unsigned long start = blk_rq_pos(req) << 9;
			unsigned long len  = blk_rq_cur_bytes(req);
//			printk("len=%d.\n",len);
		
			if (start + len > RAMBLK_SIZE) {
					printk("RAMBLK_SIZE< start+len");
					goto done;
				}
			
			if (rq_data_dir(req) == READ)
				memcpy(req->buffer, (char *)(start+ramblk_buf), len);
			else
				memcpy((char *)(start+ramblk_buf), req->buffer, len);
		
			done:
				if (!__blk_end_request_cur(req, 0))
					req = blk_fetch_request(q);
	}
	
}


static int ramblk_init(void)
{
//	1.分配gendisk结构体，使用alloc_disk函数
	ramblk_disk = alloc_disk(16);//minors=分区+1
//	2.设置
//	2.1 分配/设置队列，提供读写能力.使用函数blk_init_queue(request_fn_proc *rfn,spin_lock_t *lock)
	ramblk_request_queue = blk_init_queue(do_ramblk_request,&ramblk_spinlock);
//	2.2 设置disk的其他信息，比如容量、主设备号等
	major = register_blkdev(0,"ramblk");//注册主设备
	if(major < 0){//检查是否成功分配一个有效的主设备号
		printk(KERN_ALERT "register_blkdev err.\n");
		return -1;
	}
	//设置主设备号
	ramblk_disk->major = major;
	ramblk_disk->first_minor = 0;//设置第一个次设备号
	sprintf(ramblk_disk->disk_name, "ramblk%c", 'a');//设置设备名
	ramblk_disk->fops = &ramblk_fops;//设置fops
	/*
	//分配一个ramdisk_info结构体，并初始化
	pinfo =(struct ramblk_info*)kmalloc(sizeof(struct ramblk_info),GFP_KERNEL);
	if(!pinfo){
		printk("kmalloc pinfo err.\n");
		return -1;
	}
	ramblk_info_init(pinfo);
	ramlk_disk->private_data = pinfo;*/
	ramblk_disk->queue = ramblk_request_queue;//设置请求队列
	set_capacity(ramblk_disk, RAMBLK_SIZE/512);//设置容量
	
//	3.硬件相关的操作
	ramblk_buf = (char*)vmalloc(RAMBLK_SIZE);//申请RAMBLK_SIZE内存
	
//	4.注册
	add_disk(ramblk_disk);//add partitioning information to kernel list
	printk("ramblk_init.\n");
	return 0;
}

static void ramblk_exit(void)
{
	
	unregister_blkdev(major,"ramblk");//注销设备驱动
	blk_cleanup_queue(ramblk_request_queue);//清除队列
	del_gendisk(ramblk_disk);
	put_disk(ramblk_disk);
	vfree(ramblk_buf);//释放申请的内存
	printk("ramblk_exit.\n");
}


module_init(ramblk_init);//入口
module_exit(ramblk_exit);//出口

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");






