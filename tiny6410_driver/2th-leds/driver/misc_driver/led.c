/*
 *	Tiny6410开发板－led驱动(linux)使用miscdevice实现
 *	使用方法:
 *	1.编译内核2.6.38
 *	2.编写Makefile文件
 *	3.编译运行
 *	4.运行app/led测试(有关具体的测试细节清参考led_app.c)
 *	本程序中读写GPIO使用了新的访问I/O内存的函数ioread32,iowrite32
 *	Author:jefby
 *	Emai:jef199006@gmail.com
 */ 
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE
#include <linux/init.h>//module_init,module_exit
#include <linux/fs.h>//file_operations
#include <linux/miscdevice.h>//misdevice 
#include <asm/io.h>//ioread32,iowrite32
#include <mach/gpio-bank-k.h>//定义了GPKCON
#include <mach/regs-gpio.h>//定义了gpio-bank-k中使用的S3C64XX_GPK_BASE
#include <mach/map.h>//定义了S3C64XX_VA_GPIO

/*设备名称*/
#define DEVICE_NAME "leds"

/*	ioctl接口函数,cmd=0,表示关闭参数arg指定的LED灯;arg的值不能大于4；
 *	其中0表示所有的LED,LED1~LED4
 *	1~4分别表示LED1~LED4
 *	返回0或者错误-EINVAL
 * */
static int s3c6410_leds_ioctl(
				 struct file*filp,
				 unsigned int cmd,
				 unsigned long arg
			      )
{
	switch(cmd){
		unsigned tmp;
	case 0://close
	case 1://open
		if(arg > 4){//参数错误
			return -EINVAL;		
		}
		else if(arg == 0){//全亮或者全灭
			tmp = ioread32(S3C64XX_GPKDAT);//读出LED1~LED4所在寄存器的值
			tmp &= ~(0xF<<4);//打开LED1~LED4
			if(cmd == 0)//若是关闭，则关闭掉LED1~LED4
				tmp |= (0xF<<4);//if close,then write 0xF to GPK4~GPK7
			iowrite32(tmp,S3C64XX_GPKDAT);
		}else{	//参数为1～4范围内
			tmp = ioread32(S3C64XX_GPKDAT);
			tmp &= ~(1<<(4+arg-1));//清除arg所指示的那一位值
			tmp |= ((!cmd)<<(4+arg-1));//写入新值
			iowrite32(tmp,S3C64XX_GPKDAT);
		}
		return 0;
	default:
		return -EINVAL;
	}//switch(cmd)
	return 0;//应该不会执行
}

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = s3c6410_leds_ioctl,//定义的ioctl函数
};
static struct miscdevice misc  = {
	.minor = MISC_DYNAMIC_MINOR,//动态分配次设备号
	.name = DEVICE_NAME,//设备名称
	.fops = &dev_fops,
};

/*模块初始化*/
static int __init dev_init(void)
{
	int ret;
/*
 *	在设备驱动程序注册的时候初始化LED1～LED4所对应的GPIO管脚为输出
 *	并关闭LED1~LED4
 */ 
	unsigned tmp;
	tmp = ioread32(S3C64XX_GPKCON);
	tmp = (tmp & ~(0xFFFFU<<16)) | (0x1111U<<16);//先清除然后再设置其为输出
	iowrite32(tmp,S3C64XX_GPKCON);//写入GPKCON
	
	tmp = readl(S3C64XX_GPKDAT);
	tmp |= (0xF<<4);//关闭LED灯
	iowrite32(tmp,S3C64XX_GPKDAT);
//注册misc
	ret = misc_register(&misc);
	printk(DEVICE_NAME"\tinitialized.\n");

	return ret;
}

static void __exit dev_exit(void)
{
//卸载
	misc_deregister(&misc);
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jefby");























