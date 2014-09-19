/*
 *
  *	buttons驱动程序TIny6410
 *	使用标准字符设备驱动编写方法
 *	make完成后根据打印到终端的输出，创建字符设备
 *	格式如下:
 *	mknod /dev/buttons c 主设备号 0
 *	目前存在一些问题，只能驱动按键K1~K4,不能驱动K5~K7,原因是request_irq函数调用失败
 *	另外，使用ctrl+c中断后再次加载驱动会失败，原因不清楚
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *	K1,K2,K3,K4 => GPN0,1,2,3
 *	本程序使用查询的方式来获取按键值: CPU不断的读取按键状态.
 */
#include <linux/module.h>//MODULE_LICENSE,MODULE_AUTHOR
#include <linux/init.h>//module_init/module_exit


#include <linux/fs.h>//file_operations
#include <asm/io.h>//ioread32,iowrite32
#include <linux/cdev.h>//cdev
#include <mach/map.h>//定义了S3C64XX_VA_GPIO
#include <mach/regs-gpio.h>//定义了gpio-bank-n中使用的S3C64XX_GPN_BASE
#include <mach/gpio-bank-n.h>//定义了GPNCON
#include <mach/gpio-bank-l.h>//定义了GPNCON
#include <linux/wait.h>//wait_event_interruptible(wait_queue_head_t q,int condition);
//wake_up_interruptible(struct wait_queue **q)
#include <linux/sched.h>//request_irq,free_irq
#include <asm/uaccess.h>//copy_to_user
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq
#include <linux/device.h>//class device
MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with search");

#define GPNCON 0x7F008830
#define GPLCON0 0x7F008810

static volatile unsigned int * gpncon = NULL;
static volatile unsigned int * gpndat = NULL;
static volatile unsigned int * gplcon = NULL;
static volatile unsigned int * gpldat = NULL;
//按键设备的主设备号
static int buttons_major = 0;
//设备号
dev_t dev;
//字符设备
struct cdev * buttons_cdev;

static struct class * tiny6410_buttons_class = NULL;
static struct device * tiny6410_buttons_device = NULL;


//设备打开操作，主要完成BUTTONS所对应的GPIO的初始化，注册用户中断处理函数

int  buttons_open(struct inode *inode,struct file *filp)
{
	unsigned val;

	/*设置buttons对应的GPIO管脚,设置KEY1~KEY6*/
	gpncon = (volatile unsigned int*)ioremap(GPNCON,16);
	gpndat = gpncon + 1;
	val = ioread32(gpncon);//读取GPNCON的值
	val = (val & ~(0xFFF));//设置GPIO 0～5为输入
	iowrite32(val,gpncon);

	//设置KEY7,KEY8为输入,gpl11,gpl12
	gplcon = (volatile unsigned int*)ioremap(GPLCON0,16);
	gpldat = gplcon + 2;//gpldat
	val = ioread32(gplcon+1);//读取GPNCON1的值
	val = (val & ~(0xFF<<12));//设置GPL11和12为输入
	iowrite32(val,gplcon+1);

/*
	val = ioread32(S3C64XX_GPLCON1);
	val = (val & ~(0xFF<<12)) | (0x33);
	iowrite32(val,S3C64XX_GPLCON1);
*/
	printk("buttons open.\n");
	return 0;
}
//按键读若没有键被按下，则使进程休眠；若有按键被按下，则拷贝数据到用户空间，然后清零
int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned char keyval[8]={0};
	unsigned int temp=0;
	int i=0;
	if(len != 8)
		return -1;
	temp=ioread32(gpndat);
	//读取KEY1~KEY6的值
	for(i=0;i<6;++i){
		keyval[i] = (temp&(0x1<<i))?1 : 0;
	}
	temp = ioread32(gpldat);
	//读取KEY7和KEY8的值
	keyval[6]=(temp&(0x1<<11))?1:0;
	keyval[7]=(temp&(0x1<<12))?1:0;
	copy_to_user(buf,keyval,sizeof(keyval));

	return 0;

}
//主要是卸载用户中断处理程序
int buttons_close(struct inode *inode,struct file *filp)
{
	printk("buttons close.\n");
	return 0;
}


static struct file_operations buttons_fops = {
	.owner = THIS_MODULE,
	.read = buttons_read,
	.release = buttons_close,
	.open = buttons_open,
};
/*
	模块初始化：
		1.申请设备号，默认使用动态分配的方法
		2.申请并初始化cdev结构
		3.将cdev注册到内核
*/
static int module_buttons_init(void)
{
	int err=0;
	int result=0;
	printk("Tiny6410 buttons module init.\n");	
	if(buttons_major){
		dev = MKDEV(buttons_major,0);
		result = register_chrdev_region(dev,1,"buttons");
	}else{
		result = alloc_chrdev_region(&dev,0,1,"buttons");
		buttons_major = MAJOR(dev);
	}
	if(result < 0){
		printk(KERN_WARNING "buttons : can't get major %d\n",buttons_major);
	}

	printk("buttons major is %d",buttons_major);
	buttons_cdev = cdev_alloc();
	buttons_cdev ->ops = &buttons_fops;
	cdev_init(buttons_cdev,&buttons_fops);
	cdev_add(buttons_cdev,dev,1);

	tiny6410_buttons_class = class_create(THIS_MODULE, "tiny6410buttons");
	if (IS_ERR(tiny6410_buttons_class)) {
		err = PTR_ERR(tiny6410_buttons_class);
		printk("create class error.\n");
	}
	tiny6410_buttons_device = device_create(tiny6410_buttons_class, NULL, MKDEV(buttons_major, 0), NULL,
			      "buttons");
	printk("buttons add ok.\n");
	return 0;
}

static void module_buttons_exit(void)
{
	iounmap(gpncon);
	device_destroy(tiny6410_buttons_class, MKDEV(buttons_major, 0));
	class_destroy(tiny6410_buttons_class);
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
