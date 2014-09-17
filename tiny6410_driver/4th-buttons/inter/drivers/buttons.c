/*
 *
 *	buttonsÇı¶¯³ÌĞòTIny6410
 *	Ê¹ÓÃ±ê×¼×Ö·ûÉè±¸Çı¶¯±àĞ´·½·¨
 *	makeÍê³Éºó¸ù¾İ´òÓ¡µ½ÖÕ¶ËµÄÊä³ö£¬´´½¨×Ö·ûÉè±¸
 *	¸ñÊ½ÈçÏÂ:
 *	mknod /dev/buttons c Ö÷Éè±¸ºÅ 0
 *	Ä¿Ç°´æÔÚÒ»Ğ©ÎÊÌâ£¬Ö»ÄÜÇı¶¯°´¼üK1~K4,²»ÄÜÇı¶¯K5~K7,Ô­ÒòÊÇrequest_irqº¯Êıµ÷ÓÃÊ§°Ü
 *	ÁíÍâ£¬Ê¹ÓÃctrl+cÖĞ¶ÏºóÔÙ´Î¼ÓÔØÇı¶¯»áÊ§°Ü£¬Ô­Òò²»Çå³ş
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 */
#include <linux/module.h>//MODULE_LICENSE,MODULE_AUTHOR
#include <linux/init.h>//module_init/module_exit


#include <linux/fs.h>//file_operations
#include <asm/io.h>//ioread32,iowrite32
#include <linux/cdev.h>//cdev
#include <mach/map.h>//¶¨ÒåÁËS3C64XX_VA_GPIO
#include <mach/regs-gpio.h>//¶¨ÒåÁËgpio-bank-nÖĞÊ¹ÓÃµÄS3C64XX_GPN_BASE
#include <mach/gpio-bank-n.h>//¶¨ÒåÁËGPNCON
#include <mach/gpio-bank-l.h>//¶¨ÒåÁËGPNCON
#include <linux/wait.h>//wait_event_interruptible(wait_queue_head_t q,int condition);
//wake_up_interruptible(struct wait_queue **q)
#include <linux/sched.h>//request_irq,free_irq
#include <asm/uaccess.h>//copy_to_user
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with interrupt");

//buttons irqÃèÊö½á¹¹Ìå
struct buttons_irq_desc{
	int irq;//ÖĞ¶ÏºÅ
	unsigned long flags;//ÖĞ¶Ï´¥·¢·½Ê½
	char *name;//Ãû³Æ
};
//irqÃèÊö·ûµÄ³õÊ¼»¯,ÓÃÀ´Ö¸¶¨ËùÓÃµÄÍâ²¿ÖĞ¶ÏÒı½ÅÒÔ¼°ÖĞ¶Ï´¥·¢·½Ê½¡¢Ãû×Ö
static struct buttons_irq_desc buttons_irqs[] = {
	{IRQ_EINT(0),IRQ_TYPE_EDGE_RISING,"KEY1"},//KEY1
	{IRQ_EINT(1),IRQ_TYPE_EDGE_RISING,"KEY2"},//KEY2	
	{IRQ_EINT(2),IRQ_TYPE_EDGE_RISING,"KEY3"},//KEY3	
	{IRQ_EINT(3),IRQ_TYPE_EDGE_RISING,"KEY4"},//KEY4	
/*	{IRQ_EINT(4),IRQ_TYPE_EDGE_RISING,"KEY5"},//KEY4	
	{IRQ_EINT(5),IRQ_TYPE_EDGE_RISING,"KEY6"},//KEY4	
	{IRQ_EINT(19),IRQ_TYPE_EDGE_FALLING,"KEY7"},//KEY4	
	{IRQ_EINT(20),IRQ_TYPE_EDGE_FALLING,"KEY8"},//KEY4	
*/	
};

//ÉùÃ÷Ò»¸ö°´¼üµÄµÈ´ı¶ÓÁĞ
static DECLARE_WAIT_QUEUE_HEAD(buttons_waitq);
//Ö¸Ê¾ÊÇ·ñÓĞ°´¼ü±»°´ÏÂ
static volatile int ev_press = 0;
//°´¼üÉè±¸µÄÖ÷Éè±¸ºÅ
static int buttons_major = 0;
//Éè±¸ºÅ
dev_t dev;
//×Ö·ûÉè±¸
struct cdev * buttons_cdev;
//°´ÏÂ´ÎÊı
static volatile int press_cnt[]={0,0,0,0};

//ÖĞ¶Ï´¦Àí³ÌĞò£¬¼ÇÂ¼°´¼ü°´ÏÂµÄ´ÎÊı£¬²¢ÖÃ±êÖ¾Î»Îª1£¬»½ĞÑµÈ´ı¶ÓÁĞÉÏµÈ´ıµÄ½ø³Ì
static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{
	volatile int *press_cnt = (volatile int *)dev_id;

	*press_cnt = *press_cnt + 1;//°´¼ü¼ÆÊıÖµ¼Ó1
	ev_press = 1;//ÉèÖÃ±êÖ¾Î»
	wake_up_interruptible(&buttons_waitq);//»½ĞÑµÈ´ı¶ÓÁĞ

	return IRQ_RETVAL(IRQ_HANDLED);
}
//Éè±¸´ò¿ª²Ù×÷£¬Ö÷ÒªÍê³ÉBUTTONSËù¶ÔÓ¦µÄGPIOµÄ³õÊ¼»¯£¬×¢²áÓÃ»§ÖĞ¶Ï´¦Àíº¯Êı
int  buttons_open(struct inode *inode,struct file *filp)
{
	int i;
	int err;
	unsigned val;

	/*ÉèÖÃbuttons¶ÔÓ¦µÄGPIO¹Ü½Å*/
	val = ioread32(S3C64XX_GPNCON);
	val = (val & ~(0xFF)) | (0xaa);//ÉèÖÃGPIO 0¡«5ÎªExt interrupt[0~3]Êä³ö
	iowrite32(val,S3C64XX_GPNCON);
/*
	val = ioread32(S3C64XX_GPLCON1);
	val = (val & ~(0xFF<<12)) | (0x33);
	iowrite32(val,S3C64XX_GPLCON1);
*/
	/*×¢²áÖĞ¶Ï´¦ÀíÀı³Ìbuttons_interrupt*/
	for(i=0;i<sizeof(buttons_irqs)/sizeof(buttons_irqs[0]);++i){
		err = request_irq(buttons_irqs[i].irq,buttons_interrupt,buttons_irqs[i].flags,buttons_irqs[i].name,(void*)&press_cnt[i]);
		if(err)
			break;
	}
	if(err){
		printk("buttons_open functions err.\n");
		i--;
		for(;i>=0;--i)
			free_irq(buttons_irqs[i].irq,(void*)&press_cnt[i]);
		return -EBUSY;
	}
	return 0;
}
//°´¼ü¶ÁÈôÃ»ÓĞ¼ü±»°´ÏÂ£¬ÔòÊ¹½ø³ÌĞİÃß£»ÈôÓĞ°´¼ü±»°´ÏÂ£¬Ôò¿½±´Êı¾İµ½ÓÃ»§¿Õ¼ä£¬È»ºóÇåÁã;Ê¹ÓÃ×èÈû¶ÁµÄ·½·¨
int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned long err;
	wait_event_interruptible(buttons_waitq,ev_press);//Èç¹ûev_press==0,Ôò½ø³ÌÔÚ¶ÓÁĞbuttons_waitq¶ÓÁĞÉÏĞİÃß£¬Ö±µ½ev_press==1
	ev_press = 0;//´ËÊ±ev_press==1,Çå³ıev_press
	err = copy_to_user(buf,(const void *)press_cnt,min(sizeof(press_cnt),len));//½«press_cntµÄÖµ¿½±´µ½ÓÃ»§¿Õ¼ä
	memset((void*)press_cnt,0,sizeof(press_cnt));//åˆå§‹åŒ–press_cntä¸º0
	return err ? -EFAULT : 0;

}
//Ö÷ÒªÊÇĞ¶ÔØÓÃ»§ÖĞ¶Ï´¦Àí³ÌĞò
int buttons_close(struct inode *inode,struct file *filp)
{
	int i;
	for(i=0;i<sizeof(buttons_irqs)/sizeof(buttons_irqs[0]);++i)
		free_irq(buttons_irqs[i].irq,(void*)&press_cnt);
	return 0;
}


static struct file_operations buttons_fops = {
	.owner = THIS_MODULE,
	.read = buttons_read,
	.release = buttons_close,
	.open = buttons_open,
};
/*
	Ä£¿é³õÊ¼»¯£º
		1.ÉêÇëÉè±¸ºÅ£¬Ä¬ÈÏÊ¹ÓÃ¶¯Ì¬·ÖÅäµÄ·½·¨
		2.ÉêÇë²¢³õÊ¼»¯cdev½á¹¹
		3.½«cdev×¢²áµ½ÄÚºË
*/

static int module_buttons_init(void)
{
	
	int result;
	printk("Tiny6410 buttons module init.\n");	
	if(buttons_major){
		dev = MKDEV(buttons_major,0);
		result = register_chrdev_region(dev,1,"buttons");//Í¨ÖªÊ¹ÓÃÉè±¸ºÅdev
	}else{
		result = alloc_chrdev_region(&dev,0,1,"buttons");//ÓÉÏµÍ³×Ô¶¯·ÖÅäÉè±¸ºÅ£¬Ä¬ÈÏ·½Ê½
		buttons_major = MAJOR(dev);
	}
	if(result < 0){
		printk(KERN_WARNING "buttons : can't get major %d\n",buttons_major);
	}

	printk("buttons major is %d",buttons_major);
	buttons_cdev = cdev_alloc();//¶¯Ì¬ÉêÇëcdev½á¹¹Ìå£¬Õâ¸öº¯ÊıÖ÷ÒªÊ¹ÓÃkzmallocÉêÇëÄÚ´æ´æ·Åcdev½á¹¹Ìå£¬²¢³õÊ¼»¯kobjectÁ´±í
	buttons_cdev ->ops = &buttons_fops;//ÉèÖÃfops
	cdev_init(buttons_cdev,&buttons_fops);//³õÊ¼»¯cdev½á¹¹Ìå£¬Ö÷ÒªÊÇ³õÊ¼»¯fops×Ö¶Î
	cdev_add(buttons_cdev,dev,1);//×¢²á¸Ã½á¹¹Ìåµ½ÄÚºË,Ö÷ÒªÓĞÉè±¸ºÅ,cdev½á¹¹ÌåÖ¸Õë,¸öÊı
	printk("buttons add ok.\n");
	return 0;
}

static void module_buttons_exit(void)
{
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
