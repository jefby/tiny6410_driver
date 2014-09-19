#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/dma-mapping.h>



#define BUF_SIZE (512*1024)//定义拷贝数据字节数512KB
#define TRANS_NO_DMA 0//不使用DMA传输数据
#define TRANS_USE_DMA 1//使用DMA传输数据


static char *src=NULL;//源内存的虚拟地址
static char *dst=NULL;//目的内存的虚拟地址
static u32 src_phys=0;//源内存的物理地址
static u32 dst_phys=0;//目的内存的物理地址

static int major = 0;


static struct class * cls = NULL;
static struct deivce * dev = NULL;
int dma6410_open (struct inode *inode, struct file *filp)
{
	
	return 0;
}
long dma6410_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	int i =0;
	memset(src,0xAA,BUF_SIZE);
	memset(dst,0x55,BUF_SIZE);
	switch(cmd)
	{
		case TRANS_NO_DMA:
			for(i=0;i<BUF_SIZE;++i)
				dst[i] = src[i];//拷贝
			//检查拷贝是否正确
			if(memcmp(src,dst,BUF_SIZE) == 0)
			{
				printk("TRANS_NO_DMA ok.");
			}else{
				printk("TRANS_NO_DMA err.\n");
			}
			break;
		case TRANS_USE_DMA:
			break;
		default:
			printk(KERN_ALERT "cmd error");
			return -EINVAL;
	}
	return 0;
}


static struct file_operations fops = {
	.open = dma6410_open,
	.unlocked_ioctl = dma6410_ioctl,
};


static int dma6410_init(void)
{
	//申请连续的内存空间
	src = dma_alloc_coherent(NULL,BUF_SIZE,&src_phys,GFP_KERNEL);
	if(!src){
		printk(KERN_ERR "dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL) error.\n");
		return -ENOMEM;
	}
	dst = dma_alloc_coherent(NULL,BUF_SIZE,&dst_phys,GFP_KERNEL);
	if(!dst){
		printk(KERN_ERR "dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL) error.\n");
		dma_free_coherent(NULL,BUF_SIZE,src,src_phys);//释放掉第一步申请的源内存
		return -ENOMEM;
	}	
	//注册字符设备,返回主设备号
    	major = register_chrdev(0,"dma-6410",&fops);

	//自动创建设备
	cls = class_create(THIS_MODULE,"dma-6410");//创建类dma-6410,/sys/class/dma-6410
	if(!cls){
		printk(KERN_ERR "class_create(THIS_MODULE,\"dma-6410\") error.\n");
		return -ENOMEM;
	}
	dev = device_create(cls,NULL,MKDEV(major,0),NULL,"dma-6410s");//创建设备节点/dev/dma-6410s
	if(!dev){
		printk("device_create err.\n");
		return -1;
	}
	
	printk(KERN_ALERT "6410dma init.\n");
	return 0;
}
static void  dma6410_exit(void)
{
	dma_free_coherent(NULL,BUF_SIZE,src,src_phys);
	dma_free_coherent(NULL,BUF_SIZE,dst,dst_phys);

	device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	unregister_chrdev(major,"dma-6410");
	printk(KERN_ALERT "6410dma exit.\n");
}

module_init(dma6410_init);
module_exit(dma6410_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jef199006@gmail.com jefby");
