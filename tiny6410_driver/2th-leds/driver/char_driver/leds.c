/*
 *
 *	TIny6410的LED灯驱动程序(linux)
 *	使用标准的字符设备驱动程序来完成，和裸机程序的不同在于需要理解字符设备的框架，
 *	需要理解cdev结构的初始化和使用
 *	使用了主设备号动态分配，并打印到系统日志中,然后手动创建字符设备，接着使用应用程序对其进行操作
 *	创建字符设备格式:mknod /dev/leds c 主设备号 0
 *
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 */
 
#include <linux/init.h>//module_init,module_exit
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE

#include <linux/fs.h>//file_operations
#include <linux/cdev.h>//字符设备结构cdev需要的头文件
#include <mach/map.h>//定义了S3C64XX_VA_GPIO
#include <mach/regs-gpio.h>//定义了gpio-bank-k中使用的S3C64XX_GPK_BASE
#include <mach/gpio-bank-k.h>//定义了GPKCON
#include <asm/io.h>//ioread32,iowrite32


MODULE_LICENSE("GPL");
MODULE_AUTHOR("jefby");

static struct cdev * leds_cdev;
static int leds_major = 0;
static dev_t dev;

int leds_open(struct inode * inode,struct file * filp)
{
	/*在这里初始化I/O管脚，设置其为输出*/	
	unsigned long tmp;
	tmp = ioread32(S3C64XX_GPKCON);
	tmp = (tmp & ~(0xFFFFU<<16)) | (0x1111<<16);
	iowrite32(tmp, S3C64XX_GPKCON);
	//关闭LED1~LED4
	return 0;
}
int leds_ioctl(
		struct file * filp,
		unsigned int cmd,
		unsigned long arg
	     	)
{
	if(arg > 4)//确定参数必须为0，此时ON和OFF都对应的是LED1~LED4
		return -EINVAL;
	switch(cmd){
		unsigned long tmp;
		case 0:
		case 1:
			if(arg == 0){
				tmp = ioread32(S3C64XX_GPKDAT);
				tmp &= ~(0xF<<4);
				if(cmd == 0)
					tmp |= 0xF<<4;
				iowrite32(tmp,S3C64XX_GPKDAT);
					
			}else{
				tmp = ioread32(S3C64XX_GPKDAT);
				tmp &= ~(0x1<<(4+arg-1));//打开arg对应的LED灯
				if(cmd == 0)
					tmp |= 0x1<<(4+arg-1);
				iowrite32(tmp,S3C64XX_GPKDAT);
			}
			return 0;
		default:
			return -EINVAL;
	}
	return 0;
}
		
struct file_operations leds_fops = {
	.owner = THIS_MODULE,
	.open = leds_open,
	.unlocked_ioctl = leds_ioctl,
};
static int __init leds_init(void)
{
	/*
	 *使用linux设备驱动中介绍的新方法来写，而不是用老的接口
	 *申请主设备号
	 *register_chrdev
	 *新方法:
	 *	0.获得一个或者多个设备编号(register_chrdev_region,或者alloc_chrdev_region)
	 *	1.分配cdev结构
	 *	2.初始化该cdev结构
	 *	3.注册到内核
	 *
	 */
	int result;

	printk("Tiny 6410 leds module init.\n");
	if(leds_major){
		dev = MKDEV(leds_major,0);
		result = register_chrdev_region(dev,1,"leds");
	}else{
		result = alloc_chrdev_region(&dev,0,1,"leds");
		leds_major = MAJOR(dev);//获得主设备号
		printk(KERN_ALERT "leds major = %d.\n",leds_major);
	}
	if(result < 0){
		printk(KERN_WARNING "leds:can't get major %d\n",leds_major);
		return result;
	}
	leds_cdev  = cdev_alloc();
	leds_cdev->ops = &leds_fops;
	//void cdev_init(struct cdev*cdev,struct file_operations *fops)
	cdev_init(leds_cdev,&leds_fops);
	//int cdev_add(struct cdev *dev,dev_t num,unsigned int count)
	cdev_add(leds_cdev,dev,1);
	printk("cdev add ok.\n");
	

	return 0;
}

static void __exit leds_exit(void)
{
	cdev_del(leds_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny 6410 leds module exit.\n");
}


module_init(leds_init);
module_exit(leds_exit);


