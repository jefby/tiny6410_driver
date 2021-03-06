

//参考drivers/mtd/s3c2410.c或者/drivers/mtd/nand/at91_nand.c
#include <linux/module.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>


static struct nand_chip * s3c_nandchip = NULL;
static struct mtd_info * s3c_mtdinfo = NULL;


//nand_chip提供的选中芯片函数Xm0CSn2
static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if(chip == -1){
		//取消选中,NFCONT bit1设置为0
	}else{

		//NFCONT的bit1设置为1
	}
}
static void  s3c_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE){
		//发命令,NFCMMD = data
	}else{

		//发地址,NFADDR = data
	}
}
int s3c_nand_dev_ready(struct mtd_info *mtd)
{
	return "NFSTAT的bit0";
}

static int samsung_nand_init(void)
{
	//1.分配一个nand_chip结构体
	s3c_nandchip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(s3c_nandchip==NULL){
		printk("kzalloc nand_chip err.\n");
		return -1;
	}

	//2.设置nand_chip给nand_scan函数使用的.
	//2.1需要提供� 选中-发命令-地址-数据功能
	s3c_nandchip->select_chip 	= s3c_nand_select_chip;
	s3c_nandchip->cmd_ctrl 		= s3c_nand_cmd_ctrl;
	s3c_nandchip->IO_ADDR_R		= "NFDATA的虚拟地址";
	s3c_nandchip->IO_ADDR_W 	= "NFDATA的虚拟地址";
	s3c_nandchip->dev_ready		= ;
	
	
	//3.硬件相关的设置
	//4.使用:nand_scan
	s3c_mtdinfo = kzalloc(sizeof(struct mtd_info),GFP_KERNEL);
	if(!s3c_mtdinfo){
		printk("kzalloc s3c_mtdinfo err.\n");
		return -1;
	}

	s3c_mtdinfo->owner = THIS_MODULE;
	s3c_mtdinfo->priv = s3c_nandchip;
	
	nand_scan(s3c_mtdinfo,1);
	//5.add_mtd_partions
	add_mtd_partitions(struct mtd_info * master,const struct mtd_partition * parts,int nbparts);
	
	return 0;
}

static void samsung_nand_exit(void)
{

}

module_init(samsung_nand_init);
module_exit(samsung_nand_exit);

MODULE_LICENSE("Dual BSD/GPL");








