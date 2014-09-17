

//参考drivers/mtd/s3c2410.c /drivers/mtd/nand/at91_nand.c
//NAND芯片K9GAG08U0E.pdf,本驱动程序因为6410的NAND FLASH只支持8-bitECC，而芯片要求24-bit的ECC所以无法使用
//使用软件ECC证明是不可以的,软件ECC和硬件ECC计算出来的值不匹配，所以存在错误；只好不使用ECC，但是因为NAND FLASH存在位反转现象，
//这样是不太好的.以后有时间和精力再改进


#include <linux/module.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/clk.h>
//nand controller寄存器定义
struct s3c_nand_regstruct{
	unsigned long		   nfconf		 ;	
	unsigned long		   nfcont		 ;	
	unsigned long		   nfcmmd		 ;	
	unsigned long		   nfaddr		 ;	
	unsigned long		   nfdata		 ;	
	unsigned long		   nfmeccd0 	 ;	
	unsigned long		   nfmeccd1 	 ;	
	unsigned long		   nfseccd		 ;	
	unsigned long		   nfsblk		 ;	
	unsigned long		   nfeblk		 ;	
	unsigned long		   nfstat		 ;	
	unsigned long		   nfeccerr0	 ;	
	unsigned long		   nfeccerr1	 ;	
	unsigned long		   nfmecc0		 ;	
	unsigned long		   nfmecc1		 ;	
	unsigned long		   nfsecc		 ;	
	unsigned long		   nfmlcbitpt	 ;	
	unsigned long		   nf8eccerr0	 ;	
	unsigned long		   nf8eccerr1	 ;	
	unsigned long		   nf8eccerr2	 ;	
	unsigned long		   nfm8ecc0 	 ;	
	unsigned long		   nfm8ecc1 	 ;	
	unsigned long		   nfm8ecc2 	 ;	
	unsigned long		   nfm8ecc3 	 ;	
	unsigned long		   nfmlc8bitpt	 ;	

};

static struct nand_chip * s3c_nandchip = NULL;
static struct mtd_info * s3c_mtdinfo = NULL;
static struct s3c_nand_regstruct * s3c_nand_reg_ptr = NULL;
static struct clk * clk = NULL;

//nand_chip芯片选中函数.XmnCS2
static void s3c_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if(chip == -1){
		//取消选中: BIT1 为1
		s3c_nand_reg_ptr->nfcont |= (0x1<<1);
		
	}else{
		//选中: BIT1 为0
		s3c_nand_reg_ptr->nfcont &= ~(0x1<<1);
	}
}
static void  s3c_nand_cmd_ctrl(struct mtd_info *mtd, int dat, unsigned int ctrl)
{
	if (dat == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE){
		//命令 NFCMMD = data
		s3c_nand_reg_ptr->nfcmmd = dat;
	}else if(ctrl & NAND_ALE){
		//地址,NFADDR = data
		s3c_nand_reg_ptr->nfaddr = dat;
	}
}
int s3c_nand_dev_ready(struct mtd_info *mtd)
{
	//return "NFSTAT??bit0";
	return ( s3c_nand_reg_ptr->nfstat & (0x1<<0) );
}

static int samsung_nand_init(void)
{

	//1.分配一个nand_chip结构体
	s3c_nandchip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(s3c_nandchip==NULL){
		printk("kzalloc nand_chip err.\n");
		return -1;
	}
	//映射NAND FLASH Controller到虚拟内存,得到虚拟地址	s3c_nand_reg_ptr
	s3c_nand_reg_ptr = ioremap(0x70200000,sizeof(struct s3c_nand_regstruct));

	if(!s3c_nand_reg_ptr){
		printk("s3c_nand_reg_ptr ioremap err.\n");
		return -1;
	}else{
		printk("s3c_nand_reg_ptr=%p",s3c_nand_reg_ptr);
	}
	

	//2.设置
	s3c_nandchip->select_chip 	= s3c_nand_select_chip;//片选函数
	s3c_nandchip->cmd_ctrl 		= s3c_nand_cmd_ctrl;//发命令函数

	s3c_nandchip->IO_ADDR_R		= &s3c_nand_reg_ptr->nfdata;//读地址
	s3c_nandchip->IO_ADDR_W 	= &s3c_nand_reg_ptr->nfdata;//写地址
	s3c_nandchip->dev_ready		= s3c_nand_dev_ready;//检查设备是否忙
	
	s3c_nandchip->ecc.mode = NAND_ECC_NONE;//不使用硬件或者软件ECC,证明这样是可以的
	//3.硬件相关设置

	clk = clk_get(NULL,"nand");
	clk_enable(clk);//使能时钟
	
	//NAND FLASH硬件设置
	//HCLK=133MHZ,时间 = 7.5ns
	//TACLS:CLE和ALE芯片发出多长时间后可以发出nWE信号,由NAND FLASH芯片手册可以可以同时发送,TACLS=0
	//TWRPH0:nWE保持的时间 HCLKx(TWRPH0+1),由芯片手册可知>=15ns,????TWRPH0=1
	//TWRPH1:nWE变为高电平后维持的时间 HCLKX(TWRPH1+1)由芯片手册可知>==5ns,TWRPH1=0
#define TACLS 0
#define TWRPH0 2
#define TWRPH1 0
	s3c_nand_reg_ptr->nfconf = (TACLS<<12) | (TWRPH0 << 8) | (TWRPH1 << 4);

	//NFCONT
	//BIT1 : 取消NAND FLASH片选
	//BIT0 : 使能NAND FLASH控制器
	s3c_nand_reg_ptr->nfcont = (0x1<<1) | (0x1<<0);

	
	//4.使用nand_scan函数
	s3c_mtdinfo = kzalloc(sizeof(struct mtd_info),GFP_KERNEL);
	if(!s3c_mtdinfo){
		printk("kzalloc s3c_mtdinfo err.\n");
		return -1;
	}

	s3c_mtdinfo->owner = THIS_MODULE;
	s3c_mtdinfo->priv = s3c_nandchip;
	
	nand_scan(s3c_mtdinfo,1);//查找NAND FLASH设备，如果找到会输出
	//5.add_mtd_partions
	//add_mtd_partitions(struct mtd_info * master,const struct mtd_partition * parts,int nbparts);
	
	return 0;
}

static void samsung_nand_exit(void)
{
	iounmap(s3c_nand_reg_ptr);
	kzfree(s3c_nandchip);
	kzfree(s3c_mtdinfo);
	
}

module_init(samsung_nand_init);
module_exit(samsung_nand_exit);

MODULE_LICENSE("Dual BSD/GPL");




