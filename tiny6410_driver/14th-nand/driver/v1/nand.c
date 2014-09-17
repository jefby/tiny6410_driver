

//²Î¿¼drivers/mtd/s3c2410.c»òÕß/drivers/mtd/nand/at91_nand.c
#include <linux/module.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>


static struct nand_chip * s3c_nandchip = NULL;
static struct mtd_info * s3c_mtdinfo = NULL;


//nand_chipÌá¹©µÄÑ¡ÖÐÐ¾Æ¬º¯ÊýXm0CSn2
static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if(chip == -1){
		//È¡ÏûÑ¡ÖÐ,NFCONT bit1ÉèÖÃÎª0
	}else{

		//NFCONTµÄbit1ÉèÖÃÎª1
	}
}
static void  s3c_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE){
		//·¢ÃüÁî,NFCMMD = data
	}else{

		//·¢µØÖ·,NFADDR = data
	}
}
int s3c_nand_dev_ready(struct mtd_info *mtd)
{
	return "NFSTATµÄbit0";
}

static int samsung_nand_init(void)
{
	//1.·ÖÅäÒ»¸önand_chip½á¹¹Ìå
	s3c_nandchip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(s3c_nandchip==NULL){
		printk("kzalloc nand_chip err.\n");
		return -1;
	}

	//2.ÉèÖÃnand_chip¸ønand_scanº¯ÊýÊ¹ÓÃµÄ.
	//2.1ÐèÒªÌá¹©å Ñ¡ÖÐ-·¢ÃüÁî-µØÖ·-Êý¾Ý¹¦ÄÜ
	s3c_nandchip->select_chip 	= s3c_nand_select_chip;
	s3c_nandchip->cmd_ctrl 		= s3c_nand_cmd_ctrl;
	s3c_nandchip->IO_ADDR_R		= "NFDATAµÄÐéÄâµØÖ·";
	s3c_nandchip->IO_ADDR_W 	= "NFDATAµÄÐéÄâµØÖ·";
	s3c_nandchip->dev_ready		= ;
	
	
	//3.Ó²¼þÏà¹ØµÄÉèÖÃ
	//4.Ê¹ÓÃ:nand_scan
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








