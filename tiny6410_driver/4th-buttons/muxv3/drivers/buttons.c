/*
 *
 *	Tiny 6410µÄ°´¼üÖĞ¶ÏÇı¶¯
 *	Ôö¼ÓÁË·Ç×èÈû·ÃÎÊ.
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 */
#include <linux/module.h>//MODULE_LICENSE,MODULE_AUTHOR
#include <linux/init.h>//module_init/module_exit


#include <linux/fs.h>//file_operations
#include <asm/io.h>//ioread32,iowrite32
#include <linux/cdev.h>//cdev
#include <mach/map.h>//å®šä¹‰äº†S3C64XX_VA_GPIO
#include <mach/regs-gpio.h>//å®šä¹‰äº†gpio-bank-nä¸­ä½¿ç”¨çš„S3C64XX_GPN_BASE
#include <mach/gpio-bank-n.h>//å®šä¹‰äº†GPNCON
#include <mach/gpio-bank-l.h>//å®šä¹‰äº†GPNCON
#include <linux/wait.h>//wait_event_interruptible(wait_queue_head_t q,int condition);
//wake_up_interruptible(struct wait_queue **q)
#include <linux/sched.h>//request_irq,free_irq
#include <asm/uaccess.h>//copy_to_user
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq
#include <linux/device.h>//class device
#include <linux/poll.h>//POLLIN | POLLRDNORM
#include <linux/semaphore.h>//DECLARE_MUTEX(name)

MODULE_AUTHOR("jefby");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Tiny 6410 buttons with interrupt");

#define GPNCON 0x7F008830
#define GPNDAT 0x7F008834
#define GPLCON0 0x7F008810
#define GPLDAT 0x7F008818

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


static DECLARE_WAIT_QUEUE_HEAD(buttons_waitq);
static volatile int ev_press = 0;
static int buttons_major = 0;
dev_t dev;
struct cdev * buttons_cdev=NULL;

static volatile unsigned char key_val = 0;
static struct class * tiny6410_buttons_class = NULL;
static struct device * tiny6410_buttons_device = NULL;

struct fasync_struct * but_fasync = NULL;

static DEFINE_SEMAPHORE(buttons_lock);//¶¨ÒåĞÅºÅÁ¿,£¬²¢³õÊ¼»¯



//ÖĞ¶Ï´¦Àí³ÌĞò£¬¼ÇÂ¼°´¼ü°´ÏÂµÄ´ÎÊı£¬²¢ÖÃ±êÖ¾Î»Îª1£¬»½ĞÑµÈ´ı¶ÓÁĞÉÏµÈ´ıµÄ½ø³Ì
static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{	
	struct button_irq_desc *temp = (struct button_irq_desc *)dev_id;
	key_val = (unsigned char)(temp->number+1);
	ev_press = 1;//±íÊ¾ÖĞ¶Ï·¢ÉúÁË
	wake_up_interruptible(&buttons_waitq);
	//µ±ÓĞ°´¼ü±»°´ÏÂÊ±Çı¶¯³ÌĞòÍ¨ÖªÓ¦ÓÃ³ÌĞò
	kill_fasync(&but_fasync,SIGIO, POLL_IN);
	return IRQ_RETVAL(IRQ_HANDLED);
}

//Éè±¸´ò¿ª²Ù×÷£¬Ö÷ÒªÍê³ÉBUTTONSËù¶ÔÓ¦µÄGPIOµÄ³õÊ¼»¯£¬×¢²áÓÃ»§ÖĞ¶Ï´¦Àíº¯Êı,ÉèÖÃ´¥·¢·½Ê½ÎªË«±ßÑØ´¥·¢

int  buttons_open(struct inode *inode,struct file *filp)
{ 
    int i;
    int err = 0;
   //use atomic
    if(filp->f_flags & O_NONBLOCK)//·Ç×èÈû·ÃÎÊ 
     {
	if(down_trylock(&buttons_lock))//ÔÚµ÷ÓÃÊ±²»¿É»ñµÃ£¬ÔòÁ¢¼´·µ»ØÒ»¸ö·ÇÁãÖµ
		return -EBUSY;	
    }else{//×èÈû·ÃÎÊ
	if(down_interruptible(&buttons_lock))//·µ»Ø·ÇÁãÖµ£¬±íÊ¾²Ù×÷±»ÖĞ¶Ï;·ñÔò¼ì²éĞÅºÅÊÇ·ñ¿ÉÓÃ
		return -ERESTARTSYS;
    }
	
 	
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

    ev_press = 0;//ÉèÖÃ³õÊ¼×´Ì¬Îª0
    
    return 0;
}
//°´¼ü¶ÁÈôÃ»ÓĞ¼ü±»°´ÏÂ£¬ÔòÊ¹½ø³ÌĞİÃß£»ÈôÓĞ°´¼ü±»°´ÏÂ£¬Ôò¿½±´Êı¾İµ½ÓÃ»§¿Õ¼ä£¬È»ºóÇåÁã

int buttons_read(struct file *filp, char __user *buf, size_t len, loff_t * pos)
{
	unsigned long val = 0;
	if(len != sizeof(key_val))
		return -EINVAL;
	if(filp->f_flags & O_NONBLOCK)//·Ç×èÈû·ÃÎÊ 
    	{
		if(!ev_press)
			return -EAGAIN;//±íÊ¾°´¼üÎ´°´ÏÂ£¬¼ÌĞøÖ´ĞĞ
    	}else{//×èÈû·ÃÎÊ
		wait_event_interruptible(buttons_waitq,ev_press);//ev_press==0,ÔòĞİÃß
	}
	ev_press = 0;//ÖØĞÂÇå0
	
	if((val = copy_to_user(buf,(const void*)&key_val,sizeof(key_val))) != sizeof(key_val))
		return -EINVAL;
	return 1;

}
//Ö÷ÒªÊÇĞ¶ÔØÓÃ»§ÖĞ¶Ï´¦Àí³ÌĞò

int buttons_close(struct inode *inode,struct file *filp)
{
	int i=0;
	up(&buttons_lock);//é‡Šæ”¾ä¿¡å·é‡
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
int buttons_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &but_fasync);
}
static struct file_operations buttons_fops = {
	.owner = THIS_MODULE,
	.read = buttons_read,
	.release = buttons_close,
	.open = buttons_open,
	.poll = buttons_poll,
	.fasync = buttons_fasync,
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

	device_destroy(tiny6410_buttons_class, MKDEV(buttons_major, 0));
	class_destroy(tiny6410_buttons_class);
	cdev_del(buttons_cdev);
	unregister_chrdev_region(dev,1);
	printk("Tiny6410 buttons module exit");
}

module_init(module_buttons_init);
module_exit(module_buttons_exit);
