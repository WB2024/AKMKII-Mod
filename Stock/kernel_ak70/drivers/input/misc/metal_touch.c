
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <linux/irq.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include "metalt_ioctl.h"

/*
#cat /proc/devices
...
246 tcc_intr
250 metal_touch  <== MAJOR ID: 250
251 ump
252 clockctrl
253 rtc
254 wdma
--------------------------------------
240-254 char	LOCAL/EXPERIMENTAL USE
*/
#define METAL_TOUCH_MAJOR_ID 250
#define METAL_TOUCH_MINOR_ID 1
#define METAL_TOUCH_DEVICE_NAME "metal_touch"



#define DRV_NAME "pic-metal_touch"  // refer to board_ak_keypad.c

#if( CUR_AK_REV == AK380_HW_EVM)
#define PIC_MCLR   							TCC_GPA(9)
#define PIC_VDD									TCC_GPC(6)
#define PIC_DATA  							TCC_GPC(1)
#define PIC_DATAIN 							PIC_DATA	/* Input*/
#define PIC_CLK    								TCC_GPC(5)

#else
/*
#define GPIO_M_TOUCH_MCLR        TCC_GPA(9)
#define GPIO_METAL_PWR_EN			TCC_GPC(6) 
#define GPIO_M_TOUCH_DAT    		TCC_GPC(1) 
#define GPIO_HOME_KEY    				TCC_GPC(4) 
#define GPIO_M_TOUCH_CLK			TCC_GPC(5) 
*/
#define PIC_MCLR   							GPIO_M_TOUCH_MCLR
#define PIC_VDD									GPIO_METAL_PWR_EN
#define PIC_DATA  							GPIO_M_TOUCH_DAT
#define PIC_DATAIN 							PIC_DATA	/* Input*/
#define PIC_CLK    								GPIO_M_TOUCH_CLK
#endif

#define DELAY      				1		/* microseconds */
#define DELAY_MCLR      50	/* microseconds */

#define PICMEMSIZE (0x8000+0x10) // Program Memory: 0~0x8000 word  Configuration Memory: 0x8000~0x800A word

extern int tcc893x_direct_gpio_direction_input(unsigned gpio_group, unsigned offset);
extern int tcc893x_direct_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value);

struct picmemory {
	uint16_t  program_memory_used_cells;
	uint16_t  program_memory_max_used_address;

	uint8_t   has_configuration_data;
	uint8_t   has_eeprom_data;

	uint16_t		*data;		/* 14-bit data */
	uint8_t  *filled;	/* 1 if this cell is used */
};

struct picmicro {
	uint16_t device_id;					//#define PIC16F84A  0x0560
	char     name[16];					//"pic12lf1552"
	size_t   program_memory_size;		//0x400
	size_t   data_memory_size;			//64

	int program_cycle_time;			
	int eeprom_program_cycle_time;	
	int erase_and_program_cycle_time;	
	int bulk_erase_cycle_time;		

	uint8_t load_configuration_cmd;				//Load Configuration 			0x00
	uint8_t load_data_for_program_memory_cmd;	//Load Data for Program Memory 	0x02
	uint8_t load_data_for_data_memory_cmd;		//Load Data for Data Memory		0x03  ????
	uint8_t read_data_from_program_memory_cmd;	//Read Data from Program Memory	0x04
	uint8_t read_data_from_data_memory_cmd;	//Read Data from Data Memory	0x05  ????
	uint8_t increment_address_cmd;				//Increment Address				0x06
	uint8_t begin_erase_programming_cycle_cmd;	//Begin Erase Programming Cycle	0x08 ???
	uint8_t begin_programming_only_cycle_cmd;	//Begin Programming Only Cycle	0x18 ???
	uint8_t bulk_erase_program_memory_cmd;		//Bulk Erase Program Memory		0x09 ???
	uint8_t bulk_erase_data_memory_cmd;			//Bulk Erase Data Memory		0x0B ???
	uint8_t reset_address_cmd;			//Reset address	0x16

};

#define PIC12LF1552  0x2BC0
/*                                                id      ,      name ,  program_memory_size,  data_memory_size,  program_cycle_time  */                          
const struct picmicro pic12lf1552  = {PIC12LF1552,"pic12lf1552",      0x400,                    0,  		        100,           4000,  8000, 10000, 0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x18, 0x09, 0x0B, 0x16};

const struct picmicro *piclist[] = {&pic12lf1552,NULL};

void send_magic_key();	

void GPIO_CLR(int n)
{
	gpio_set_value(n,0); 
	if(n==PIC_MCLR){
		//uwait(50);  // 중요 MCLR 신호를 Low로 한 후 일정 시간 뒤에 커맨드를 보낸다. 
		mdelay(1);
		send_magic_key();
	}
}

void GPIO_SET(int n)
{
	gpio_set_value(n,1); 
}

 void GPIO_IN(unsigned n)
{
	//gpio_direction_input(n);
	//tcc893x_direct_gpio_direction_input((2 << GPIO_REG_SHIFT),(n&0x0000001F));
	tcc893x_direct_gpio_direction_input((n&(~0x0000001F)),(n&0x0000001F));
	//printk("0x0000001F, 0x%x, 0x%x\n",!0x0000001F,~0x0000001F);
}

void SettingGpioInputMode(void)
{
	//printk("SettingGpioInputMode\n");
	GPIO_IN(GPIO_HOME_KEY);
	tcc_gpio_config(GPIO_HOME_KEY,GPIO_FN(0) | GPIO_PULLUP);
}
	
 void GPIO_OUT(unsigned n,unsigned val)
{
	//gpio_direction_output(n,0);
	tcc893x_direct_gpio_direction_output((n&(~0x0000001F)),(n&0x0000001F),0);
}

int GPIO_LEV(int n)
{
	return gpio_get_value(n);
}

void uwait(int utime)
{
	udelay(utime);
}
void GPIO_INIT()
{
	//gpio_request(PIC_VDD, "PIC_VDD");
	//tcc_gpio_config(PIC_VDD, GPIO_FN(0));
	GPIO_OUT(PIC_VDD, 0);	
	
	//gpio_request(PIC_MCLR, "PIC_MCLR");
	//tcc_gpio_config(PIC_MCLR, GPIO_FN(0));
	GPIO_OUT(PIC_MCLR, 0);	

	//gpio_request(PIC_DATA, "PIC_DATA");
	//tcc_gpio_config(PIC_DATA, GPIO_FN(0));	
	GPIO_OUT(PIC_DATA,0);	

	//gpio_request(PIC_CLK, "PIC_CLK");
	//tcc_gpio_config(PIC_CLK, GPIO_FN(0));
	GPIO_OUT(PIC_CLK,0);	

}

void pic_send_cmd_n(uint32_t bits, unsigned int n)
{
	unsigned int i;
	GPIO_OUT(PIC_DATA, 0);	//output

	for(i = 0; i < n; i++){
		gpio_set_value(PIC_CLK,true);  // high
		gpio_set_value(PIC_DATA,bits & (1 << i));
		uwait(DELAY);
		gpio_set_value(PIC_CLK,false); // Low
		uwait(DELAY);
	}
	gpio_set_value(PIC_DATA,false);
}

void pic_send_cmd(uint8_t bits)
{
	unsigned int i;
	GPIO_OUT(PIC_DATA, 0);	//output

	for(i = 0; i < 6; i++){
		gpio_set_value(PIC_CLK,true);
		gpio_set_value(PIC_DATA,bits & (1 << i));
		uwait(DELAY);
		gpio_set_value(PIC_CLK,false);
		uwait(DELAY);
	}
	gpio_set_value(PIC_DATA,false);
}	

uint16_t pic_read_data(void)
{
#if 0
	int i;
	uint16_t data = 0;

	gpio_direction_input(PIC_DATAIN);	//input
	
	for (i = 0; i < 16; i++) {
		GPIO_SET(PIC_CLK);
		uwait(DELAY);	/* uwait for data to be valid */
		data |= (GPIO_LEV(PIC_DATAIN)) << i;
		uwait(DELAY);
		GPIO_CLR(PIC_CLK);
		uwait(DELAY);
	}
	data = (data >> 1) & 0x3FFF;

	GPIO_OUT(PIC_DATAIN);

	return data;
#else
	int i;
	uint16_t data = 0;

	GPIO_IN(PIC_DATAIN);	//input
	
	for (i = 0; i < 16; i++) {
		GPIO_SET(PIC_CLK);
		uwait(DELAY);	/* uwait for data to be valid */
		data |= (GPIO_LEV(PIC_DATAIN)) << i;
		uwait(DELAY);
		GPIO_CLR(PIC_CLK);
		uwait(DELAY);
	}
	data = (data >> 1) & 0x3FFF;

	GPIO_OUT(PIC_DATAIN,0);

	return data;

#endif
}


void pic_load_data(uint16_t data)
{
	int i;

	/* Insert start and stop bit (0s) */
	data = (data << 1) & 0x7FFE;
	for (i = 0; i < 16; i++) {
		GPIO_SET(PIC_CLK);
		if ((data >> i) & 0x01)
			GPIO_SET(PIC_DATA);
		else
			GPIO_CLR(PIC_DATA);
		uwait(DELAY);	/* Setup time */
		GPIO_CLR(PIC_CLK);
		uwait(DELAY);	/* Hold time */
	}
	GPIO_CLR(PIC_DATA);
	uwait(DELAY);
}

uint16_t pic_read_device_id_word(const struct picmicro *pic)
{
	int i;
	uint16_t data;

	GPIO_CLR(PIC_MCLR);	/* Enter Program/Verify Mode */

	pic_send_cmd(pic->load_configuration_cmd);
	pic_load_data(0x3FFF);
	uwait(1);
	for (i = 0; i < 6; i++)
		pic_send_cmd(pic->increment_address_cmd);
	pic_send_cmd(pic->read_data_from_program_memory_cmd);
	data = pic_read_data();

	GPIO_SET(PIC_MCLR);	/* Exit Program/Verify Mode */
	uwait(DELAY);

	return data;
}


void send_magic_key()
{
	// 0x4D434850 is "MCHP in ASCII"
	pic_send_cmd_n(0x4D434850, 32);
	pic_send_cmd_n(0, 1);
	//uwait(100);
}

void pic_config_read(const struct picmicro *pic)
{
	uint16_t data;
	int i;
	
	printk("pic_config_read...\n");

	GPIO_CLR(PIC_MCLR);
	//uwait(DELAY_MCLR);
	
	pic_send_cmd(pic->load_configuration_cmd);	
	pic_load_data(0x3FFF);
	
	printk("[%04x]",0x8000);

	for(i=0x8000; i<=0x800A; i++) {
		pic_send_cmd(pic->read_data_from_program_memory_cmd);
		data = pic_read_data();
		pic_send_cmd(pic->increment_address_cmd);
		
		printk(" %04x",data);
		if(((i+1) % 8) ==0)
			printk("\n[%04x]",i+1);
		
	}
	printk("\n");
	
	GPIO_SET(PIC_MCLR);
}

void pic_config_write(const struct picmicro *pic, uint16_t start_address )
{
	printk( "pic_config_write...\n");

	GPIO_CLR(PIC_MCLR);

	pic_send_cmd(pic->load_configuration_cmd);	
	pic_load_data(0x3FFF);
	

	//pic_send_cmd(pic->increment_address_cmd);
	pic_send_cmd(pic->load_data_for_program_memory_cmd);
	pic_load_data(0x0001);
	pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
	mdelay(10); // config write시 5ms 이상 
	
	GPIO_SET(PIC_MCLR);

	pic_config_read(pic);
}

void pic_program_read(const struct picmicro *pic, uint16_t start_address, uint16_t size)
{
	uint16_t data;
	int i;
	
	printk( "pic_program_read...\n");

	GPIO_CLR(PIC_MCLR);

	pic_send_cmd(pic->reset_address_cmd);	/* Clear program memory only */

	printk( "[%04x]",start_address);

	for(i=0; i<start_address; i++) 
		pic_send_cmd(pic->increment_address_cmd);


	for(i=start_address; i<(start_address+size+1); i++) {
		pic_send_cmd(pic->read_data_from_program_memory_cmd);
		data = pic_read_data();
		pic_send_cmd(pic->increment_address_cmd);
				
		printk( " %04x",data);
		
		if(((i+1) % 8) ==0){
		printk("\n[%04x]",i+1);
		}
	}
	GPIO_SET(PIC_MCLR);

	printk( "\n");

}

void pic_program_write(const struct picmicro *pic, uint16_t start_address )
{
	printk( "pic_program_write...\n");

	GPIO_CLR(PIC_MCLR);
	pic_send_cmd(pic->reset_address_cmd);	/* Clear program memory only */	

	pic_send_cmd(pic->load_data_for_program_memory_cmd);
	pic_load_data(0x1234);
	pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
	mdelay(10); // program write시 2.5ms 이상 
	
	GPIO_SET(PIC_MCLR);

	pic_program_read(pic,start_address,0x10);
}

		
/* Bulk erase the chip */
void pic_bulk_erase(const struct picmicro *pic, int debug)
{
	printk( "Bulk erasing chip...\n");

	if (debug)
		printk( "  Erasing program and configuration memory...\n");

	GPIO_CLR(PIC_MCLR);
	uwait(DELAY_MCLR);

	pic_send_cmd(pic->reset_address_cmd);
	pic_send_cmd(pic->load_configuration_cmd);	
	pic_load_data(0x3FFF);
	pic_send_cmd(pic->bulk_erase_program_memory_cmd);
	mdelay(50); // 최소 5ms 이상 

	GPIO_SET(PIC_MCLR);
	uwait(DELAY);
		
}

uint16_t read_sw_version(const struct picmicro *pic)
{

	uint16_t data;
	
	GPIO_CLR(PIC_MCLR);
	
	pic_send_cmd(pic->load_configuration_cmd);	
	pic_load_data(0x3FFF);
	

	pic_send_cmd(pic->read_data_from_program_memory_cmd);
	data = pic_read_data();
	
	GPIO_SET(PIC_MCLR);
	
	return data;
}

void write_sw_version(const struct picmicro *pic, uint16_t data)
{
	printk( "write_sw_version...0x%04x\n",data);

	GPIO_CLR(PIC_MCLR);

	pic_send_cmd(pic->load_configuration_cmd);	
	pic_load_data(0x3FFF);

	pic_send_cmd(pic->load_data_for_program_memory_cmd);
	pic_load_data(data);
	pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
	mdelay(10); // config write시 5ms 이상 
	
	GPIO_SET(PIC_MCLR);

	pic_program_read(pic,0x0000,0x20);
	pic_config_read(pic);
}

int pic_write(const struct picmicro *pic,  struct picmemory *pm, int debug)
{
	int perror = 0;
	int cerror = 0;
	uint16_t addr, data;
	//struct picmemory *pm;

	printk( "Writing program...\n");
	

	//pm->has_configuration_data = pic->has_configuration_data;
	//pm->program_memory_max_used_address = pic->program_memory_max_used_address;

	printk("pm.has_configuration_data:%d\n",pm->has_configuration_data);
	printk("pm.program_memory_max_used_address:%d\n",pm->program_memory_max_used_address);
	while(1);


	//ReadIntelHexFile(fopen(infile, "r"));
	//pm = read_inhx16(infile, debug);

	if (!pm)
		return 0;

	pic_bulk_erase(pic, debug);


	GPIO_CLR(PIC_MCLR);
	
	pic_send_cmd(pic->reset_address_cmd);	/* Clear program memory only */	


	for (addr = 0; addr <= pm->program_memory_max_used_address; addr++) {
		if (pm->filled[addr]) {

			pic_send_cmd(pic->load_data_for_program_memory_cmd);
			pic_load_data(pm->data[addr]);
			pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
			mdelay(10); // program write시 2.5ms 이상 
		
			pic_send_cmd(pic->read_data_from_program_memory_cmd);
			data = pic_read_data();

			if (debug)
				printk( "  addr = 0x%04X  written = 0x%04X  read = 0x%04X\n", addr, pm->data[addr], data);

			if (pm->data[addr] != data) {
				printk( "\nProgram Error: addr = 0x%04X, written = 0x%04X, read = 0x%04X\n", addr, pm->data[addr], data);
				perror = 1;
				break;
			}
		}	

		if(!(addr%0x100))
			printk(".");
			
			pic_send_cmd(pic->increment_address_cmd);
	}

	printk("\n");

	GPIO_SET(PIC_MCLR);
	uwait(DELAY_MCLR);	
	
	/* Write Configuration Memory */
	if (pm->has_configuration_data) {
		if (debug)
			printk( "  Configuration memory:\n");

		GPIO_CLR(PIC_MCLR);
		//uwait(DELAY_MCLR);

		pic_send_cmd(pic->load_configuration_cmd);	
		pic_load_data(0x3FFF);		

		for (addr = 0x8000; addr < 0x8009; addr++) {
			if ((addr >= 0x8000 ) && pm->filled[addr]) {
				pic_send_cmd(pic->load_data_for_program_memory_cmd);
				pic_load_data(pm->data[addr]);
				pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
				mdelay(10); // config write시 5ms 이상 

				pic_send_cmd(pic->read_data_from_program_memory_cmd);
				data = pic_read_data();

				if (debug)
					printk( "  addr = 0x%04X  written = 0x%04X  read = 0x%04X\n", addr, pm->data[addr], data);

				if ((pm->data[addr]&0x3FFF) != data) {
					printk( "Config Error: addr = 0x%04X, written = 0x%04X, read = 0x%04X\n", addr, pm->data[addr], data);
					cerror = 1;
				//	break;
				}
			}
			pic_send_cmd(pic->increment_address_cmd);
		}
		GPIO_SET(PIC_MCLR);

	}	

	return perror;
}
static ssize_t  metal_touch_read(struct file *file, char *buf, size_t count, loff_t *f_pos)
{
    printk("%s\n", __func__);
	return 0;
}

uint8_t 	data8[PICMEMSIZE*2];
uint16_t *pDataAddr;
static ssize_t  metal_touch_write(struct file *file, const char *buf, size_t szie, loff_t *f_pos)
{

	printk("+%s\n", __func__);

	copy_from_user(&data8, buf, PICMEMSIZE*2);
	
#if 0
	for(i=0; i<PICMEMSIZE*2; i=i+2){
			data16[add] = ((data8[i+1]<<8) | data8[i]);
			add++;
	}
#endif

	pDataAddr=(uint16_t *)data8;  //byte -> word단위 주소로 변경 

#if 0	
	printk("[%04x]",0);
	for(i=0; i<PICMEMSIZE; i++){			
			printk(" %04x",pDataAddr[i]);
			if(((i+1) % 8) ==0){
			printk("\n[%04x]",(i+1));
			}
	}
#endif

	printk("-%s\n", __func__);

    return 0;
 }

uint16_t 	data16[PICMEMSIZE];
static long do_metal_touch_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int ret;
 	ioctl_metalt_info    pm;
	const struct picmicro *pic;
	int perror = 0;
	int cerror = 0;
	uint16_t addr, data;
	int debug = 0;
	uint16_t mTouch_id,mTouch_sw_version,mTouch_update_state;

	pic = &pic12lf1552;

//rintk("do_metal_touch_ioctl\n");
	switch (cmd) {
		case METALT_IO_READ_VERSION:
			printk("METALT_IO_READ_VERSION\n" );
			mTouch_id=	pic_read_device_id_word(pic);
			mTouch_sw_version = read_sw_version(pic);
			pm.picid= mTouch_id;
			pm.sw_version= mTouch_sw_version;
			//printk("pic id:0x%x sw_ver:%04d (%04d)\n",mTouch_id,mTouch_sw_version);
			copy_to_user((void*)arg, (const void*)&pm, sizeof(pm));

			SettingGpioInputMode();
		
			return 0;			
		case METALT_IO_BULK_ERASE:
			printk("METALT_IO_BULK_ERASE\n" );
			ret = copy_from_user( (void *)&pm, (void *)arg, sizeof(pm) );
			pic_bulk_erase(pic, 1); 

			SettingGpioInputMode();
		
			return 0;	
		case METALT_IO_FILE_READ:	
			printk("METALT_IO_FILE_READ\n" );
			pic_program_read(pic,0x0000,0x20);
			pic_program_read(pic,0x8000,0x10);
			
			SettingGpioInputMode();
		
			return 0;
		case METALT_IO_FILE_WRITE:
			printk("METALT_IO_HEX_WRITE\n" );
			ret = copy_from_user( (void *)&pm, (void *)arg, sizeof(pm) );
			//printk("pm.has_configuration_data:%d\n",pm.has_configuration_data);
			//printk("pm.program_memory_max_used_address:%d\n",pm.program_memory_max_used_address);

			//erase
			pic_bulk_erase(pic, 1); 

			GPIO_CLR(PIC_MCLR);
			
			pic_send_cmd(pic->reset_address_cmd);	/* Clear program memory only */	

			for (addr = 0; addr <= pm.program_memory_max_used_address; addr++) {
				if (pm.filldata[addr]) {

					pic_send_cmd(pic->load_data_for_program_memory_cmd);
					pic_load_data(pm.hexdata[addr]);
					pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
					mdelay(3); // program write시 2.5ms 이상 
				
					pic_send_cmd(pic->read_data_from_program_memory_cmd);
					data = pic_read_data();

					if (debug)
						printk( "  addr = 0x%04X  written = 0x%04X  read = 0x%04X\n", addr, pm.hexdata[addr], data);

					if (pm.hexdata[addr] != data) {
						printk( "\nProgram Error: addr = 0x%04X, written = %04d, read = %0dX\n", addr, pm.hexdata[addr], data);
						perror = 1;
						break;
					}
				}	

				if(!(addr%0x10))
					printk(".");
					
					pic_send_cmd(pic->increment_address_cmd);
			}

			printk("\n");

			GPIO_SET(PIC_MCLR);
			
			/* Write Configuration Memory */
			if (pm.has_configuration_data) {
				if (debug)
					printk( "  Configuration memory:\n");

				GPIO_CLR(PIC_MCLR);
				//uwait(DELAY_MCLR);

				pic_send_cmd(pic->load_configuration_cmd);	
				pic_load_data(0x3FFF);		

				for (addr = 0x8000; addr < 0x8009; addr++) {
					if ((addr >= 0x8000 ) && pm.filldata[addr]) {
						pic_send_cmd(pic->load_data_for_program_memory_cmd);
						pic_load_data(pm.hexdata[addr]);
						pic_send_cmd(pic->begin_erase_programming_cycle_cmd);
						mdelay(6); // config write시 5ms 이상 

						pic_send_cmd(pic->read_data_from_program_memory_cmd);
						data = pic_read_data();

						if (debug)
							printk( "  addr = 0x%04X  written = 0x%04X  read = 0x%04X\n", addr, pm.hexdata[addr], data);

						if ((pm.hexdata[addr]&0x3FFF) != data) {
							printk( "Config Error: addr = 0x%04X, written = 0x%04X, read = 0x%04X\n", addr,pm.hexdata[addr], data);
							perror = 1;
							break;
						}
					}
					pic_send_cmd(pic->increment_address_cmd);
				}
				GPIO_SET(PIC_MCLR);

			}

			if(perror)
				write_sw_version(pic, 0x00); //update fial
			else
				write_sw_version(pic, pm.sw_version); 	//upate success

			/*
			printk("[%04x]",0);
			for(i=0; i<PICMEMSIZE; i++){			
					printk(" %04x",param.hexdata[i]);
					if(((i+1) % 8) ==0)
					printk("\n[%04x]",(i+1));						
			}
			*/
			
			SettingGpioInputMode();

			return 0;
	}
}
static DEFINE_MUTEX(metal_touch_mutex);
int metal_touch_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{  
	long ret;
	mutex_lock(&metal_touch_mutex);
	ret = do_metal_touch_ioctl(file, cmd, arg);
	mutex_unlock(&metal_touch_mutex);
	return ret;
}

static int metal_touch_open(struct inode *inode, struct file *filp)
{
    printk("%s\n", __func__);

	return 0;
}

struct file_operations metal_touch_fops = {
    .owner          = THIS_MODULE,
    .read           = metal_touch_read,
    .write          =metal_touch_write,
    .unlocked_ioctl = metal_touch_ioctl,
    .open           = metal_touch_open,
//    .release        = metal_touch_release,
};
 

static struct class *metalT_class;
static int __init metal_touch_init(void)
{
	printk("metal_touch_init\n");
	static int res;	
	const struct picmicro *pic;
	uint16_t device_id;

	
	res = register_chrdev(METAL_TOUCH_MAJOR_ID, METAL_TOUCH_DEVICE_NAME, &metal_touch_fops);
	if(res  < 0)
		return res;

	metalT_class = class_create(THIS_MODULE, METAL_TOUCH_DEVICE_NAME);

	device_create(metalT_class, NULL, MKDEV(METAL_TOUCH_MAJOR_ID, METAL_TOUCH_MINOR_ID), NULL, METAL_TOUCH_DEVICE_NAME);

	//testpic();
	
	GPIO_INIT();

	uwait(100);
	
	GPIO_SET(PIC_VDD);
	GPIO_SET(PIC_MCLR);

	
	pic = &pic12lf1552;
	device_id = pic_read_device_id_word(pic);

	printk( "PIC ID:0x%x Cur_ver:%03d\n",device_id,	read_sw_version(pic));


	return 0;
}

static void __exit metal_touch_exit(void)
{
	printk("metal_touch_init\n");
	unregister_chrdev(METAL_TOUCH_MAJOR_ID, METAL_TOUCH_DEVICE_NAME);
}

module_init(metal_touch_init);
module_exit(metal_touch_exit);

MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DESCRIPTION("Metal Touch driver");
MODULE_AUTHOR("Youshin Yim");
MODULE_LICENSE("GPL v2");
