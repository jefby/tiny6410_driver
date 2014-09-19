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
#include <linux/ip.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct net_device *vnet_dev;
static unsigned long cnt = 0;
static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
	/* 参考LDD3 */
	unsigned char *type;
	struct iphdr *ih;//ip报头
	__be32 *saddr, *daddr, tmp;//使用大端
	unsigned char	tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr;//以太网帧头
	
	struct sk_buff *rx_skb;//套接字缓冲区
		
	// 从硬件读出/保存数据
	/* 对调"源/目的"的mac地址 */
	ethhdr = (struct ethhdr *)skb->data;
	memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
	memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);

	/* 对调"源/目的"的ip地址 */    
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;
	
	//((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
	//((u8 *)daddr)[2] ^= 1;
	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
	//printk("tx package type = %02x\n", *type);
	// 修改类型, 原来0x8表示ping
	*type = 0; /* 0表示reply */
	
	ih->check = 0;		   /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
	
	// 假装接收到了一个包,然后构造一个sk_buff
	rx_skb = dev_alloc_skb(skb->len + 2);
	skb_reserve(rx_skb, 2); /* ether 帧头是14个字节,故需要保留2个字节用于对齐.align IP on 16B boundary */	
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

	/* Write metadata, and then pass to the receive level */
	rx_skb->dev = dev;
	rx_skb->protocol = eth_type_trans(rx_skb, dev);
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	dev->stats.rx_packets++;//增加统计数目
	dev->stats.rx_bytes += skb->len;

	// 向IP层提交sk_buff
	netif_rx(rx_skb);
}

static netdev_tx_t virtnet_xmit(struct sk_buff *skb, struct net_device *dev)
{
	printk("send %d packet.\n",cnt++);
	//对于真实的网卡，把skb里的数据通过网卡发送出去
	netif_stop_queue(dev);//停止该网卡发送队列
	//....................//把skb的数据写入网卡

	emulator_rx_packet(skb,dev);//构造一个假包发出去

	
	dev_kfree_skb(skb);//释放skb
	netif_wake_queue(dev);//数据全部发送出去后，唤醒网卡队列
	
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
	ether_setup(dev);//设置ether设备

	/* Initialize the device structure. */
	dev->netdev_ops = &dummy_netdev_ops;
	//dev->destructor = dummy_dev_free;

	/* Fill in device structure with ethernet-generic values. */
	//dev->tx_queue_len = 0;
	dev->flags |= IFF_NOARP;//不支持ARP协议
	//dev->flags &= ~IFF_MULTICAST;
	dev->features	|= NETIF_F_NO_CSUM ;
	random_ether_addr(dev->dev_addr);
}
static int virt_net_init(void)
{
	int err = 0;
	/* 1. 分配一个net_device结构体 */
	vnet_dev=alloc_netdev(0,"vnet%d",virtnet_setup);
	if(vnet_dev){
		printk("alloc_netdev ok.\n");
		return -1;
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


