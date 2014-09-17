/*
	将USB鼠标当做按键来使用，使其发出L,S,和ENTER按键值
	还没有实现,只是将按键事件数据打印出来
	一般数据格式:
	第一个字节 
	0位表示鼠标左键是否被按下,0表示未按下,1表示按下
	1位表示鼠标右键是否被按下,0表示未按下,1表示按下
	2位表示鼠标中键是否被按下,0表示未按下,1表示按下
*/


#include <linux/module.h>
#include <linux/init.h>


#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/input.h>//input_dev
#include <linux/usb.h>

static struct input_dev * uk_dev = NULL;
static dma_addr_t uk_buf_phys ;//申请的usb缓冲区物理地址
static char * uk_buf = NULL;//申请的usb缓冲区虚拟地址
static int len = 0;
static struct urb * uk_urb= NULL;//usb请求块


//usb完成函数
void usbmouse_as_key_irq(struct urb * urb)
{
	int i=0 ;
	static int cnt = 0;
	printk("data cnt %d :",cnt++);
	for(i=0;i<len;++i){
		printk("%02x ",uk_buf[i]);
	}
	printk("\n");
	usb_submit_urb (urb, GFP_ATOMIC);
}

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);//获取USB设备描述符指针
	struct usb_host_interface *interface = NULL;
	struct usb_endpoint_descriptor *endpoint = NULL;//usb端口描述符
	int pipe =0;
	
	interface = intf->cur_altsetting;
	
	if (interface->desc.bNumEndpoints != 1)
			return -ENODEV;
	
	endpoint = &interface->endpoint[0].desc;//端点0用于控制传输类型和传输方向
	if (!usb_endpoint_is_int_in(endpoint))//如果不是中断传输且方向是IN,返回错误
			return -ENODEV;
	
	
	printk("found usbmouse.\n");
	printk("bcdUSB= %x\n",dev->descriptor.bcdUSB);
	printk("idProduct=%x\n",dev->descriptor.idProduct);
	printk("idVendor=%x\n",dev->descriptor.idVendor);

//1.分配一个input_device结构体
	uk_dev = input_allocate_device();//分配并初始化一个输入设备
	if(uk_dev == NULL){
		printk(KERN_ALERT "input_allocate_device err.\n");
		return -1;
	}
//2.设置
	//2.1 设置能产生按键类事件
	set_bit(EV_KEY,uk_dev->evbit);
	//2.2 设置能产生哪些按键
	set_bit(KEY_L,uk_dev->keybit);
	set_bit(KEY_S,uk_dev->keybit);
	set_bit(KEY_ENTER,uk_dev->keybit);

//3.注册设备到输入子系统中
	input_register_device(uk_dev);

//4.硬件相关的操作
	//数据传输的三要素:源,目的,长度
	//源-设备传输的某个端点
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);//端点和方向
	//长度
	len = endpoint->wMaxPacketSize;
	//目的:分配接收缓冲区
	uk_buf = usb_alloc_coherent(dev, len, GFP_ATOMIC, &uk_buf_phys);
	//使用三要素,urb(usb request block)
	//(1)分配一个urb
	uk_urb = usb_alloc_urb(0,GFP_KERNEL);
	//(2)设置urb
	usb_fill_int_urb(uk_urb,dev,pipe,uk_buf,len,usbmouse_as_key_irq,NULL,endpoint->bInterval);
	uk_urb->transfer_dma = uk_buf_phys;//要发送的DMA缓冲区地址
	uk_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP; //urb->transfer_dma valid on submit,应当被置位, 当 urb 包含一个要被发送的 DMA 缓冲. USB 核心使用这个被 transfer_dma 变量指向的缓冲, 不是被 transfer_buffer 变量指向的缓冲.

	//(3)使用urb
	usb_submit_urb(uk_urb, GFP_ATOMIC);//被提交给 USB 核心来发送出到 USB 设备
	
	return 0;
}


static void usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_device *dev = interface_to_usbdev(intf);

	usb_kill_urb(uk_urb);
	usb_free_urb(uk_urb);
	
	usb_free_coherent(dev,len,uk_buf,uk_buf_phys);

	input_unregister_device(uk_dev);
	input_free_device(uk_dev);
	
	printk("usbmouse disconnect.\n");
}


static struct usb_device_id usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

static struct usb_driver usb_mouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};


static int usbmouse_as_key_init(void)
{
	usb_register(&usb_mouse_as_key_driver);
	return 0;
}

static void usbmouse_as_key_exit(void)
{
	usb_deregister(&usb_mouse_as_key_driver);
}

module_init(usbmouse_as_key_init);
module_exit(usbmouse_as_key_exit);

MODULE_LICENSE("Dual BSD/GPL");










