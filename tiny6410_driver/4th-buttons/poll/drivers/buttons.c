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
#include <linux/device.h>//class device
#include <linux/poll.h>//POLLIN | POLLRDNORM

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with interrupt");


struct button_irq_desc {
    int irq;
    int number;
    char *name;	
};

/*
static volatile unsigned long* gpncon = NULL;
static volatile unsigned long* gpndat = NULL;
static volatile unsigned long* gplcon = NULL;
static volatile unsigned long* gpldat = NULL;
*/
static struct button_irq_desc button_irqs [] = {
    {IRQ_EINT( 0), 0, "KEY0"},
    {IRQ_EINT( 1), 1, "KEY1"},
    {IRQ_EINT( 2), 2, "KEY2"},
    {IRQ_EINT( 3), 3, "KEY3"},
    {IRQ_EINT( 4), 4, "KEY4"},
    {IRQ_EINT( 5), 5, "KEY5"},
    {IRQ_EINT(19), 6, "KEY6"},
    {IRQ_EINT(20), 7, "KEY7"},
};
//ÉùÃ÷Ò»¸ö°´¼üµÄµÈ´ı¶ÓÁĞ
static DECLARE_WAIT_QUEUE_HEAD(buttons_waitq);
//Ö¸Ê¾ÊÇ·ñÓĞ°´¼ü±»°´ÏÂ£¬ÔÚÖĞ¶Ï´¦Àí³ÌĞòÖĞÖÃÎª£¬read³ÌĞò½«ÆäÇå0
static volatile int ev_press = 0;
//°´¼üÉè±¸µÄÖ÷Éè±¸ºÅ
static int buttons_major = 0;
//Éè±¸ºÅ
dev_t dev;
//×Ö·ûÉè±¸
struct cdev * buttons_cdev=NULL;
static volatile unsigned char key_val = 0;
static struct class * tiny6410_buttons_class = NULL;
static struct device * tiny6410_buttons_device = NULL;

//ÖĞ¶Ï´¦Àí³ÌĞò£¬¼ÇÂ¼°´¼ü°´ÏÂµÄ´ÎÊı£¬²¢ÖÃ±êÖ¾Î»Îª1£¬»½ĞÑµÈ´ı¶ÓÁĞÉÏµÈ´ıµÄ½ø³Ì

static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{	
	struct button_irq_desc *temp = (struct button_irq_desc *)dev_id;
	key_val = (unsigned char)(temp->number+1);
	ev_press = 1;//±íÊ¾·¢ÉúÁËÖĞ¶Ï
	wake_up_interruptible(&buttons_waitq);
	return IRQ_RETVAL(IRQ_HANDLED);
}

//Éè±¸´ò¿ª²Ù×÷£¬Ö÷ÒªÍê³ÉBUTTONSËù¶ÔÓ¦µÄGPIOµÄ³õÊ¼»¯£¬×¢²áÓÃ»§ÖĞ¶Ï´¦Àíº¯Êı,ÉèÖÃ´¥·¢·½Ê½ÎªË«±ßÑØ´¥·¢

int  buttons_open(struct inode *inode,struct file *filp)
{ 
    int i;
    int err = 0;
    //ÉêÇëÖĞ¶ÏºÅ

    for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
	if (button_irqs[i].irq < 0) {
		continue;
	}
        err = request_irq(button_irqs[i].irq, buttons_interrupt, IRQ_TYPE_EDGE_BOTH, 
                          button_irqs[i].name, (void *)&button_irqs[i]);
        if (err)
            break;
    }
    if (err) {
	    printk("err=%d.\n",err);
        i--;
        for (; i >= 0; i--) {
	    if (button_irqs[i].irq < 0) {
		continue;
	    }
	    disable_irq(button_irqs[i].irq);
            free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
        }
        return -EBUSY;
    }

    ev_press = 0;//³õÊ¼»¯
    
    return 0;
}
//°´¼ü¶ÁÈôÃ»ÓĞ¼ü±»°´ÏÂ£¬ÔòÊ¹½ø³ÌĞİÃß£»ÈôÓĞ°´¼ü±»°´ÏÂ£¬Ôò¿½±´Êı¾İµ½ÓÃ»§¿Õ¼ä£¬È»ºóÇåÁã
int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned long val = 0;
	if(len != sizeof(key_val))
		return -EINVAL;
	wait_event_interruptible(buttons_waitq,ev_press);//ev_press==0,ÔòĞİÃß
	ev_press = 0;//ÖØĞÂÇå0
	
	if((val = copy_to_user(buf,(const void*)&key_val,sizeof(key_val))) != sizeof(key_val))
		return -EINVAL;
	return sizeof(key_val);

}
//Ö÷ÒªÊÇĞ¶ÔØÓÃ»§ÖĞ¶Ï´¦Àí³ÌĞò

int buttons_close(struct inode *inode,struct file *filp)
{
	int i=0;
	printk("buttons close.\n");
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
		disable_irq(button_irqs[i].irq);
		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
	}
	return 0;
}
unsigned int buttons_poll (struct file *filp, struct poll_table_struct *tb)
{
	unsigned int mask = 0;
	poll_wait(filp,&buttons_waitq,tb);
	if(ev_press)
		mask |= POLLIN | POLLRDNORM;
	return mask;
}

static struct file_operations buttons_fops = {
	.owner = THIS_MODULE,
	.read = buttons_read,
	.release = buttons_close,
	.open = buttons_open,
	.poll = buttons_poll,
};
/*
	æ¨¡å—åˆå§‹åŒ–ï¼š
		1.ç”³è¯·è®¾å¤‡å·ï¼Œé»˜è®¤ä½¿ç”¨åŠ¨æ€åˆ†é…çš„æ–¹æ³•
		2.ç”³è¯·å¹¶åˆå§‹åŒ–cdevç»“æ„
		3.å°†cdevæ³¨å†Œåˆ°å†…æ ¸
*/
static int module_buttons_init(void)
{
	
	int result;
	int err=0;
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
/*
	//é…ç½®GPN,GPL GPIOä¸ºä¸­æ–­æ–¹å¼
	gpncon = (volatile unsigned long*)ioremap(GPNCON,16);
	gplcon = (volatile unsigned long*)ioremap(GPLCON,16);

	temp = ioread32(gpncon);
	temp &= ~(0xFFF);
	temp |= 0xaaa;//è®¾ç½®ä¸ºEINTæ–¹å¼
	iowrite32(temp,gpncon);
	
	temp = ioread32(gplcon+1);
	temp &= ~(0xFF<<12);
	temp |= (0x33<<12);
	iowrite32(temp,gplcon+1);

	gpndat = gpncon + 1;
	gpldat = gplcon + 2;
*/	
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
//	iounmap(gpncon);
//	iounmap(gplcon);
	device_destroy(tiny6410_buttons_class, MKDEV(buttons_major, 0));
	class_destroy(tiny6410_buttons_class);
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
