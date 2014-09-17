/*对版本1的修改:包含了头文件usb.h*/
#include <linux/module.h>
#include <linux/init.h>


#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/usb.h>//interface_to_usbdev

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	
	printk("found usbmouse.\n");
	printk("bcdUSB= %x\n",dev->descriptor.bcdUSB);//bcdUSB域包含该描述符遵循的USB规范的版本号
	printk("idProduct=%x\n",dev->descriptor.idProduct);//产品ID
	printk("idVendor=%x\n",dev->descriptor.idVendor);//供应商ID
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










