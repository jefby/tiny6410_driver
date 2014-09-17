/*
	块设备驱动程序:搭建块设备驱动的框架
	该块设备由内存来模拟,动态分配1MB的内存作为块设备
*/


#include <linux/init.h>//module_init/exit
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE等
#include <linux/genhd.h>//alloc_disk
#include <linux/blkdev.h>//blk_init_queue
#include <linux/fs.h>//register_blkdev,unregister_blkdev
#include <linux/types.h>//u_char,u_short

#define RAMBLK_SIZE (1024*1024)//块设备大小




static struct gendisk * ramblk_disk = NULL;
static struct request_queue * ramblk_request_queue = NULL;
static int major = 0;//块设备的主设备号

static DEFINE_SPINLOCK(ramblk_spinlock);//定义并初始化一个自旋锁



int ramblk_ioctl(struct block_device *blk_dev, fmode_t fm, unsigned cmd , unsigned long arg)
{
	return 0;
}

int ramblk_getgeo(struct block_device * blk_Dev, struct hd_geometry * hg)
{
	return 0;
}



static const struct block_device_operations ramblk_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= ramblk_ioctl,
	.getgeo = ramblk_getgeo,
};
//块设备请求处理函数
static void ramblk_request_fn(struct request_queue *q )
{
	static int cnt = 0;
	struct request *req;
	printk("ramblk_request_fn %d.\n",cnt++);
	req = blk_fetch_request(q);//从一个请求队列中获取一个I/O请求
	while (req) {
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
	ramblk_request_queue = blk_init_queue(ramblk_request_fn,&ramblk_spinlock);
//	2.2 设置disk的其他信息，比如容量、主设备号等
	major = register_blkdev(0,"ramblk");//注册块设备
	if(major < 0){//检查是否成功分配一个有效的主设备号
		printk(KERN_ALERT "register_blkdev err.\n");
		return -1;
	}
	//设置主设备号
	ramblk_disk->major = major;
	ramblk_disk->first_minor = 0;//设置第一个次设备号
	sprintf(ramblk_disk->disk_name, "ramblk%c", 'a');//设置块设备名
	ramblk_disk->fops = &ramblk_fops;//设置fops

	//这个没有必要，可以删除.分配一个ramdisk_info结构体，并初始化
	#ifdef 0
	pinfo =(struct ramblk_info*)kmalloc(sizeof(struct ramblk_info),GFP_KERNEL);
	if(!pinfo){
		printk("kmalloc pinfo err.\n");
		return -1;
	}
	ramblk_info_init(pinfo);
	ramblk_disk->private_data = pinfo;
	#endif 
	
	ramblk_disk->queue = ramblk_request_queue;//设置请求队列
	set_capacity(ramblk_disk, RAMBLK_SIZE);//设置容量
	printk(" %s: CHS=%d/%d/%d\n", ramblk_disk->disk_name,pinfo->cylinders, pinfo->heads, pinfo->sectors);
//	3.注册
	add_disk(ramblk_disk);//add partitioning information to kernel list
	printk("ramblk_init.\n");
	return 0;
}

static void ramblk_exit(void)
{
	kfree(pinfo);//释放申请的空间
	unregister_blkdev(major,"ramblk");//注销设备驱动
	blk_cleanup_queue(ramblk_request_queue);//清除队列
	del_gendisk(ramblk_disk);
	put_disk(ramblk_disk);
	printk("ramblk_exit.\n");
}


module_init(ramblk_init);//入口
module_exit(ramblk_exit);//出口

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");






