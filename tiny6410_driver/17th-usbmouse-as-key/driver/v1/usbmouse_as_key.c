/*

	搭建USB设备驱动程序框架,最终目的是要写一个USB设备驱动程序,将一个USB鼠标当做按键来使用.左击产生"L",右击产生"s",中键点击产生"enter"
	Author:jefby
	Email:jef199006@gmail.com

*/
#include <linux/module.h>
#include <linux/init.h>

#include <linux/usb/input.h>
#include <linux/hid.h>

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);//找到USB设备描述符
	
	printk("found usbmouse.\n");
	printk("manufacture is %s,produce id is %s.\n",dev->manufacturer,dev->product);//打印出USB设备厂商ID和产品ID号
	return 0;
}


static void usb_mouse_disconnect(struct usb_interface *intf)
{
	printk("usbmouse disconnect.\n");
}


static struct usb_device_id usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};
//usb_driver结构体
static struct usb_driver usb_mouse_as_key_driver = {
	.name		= "usbmouse_as_key",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};

//入口函数
static int usbmouse_as_key_init(void)
{
	usb_register(&usb_mouse_as_key_driver);
	return 0;
}
//出口函数
static void usbmouse_as_key_exit(void)
{
	usb_deregister(&usb_mouse_as_key_driver);
}

module_init(usbmouse_as_key_init);
module_exit(usbmouse_as_key_exit);

MODULE_LICENSE("Dual BSD/GPL");










