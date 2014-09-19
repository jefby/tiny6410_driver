#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>//file_operations
#include <linux/device.h>//class,device_create
#include <asm/string.h>//memset,memcmp
#include <linux/irq.h>//IRQ_TYPE_EDGE_BOTH
#include <asm/io.h>//ioremap
#include <linux/wait.h>//wait_event_interruptible
#include <linux/sched.h>//request_irq,free_irq
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq


#define BUF_SIZE (512*1024)//定义拷贝数据字节数512KB
#define TRANS_NO_DMA 0//不使用DMA传输数据
#define TRANS_USE_DMA 1//使用DMA传输数据

//S3C6410-DMA通道定义
struct s3c6410_dma_chan{
	unsigned long DMACCxSrcAddr;//源地址
	unsigned long DMACCxDestAddr;//目的地址
	unsigned long DMACCxLLI;//链接的链表地址
	unsigned long DMACCxControl0;//通道控制器0
	unsigned long DMACCxControl1;//通道控制器1
	unsigned long DMACCxConfiguration;//配置寄存器
	unsigned long DMACCxConfigurationExp;//配置寄存器扩展
};
//s3c6410 dmac控制器结构定义
struct s3c6410_dmac{
	unsigned long DMACIntStatus;
	unsigned long DMACIntTCStatus;
	unsigned long DMACIntTCClear;
	unsigned long DMACIntErrorStatus;
	unsigned long DMACIntErrClr;
	unsigned long DMACRawIntTCStatus;
	unsigned long DMACRawIntErrorStatus;
	unsigned long DMACEnbldChns;
	unsigned long DMACSoftBReq;
	unsigned long DMACSoftSReq;
	unsigned long reserved[2];//保留
	unsigned long DMACConfiguration;
	unsigned long DMACSync;
};


static char *src=NULL;//源内存的虚拟地址
static char *dst=NULL;//目的内存的虚拟地址
static u32 src_phys=0;//源内存的物理地址
static u32 dst_phys=0;//目的内存的物理地址

static int major = 0;
static volatile struct s3c6410_dma_chan *dma_chan_p = NULL;
static volatile struct s3c6410_dmac *dmac_p = NULL;
static volatile unsigned long * sdma = NULL;

static struct class * cls = NULL;
static struct deivce * dev = NULL;

//定义并初始化等待队列
static DECLARE_WAIT_QUEUE_HEAD(sdma_waitq);
static volatile int ev_dma = 0;


//DMA中断处理函数
irqreturn_t sdma_irq(int irq, void *devid)
{
	ev_dma = 1;
	wake_up_interruptible(&sdma_waitq);//唤醒休眠队列
	return IRQ_HANDLED;
}


int dma6410_open (struct inode *, struct file *)
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
		case TRANS_USE_DMA://启动方式:手工启动
			//5.设置源地址
			dma_chan_p->DMACCxSrcAddr = src_phys;//源地址
			//6.设置目的地址
			dma_chan_p->DMACCxDestAddr = dst_phys;//目的物理地址
			//7.写入下一个LLI
			dma_chan_p->DMACCxLLI = 0;//设置AHB master select for loading the next LLI:LM = 0 = AHB master 1
			//8.写控制信息
			dma_chan_p->DMACCxControl0 = (1 << 27) \ //Destination increment
										| (1 <<26) \//Source increment
										| (0 <<25) \//Destination AHB master select:0 = AHB master 1 (AXI_SYSTEM) selected for the destination transfer
										| (0<<24) \//Source AHB master select:0 = AHB master 1 (AXI_SYSTEM) selected for the source transfer
										| (2<<21) \//Destination transfer width,32bit
										| (2<<18) \//Source transfer width
										| (0<<15) \//Destination burst size
										| (0<<12) \//Source burst size
										;
			dma_chan_p->DMACCxControl1 = BUF_SIZE;//设置传输的字节数
			//9.写通道配置信息，启动通道0
			dma_chan_p->DMACCxConfiguration = (0 << 18) \ //allow DMA request
											| (0 << 11) \ //Flow control and transfer type ; Memory to Memory
											| (1 << 0);//channel enabled
			//如果DMA传输未完则睡眠
			wait_event_interruptible(&sdma_waitq,ev_dma);

			//DMACCxConfigurationExp;配置寄存器扩展用于在Mem-to-Mem传输时外设的配置，因为不用外设，所以不做设置
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
	int ret = 0;
	int chx = 0;
	//中断请求
	ret = request_irq(IRQ_SDMA0, sdma_irq,IRQ_TYPE_EDGE_BOTH,"sdma-0",1);
	if(ret){
		printk(KERN_ERR "request_irq err.\n");
		return -1;
	}
	//如果申请中断请求成功，则使能DMAC3，也就是SDMAC0
	dmac_p = ioremap(0x7DB00000,0x200);//映射SDMAC0的物理地址到虚拟地址处
	if(!dmac_p){
		printk("ioremap(0x7DB00000,0x200) err.\n");
		free_irq(IRQ_SDMA0,1);//释放申请的中断
		return -1;
	}
	
	dmac_p->DMACConfiguration = 0x1 | 0x0<<1;//启用SDMA控制器0
	
	//1.检查通道0是否正在被使用
	if(!dmac_p->DMACEnbldChns&0x1){
		dmac_p->DMACEnbldChns |= 0x1;//启用通道0
		dma_chan_p = dmac_p + 0x100;//设置为通道0的起始地址
	}
	//2.决定使用SDMAC还是通用的DMAC
	sdma = ioremap(0x7E00_F110,16);//SDMA_SEL ,设置使用SDMA
	iowrite32(0,sdma);
	iounmap(sdma);//清除映射
	
	//3.使用空闲通道0
	//4.清除在该中断上挂起的中断
	dmac_p->DMACIntErrClr |= 0x1;
	dmac_p->DMACIntTCClear |= 0x1;
	//5.剩下的到ioctl中继续配置
	//申请连续的内存空间
	src = dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL);
	if(!src){
		printk(KERN_ERR "dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL) error.\n");
		return -ENOMEM;
	}
	dst = dma_alloc_writecombine(NULL,BUF_SIZE,&dst_phys,GFP_KERNEL);
	if(!dst){
		printk(KERN_ERR "dma_alloc_writecombine(NULL,BUF_SIZE,&src_phys,GFP_KERNEL) error.\n");
		dma_free_writecombine(NULL,BUF_SIZE,src,src_phys);//释放掉第一步申请的源内存
		return -ENOMEM;
	}	
	//注册字符设备,返回主设备号
    major = register_chrdev(0,"dma-6410",fops);
	if(ret){
		printk("register_chrdev err.\n");
		return -1;
	}
	//自动创建设备
	cls = class_create(THIS_MODULE,"dma-6410");//创建类dma-6410,/sys/class/dma-6410
	if(!cls){
		printk(KERN_ERR "class_create(THIS_MODULE,"dma-6410") error.\n");
		return -ENOMEM;
	}
	device_create(cls,NULL,MKDEV(major,0),NULL,"dma-6410s");//创建设备节点/dev/dma-6410s


	printk(KERN_ALERT "6410dma init.\n");
	return 0;
}
static void  dma6410_exit(void)
{
	free_irq(IRQ_SDMA0,1);//释放申请的中断
	iounmap(dmac_p);//拆掉映射连接
	dma_free_writecombine(NULL,BUF_SIZE,src,src_phys);
	dma_free_writecombine(NULL,BUF_SIZE,dst,dst_phys);

	device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	unregister_chrdev(major,"dma-6410");
	printk(KERN_ALERT "6410dma exit.\n");
}

module_init(dma6410_init);
module_exit(dma6410_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jef199006@gmail.com jefby");














