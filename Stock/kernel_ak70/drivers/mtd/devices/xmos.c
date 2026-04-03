#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mtd/mtd.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>

#include <mach/gpio.h>
#include <mach/ak_gpio_def.h>


#if (AK_HAVE_XMOS==1)

#include "xmosdata.h"

#define MTD_DEVICE		"XMOS-boot"
#define BUFF_SZ			1024

#define COMP_SZ			1024	

#define UPGRADE_SECTOR	1

#define UPGRADE_TRY_CNT	4

#define UPDATER			"XMOS Updater"

#define DESCRIPTOR_STRING_ATTR(field, buffer)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return sprintf(buf, "%s", buffer);				\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	if (size >= sizeof(buffer))					\
		return -EINVAL;						\
	return strlcpy(buffer, buf, sizeof(buffer));			\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

static char sflash_success_string[256];
static char sflash_probing_cnt_string[256];
static char sflash_sflash_id_string[256];

DESCRIPTOR_STRING_ATTR(success, sflash_success_string)
DESCRIPTOR_STRING_ATTR(probing_cnt, sflash_probing_cnt_string)
DESCRIPTOR_STRING_ATTR(sflash_id, sflash_sflash_id_string)

static struct device_attribute *sflash_attributes[] = {
	&dev_attr_success,
	&dev_attr_probing_cnt,
	&dev_attr_sflash_id,
	NULL
};

struct xmos_flash {
	struct device *dev;
};

static struct task_struct *g_th_id=NULL;
static struct class *class_xmos;

void set_sflash_success(int success)
{
	if(success)
	{
		strncpy(sflash_success_string, "success\n", strlen("success\n"));
	}
	else
	{
		strncpy(sflash_success_string, "fail\n", strlen("fail\n"));
	}
}

void set_sflash_probing_cnt(int cnt)
{
	sprintf(sflash_probing_cnt_string, "%d\n", cnt);
}

void set_sflash_sflash_id(char *id)
{
	sprintf(sflash_sflash_id_string, "%s\n", id);
}

static void dump(struct mtd_info*mtd)
{
	unsigned char rd[BUFF_SZ];
	int ret;
	int i;
	size_t retlen = 0;
	
	ret = mtd->read(mtd, (mtd->erasesize * 0), BUFF_SZ, &retlen, rd);
	if(ret)
	{
		printk("%s : read failed\n", UPDATER);
		return;
	}

	printk("%s : ", UPDATER);
	for(i = 0; i < BUFF_SZ; i++)
	{
		printk("0x%02x, ", rd[i]);
		if((i % 16) == 0)
			printk("\n");
	}
	printk("\n");
}

static int check_empty(struct mtd_info*mtd)
{
	unsigned char rd[COMP_SZ];
	int ret;
	int i;
	size_t retlen = 0;
	
	ret = mtd->read(mtd, 0, COMP_SZ, &retlen, rd);
	if(ret)
	{
		printk("%s : read failed\n", UPDATER);
		return -1;
	}

	for(i = 0; i < COMP_SZ; i++)
	{
		if(rd[i] != 0xff)
		{
			printk("%s : SFLASH is not empty\n", UPDATER);
			return 1;
		}
	}

	msleep(1000);

	/* SFLASH is empty */
	return 0;
}

static int compare_data(struct mtd_info*mtd, int sect, unsigned char *img, int sz)
{
	unsigned char *rd = NULL;
	int ret;
	int i;
	size_t retlen = 0;

	rd = (unsigned char*)kmalloc(mtd->erasesize, GFP_KERNEL);
	if(rd == NULL)
	{
		printk("%s : memory allocation failed\n", UPDATER);
		ret = -1;
		goto out;
	}

	ret = mtd->read(mtd, (mtd->erasesize * sect), sz, &retlen, rd);
	if(ret)
	{
		printk("%s : read failed\n", UPDATER);
		set_sflash_success(0);
		ret = -1;
		goto out;
	}

	for(i = 0; i < sz; i++)
	{
		/* upgrade firmware is changed. so, we need firmware upgrade */
		if(img[i] != rd[i])
		{
			ret = 0;
			goto out;
		}
	}

	msleep(1000);
	ret = 1;

out:
	if(rd)
		kfree(rd);

	return ret;
}

static int verify_image(struct mtd_info*mtd, int sect, unsigned char *img, int sz)
{
	return compare_data(mtd, sect, img, sz);
}

static int erase_sector(struct mtd_info*mtd, int start, int len)
{
	int ret;
	struct erase_info ifo;

	/* erase area to write */
	memset(&ifo, 0, sizeof(ifo));
	ifo.mtd = mtd;
	ifo.addr = start;

	/* calculate accurately size to erase. 
	   the size value is aligned by erasesize of mtd */
	if(len <= mtd->erasesize)
		ifo.len = mtd->erasesize;
	else if(len >= mtd->size)
		ifo.len = mtd->size;
	else
		ifo.len = ((len / mtd->erasesize) + 1) * mtd->erasesize;

	printk("%s: accurate size of erase : %d\n", UPDATER, ifo.len);

	ret = mtd->erase(mtd, &ifo);
	if(ret)
	{
		printk("%s : erase failed : %d\n", UPDATER, ret);
		return -1;
	}

	msleep(600);

	return 0;
}

static int write_firmware(struct mtd_info*mtd, int addr, unsigned char*data, int len)
{
	size_t retlen = 0;
	int from = 0;
	int to = addr;
	int ret;

	printk("%s : addr %x size %d\n", UPDATER, addr, len);

	/* start writing */
	for(to = addr; to < (len + addr); to += retlen)
	{
		ret = mtd->write(mtd, to, (len - retlen), &retlen, &data[from]);
		if(ret)
		{
			printk("%s : write image failed\n", UPDATER);
			return -1;
		}

		from += retlen;
	}

	msleep(200);

	return 0;
}

void power_on_xmos()
{
	// XMOS power on
	gpio_request(GPIO_XMOS_PWR_EN, "xmos_pwr_en");
	tcc_gpio_config(GPIO_XMOS_PWR_EN, GPIO_OUTPUT | GPIO_FN(0)); 
	gpio_set_value(GPIO_XMOS_PWR_EN, 1);

	printk("XMOS : power on\n");
}

void connect_flash_cpu()
{
	// connect cpu and flash
	gpio_request(GPIO_XMOS_SPI_SEL, "xmos_spi_sel");
	tcc_gpio_config(GPIO_XMOS_SPI_SEL, GPIO_OUTPUT | GPIO_FN(0));
	gpio_set_value(GPIO_XMOS_SPI_SEL, 0);
	msleep(100);

	printk("XMOS : paired SFLASH and CPU\n");
}

void connect_flash_xmos()
{
	gpio_set_value(GPIO_XMOS_SPI_SEL, 1);
	printk("XMOS : paired SFLASH and XMOS\n");
}

static int kthread_xmos_firmware_func(void *arg)
{
	int ret;
	int i;
	struct mtd_info*mtd = (struct mtd_info*)arg;

	printk("XMOS Updater : Start\n");

	msleep_interruptible(10);

	/* TODO : make sure SPI path to be connect to CPU */

#if 0
	if(erase_sector(mtd, 0, mtd->size) < 0)
		goto end;
	printk("%s : clearing flash is completed\n", UPDATER);
#else
	/* check for factory image upgrading is needed */
	for(i = 0; i < UPGRADE_TRY_CNT; i++) {
		ret = compare_data(mtd, 0, factory_data, sizeof(factory_data));
		if(ret < 0)  {
			printk("%s: error on comparing factory image.\n", UPDATER);
			goto end;
		} else if(ret == 0) {
			if(i == 0)
				printk("%s: started upgrading factory image.\n", UPDATER);
			else
				printk("%s: retry upgrading factory image.\n", UPDATER);

			if(erase_sector(mtd, (mtd->erasesize * 0), sizeof(factory_data)) < 0)
				continue;

			if(write_firmware(mtd, (mtd->erasesize * 0), factory_data, sizeof(factory_data)) < 0)
				continue;
		} else {
			if(i == 0)
				printk("%s: Don't need factory firmware upgrade\n", UPDATER);
			else
				printk("%s: Factory image upgrading ended successfully.\n", UPDATER);
			
			break;
		}
	}

	if(i == UPGRADE_TRY_CNT) {
		set_sflash_success(0);
		printk("%s: writing factory firmware finally failed\n", UPDATER);
		goto end;
	}

	/* check for upgrade image upgrading is needed */
	for(i = 0; i < UPGRADE_TRY_CNT; i++) {
		ret = compare_data(mtd, UPGRADE_SECTOR, upgrade_data, sizeof(upgrade_data));
		if(ret < 0)  {
			printk("%s: error on comparing upgrade image.\n", UPDATER);
			goto end;
		} else if(ret == 0) {
			if(i == 0)
				printk("%s: started upgrading upgrade image.\n", UPDATER);
			else
				printk("%s: retry upgrading upgrade image.\n", UPDATER);

			if(erase_sector(mtd, (mtd->erasesize * UPGRADE_SECTOR), sizeof(upgrade_data)) < 0)
				continue;

			if(write_firmware(mtd, (mtd->erasesize * UPGRADE_SECTOR), upgrade_data, sizeof(upgrade_data)) < 0)
				continue;
		} else {
			if(i == 0)
				printk("%s: Don't need upgrade firmware upgrade\n", UPDATER);
			else
				printk("%s: Upgrade image upgrading ended successfully.\n", UPDATER);
			
			break;
		}
	}

	if(i == UPGRADE_TRY_CNT) {
		set_sflash_success(0);
		printk("%s: writing upgrade firmware finally failed\n", UPDATER);
	}
#endif

end:
	connect_flash_xmos();
	msleep_interruptible(10);
	gpio_set_value(GPIO_XMOS_PWR_EN, 0);
	msleep_interruptible(10);
	gpio_set_value(GPIO_XMOS_PWR_EN, 1);
	printk("XMOS Updater : Reset XMOS\n");

	/* default xmos power off */
	msleep_interruptible(3000);
	gpio_set_value(GPIO_XMOS_PWR_EN, 0);

	return 0;
} 

static int xmos_create_device(struct xmos_flash *dev)
{
	struct device_attribute **attrs = sflash_attributes;
	struct device_attribute *attr;
	int err;

	dev->dev = device_create(class_xmos, NULL,
					MKDEV(0, 0), NULL, "sflash");
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(class_xmos, dev->dev->devt);
			return err;
		}
	}
    
	return 0;
}

static int __init xmos_firmware_init(void)
{
	struct mtd_info *mtd = NULL;
	struct xmos_sflash *dev;
	int err;

	class_xmos = class_create(THIS_MODULE, "xmos");
	if(IS_ERR(class_xmos))
		return PTR_ERR(class_xmos);

	dev = kzalloc(sizeof(struct xmos_flash), GFP_KERNEL);
	if(!dev)
	{
		class_destroy(class_xmos);
		return -ENOMEM;
	}

	err = xmos_create_device(dev);
	if (err) {
		class_destroy(class_xmos);
		kfree(dev);
		return err;
	}

	mtd = get_mtd_device_nm(MTD_DEVICE);
	if(IS_ERR(mtd))
	{
		printk("XMOS Updater : cannot find %s\n", MTD_DEVICE); 
		connect_flash_xmos();

		msleep(10);
		gpio_set_value(GPIO_XMOS_PWR_EN, 0);

		set_sflash_success(0);
		return -ENODEV;
	}
	else
	{
		set_sflash_success(1);
		printk("XMOS Updater : found mtd dev : %s \n", mtd->name);
	}

	if(g_th_id == NULL){ 
		g_th_id = (struct task_struct *)kthread_run(kthread_xmos_firmware_func, (void*)mtd, "kthread_xmos");
	}

	return 0;
} 

static void __exit xmos_firmware_release(void)
{
	if(g_th_id){
		kthread_stop(g_th_id);
		g_th_id = NULL;
	}

	if(class_xmos != NULL)
		class_destroy(class_xmos);

	printk("XMOS Updater : Bye.\n");
} 

late_initcall(xmos_firmware_init);
module_exit(xmos_firmware_release);
MODULE_LICENSE("Dual BSD/GPL"); 

#endif
