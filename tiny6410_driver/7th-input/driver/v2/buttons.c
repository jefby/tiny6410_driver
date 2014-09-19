
#include <linux/module.h>//MODULE_AUTHOR,MODULE_LICENSE
#include <linux/init.h>//module_init/exit

#include <linux/fs.h>//file_operations
#include <asm/io.h>//ioread32,iowrite32
#include <linux/cdev.h>//cdev
#include <mach/map.h>//S3C64XX_VA_GPIO
#include <mach/regs-gpio.h>//
#include <mach/gpio-bank-n.h>//GPNCON
#include <mach/gpio-bank-l.h>//GPLCON
#include <linux/wait.h>//wait_event_interruptible(wait_queue_head_t q,int condition);//wake_up_interruptible(struct wait_queue **q)
#include <linux/sched.h>//request_irq,free_irq
#include <asm/uaccess.h>//copy_to_user
#include <linux/irq.h>//IRQ_TYPE_EDGE_FALLING
#include <linux/interrupt.h>//request_irq,free_irq
#include <linux/device.h>//class device
#include <linux/input.h>//input_dev


struct button_irq_desc {  
	int irq;   //中断号
	int index;//指示几号按键
	int key_val; //按键值
	char *name;	//名称
};


//定时器结构体
static struct timer_list buttons_timer;
//用于保存中断处理函数中使用的devid值
static struct button_irq_desc * buttons_devid = NULL;

static struct button_irq_desc button_irqs [] = 
{    
	{IRQ_EINT( 0), 0,KEY_L, "KEY0"},    
	{IRQ_EINT( 1), 1,KEY_S, "KEY1"},   
	{IRQ_EINT( 2), 2,KEY_ENTER, "KEY2"},    
	{IRQ_EINT( 3), 3,KEY_LEFTSHIFT, "KEY3"},    
	{IRQ_EINT( 4), 4,KEY_D, "KEY4"},    
	{IRQ_EINT( 5), 5,KEY_A, "KEY5"},    
	{IRQ_EINT(19), 6,KEY_T, "KEY6"},    
	{IRQ_EINT(20), 7,KEY_E, "KEY7"},
};

static struct input_dev * buttons_dev = NULL;

//按键的中断处理函数
static irqreturn_t buttons_interrupt(int irq,void *dev_id)
{		
	buttons_devid = (struct button_irq_desc *)dev_id;	
	mod_timer(&buttons_timer,jiffies + HZ/100);//10ms后启动定时器
	return IRQ_RETVAL(IRQ_HANDLED);
}

//定时器处理函数
void buttons_timer_function(unsigned long data)
{
	volatile int cmp = 0;
	volatile unsigned long rval = 0;
	volatile int key_val = 0;
	if(!buttons_devid)//若没有中断，即没有按键被按下
		return;
	//上报事件即可
	cmp = buttons_devid->index;
	key_val = buttons_devid->key_val;
	if(cmp >=0 && cmp < 6 )//KEY0~KEY5
	{
		rval = readl(S3C64XX_GPNDAT);
		if(rval & (0x1<<cmp))//松开
			input_event(buttons_dev,EV_KEY, key_val, 0);
		else//按下
			input_event(buttons_dev,EV_KEY, key_val, 1);
	}else{//KEY6,7
		rval = readl(S3C64XX_GPLDAT);
		if(rval & (0x1<<(cmp-6+11)))//松开
			input_event(buttons_dev,EV_KEY, key_val, 0);
		else//按下
			input_event(buttons_dev,EV_KEY, key_val, 1);
	}
}
static int input_buttons_init(void)
{
	int err = 0;
	int i = 0;
	//1.分配一个input_dev结构体
	buttons_dev = input_allocate_device();
	if(buttons_dev == NULL){
		printk(KERN_ALERT "input_allocate_device err.\n");
		return -1;
	}
	//2.设置
	//2.1能产生哪一类事件
	set_bit(EV_KEY,buttons_dev->evbit);
	//2.2能产生这类操作里的那些事件
	//产生L,S,ENTER,LEFTSHIFT,D,A,T,E
	set_bit(KEY_L,buttons_dev->keybit);
	set_bit(KEY_S,buttons_dev->keybit);
	set_bit(KEY_ENTER,buttons_dev->keybit);
	set_bit(KEY_LEFTSHIFT,buttons_dev->keybit);
	set_bit(KEY_D,buttons_dev->keybit);
	set_bit(KEY_A,buttons_dev->keybit);
	set_bit(KEY_T,buttons_dev->keybit);
	set_bit(KEY_E,buttons_dev->keybit);

	
	//3.注册
	input_register_device( buttons_dev);
	//4.硬件相关的操作
	//4.1 申请中断号,并注册中断
    for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
		if (button_irqs[i].irq < 0) {//检查传入的irq号是否正确
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
	//4.2分配并初始化定时器
	init_timer(&buttons_timer);//初始化定时器
	buttons_timer.function = buttons_timer_function;//设置定时器到期处理函数
	//buttons_timer.expires = 0;
	add_timer(&buttons_timer);//启动定时器
	
	printk(KERN_ALERT "input_buttons_init\n");
	return 0;
}

static void input_buttons_exit(void)
{
	int i = 0;
	input_unregister_device(buttons_dev);
	for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) 
	{
		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
	}
	del_timer(&buttons_timer);//删除定时器
	input_free_device(buttons_dev);//释放掉分配的空间
	
	printk(KERN_ALERT "input_buttons_exit\n");
}

module_init(input_buttons_init);//入口
module_exit(input_buttons_exit);//出口


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jefby");
MODULE_DESCRIPTION("Tiny6410 key driver with input subsystem.");



















