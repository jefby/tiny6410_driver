/*
 * 参考 drivers\net\cs89x0.c
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct net_device *vnet_dev;
static unsigned long cnt = 0;

static netdev_tx_t virtnet_xmit(struct sk_buff *skb, struct net_device *dev)
{
	printk("send %d packet.\n",cnt++);
	dev_kfree_skb(skb);
	//
	//dev->stats.tx_packets++;
	//dev->stats.tx_bytes += skb->len;
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;
	return NETDEV_TX_OK;
}

static int virtnet_dev_init(struct net_device *dev)
{
	return 0;
}

static const struct net_device_ops dummy_netdev_ops = {
	.ndo_init		= virtnet_dev_init,
	.ndo_start_xmit		= virtnet_xmit,

};

static void virtnet_setup(struct net_device *dev)
{
	ether_setup(dev);

	/* Initialize the device structure. */
	dev->netdev_ops = &dummy_netdev_ops;
	//dev->destructor = dummy_dev_free;

	/* Fill in device structure with ethernet-generic values. */
	//dev->tx_queue_len = 0;
	//dev->flags |= IFF_NOARP;
	//dev->flags &= ~IFF_MULTICAST;
	//dev->features	|= NETIF_F_NO_CSUM ;
	//random_ether_addr(dev->dev_addr);
}
static int virt_net_init(void)
{
	int err = 0;
	/* 1. 分配一个net_device结构体 */
	vnet_dev=alloc_netdev(0,"vnet%d",virtnet_setup);
	if(vnet_dev){
		printk("alloc_netdev ok.\n");
	}
	/* 2. 设置 */

	/* 3. 注册 */
	//register_netdevice(vnet_dev);
	err = register_netdev(vnet_dev);
	if(err)
		goto out;
	return 0;
out:
	free_netdev(vnet_dev);
	return -1;
}

static void virt_net_exit(void)
{
	unregister_netdev(vnet_dev);
	free_netdev(vnet_dev);
}

module_init(virt_net_init);
module_exit(virt_net_exit);

MODULE_AUTHOR("thisway.diy@163.com,17653039@qq.com");
MODULE_LICENSE("GPL");


