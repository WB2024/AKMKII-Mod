/*
 * ir_ioctl.
 *
 * Copyright 2010 iriver
 * 2013-5-22 
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <linux/io.h>
#include <mach/gpio.h>

#include <linux/miscdevice.h>

#include <mach/ir_ioctl.h>
#include <linux/file.h>
#include <asm/cacheflush.h>

#include <linux/string.h>

#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

//
// smart byte stream IO.
//
//

int sio_tell(char** data_buffer)
{
	return (int)(*data_buffer);
}

inline int _write_data_8(char** data_buffer,unsigned char Value)
{
	**data_buffer = Value;
	(*data_buffer)++;
	return  0;
}

int write_data_8(char** data_buffer,unsigned char Value)
{
	return _write_data_8(data_buffer,Value);
}

int  write_data_16(char** data_buffer,unsigned short Value)
{
	write_data_8(data_buffer,(unsigned char)Value);
	write_data_8(data_buffer,(unsigned char)(Value >> 8));
    return 0;
}
int write_data_32(char** data_buffer,int Value)
{
    write_data_8(data_buffer, (unsigned char)Value);
    write_data_8(data_buffer, (unsigned char)(Value >> 8));
    write_data_8(data_buffer, (unsigned char)(Value >> 16));
    write_data_8(data_buffer, (unsigned char)(Value >> 24));
    return 0;	 
}

int sio_write_int(char** data_buffer,int Value,int type)
{
    char *org_data_buffer = *data_buffer;
 
    write_data_8(data_buffer,type);
    switch(type) {
	case SIO_ARGV_INT8: 
		write_data_8(data_buffer,1);
		write_data_8(data_buffer,Value); 
		break;
	case SIO_ARGV_INT16:
		write_data_8(data_buffer,2);
		write_data_16(data_buffer,Value); 
		break;
	case SIO_ARGV_INT32:  
		write_data_8(data_buffer,4);
		write_data_32(data_buffer,Value); 
		break;
    }
 
    return (int)*data_buffer - (int)org_data_buffer;
}

int  sio_write_string(char** data_buffer,char *string)
{
    char *org_data_buffer = *data_buffer;
	int str_len;
		
    write_data_8(data_buffer,SIO_ARGV_STR);
	str_len = strlen(string);
    write_data_8(data_buffer,str_len);
	memcpy(*data_buffer,string,str_len);
	(*data_buffer) += str_len;
	
	return (int)(*data_buffer) - (int)org_data_buffer;
}

int  sio_write_data(char** data_buffer,char *data,int len)
{
    char *org_data_buffer = *data_buffer;
	
    write_data_8(data_buffer,SIO_ARGV_DATA);
    write_data_32(data_buffer,len);
	 
	memcpy(*data_buffer,data,len);
	(*data_buffer) += len;
	
	return (int)(*data_buffer) - (int)org_data_buffer;
}

inline int read_data_8(char** data_buffer)
{
	int Value;
	Value = **data_buffer;
	(*data_buffer)++;
	return Value;
}

unsigned short read_data_16(char** data_buffer)
{
    unsigned int iValue;
    iValue = read_data_8(data_buffer);
    iValue |= read_data_8(data_buffer) << 8;
    return (unsigned short)iValue;
}

unsigned int read_data_32(char** data_buffer)
{
    unsigned int iValue;
    iValue = read_data_8(data_buffer);
    iValue |= read_data_8(data_buffer) << 8;
    iValue |= read_data_8(data_buffer) << 16;
    iValue |= read_data_8(data_buffer) << 24;
    return iValue;
}

int  sio_read_int(char** data_buffer,int *offset)
{
	int value = 0;
	
	char *org_data_buffer = *data_buffer;
	int Type,Size;

	Type = read_data_8(data_buffer );
	Size = read_data_8(data_buffer);

    switch(Type) {
    case SIO_ARGV_INT8: 
        value = read_data_8(data_buffer);
        break;
    case SIO_ARGV_INT16:
        value = read_data_16(data_buffer);
        break;
    case SIO_ARGV_INT32:  
        value= read_data_32(data_buffer);
        break;
 	}	

	if(offset) *offset =  (int)*data_buffer - (int)org_data_buffer;
 
	return value;
}

int  sio_read_data(char** data_buffer,char *data)
{
	char *org_data_buffer = *data_buffer;
	int Type,Size;
    
	Type = read_data_8(data_buffer );
	Size = read_data_32(data_buffer);	
	memcpy(data,*data_buffer,Size);
	(*data_buffer) += Size;
    
	return (int)*data_buffer - (int)org_data_buffer;
}
int  sio_read_string(char** data_buffer,char *string)
{
	int offset;
	char *org_data_buffer = *data_buffer;
	int Type,Size;
    
	Type = read_data_8(data_buffer );
	Size = read_data_8(data_buffer);	

	if(Size>= MAX_SIO_STR) {
		DBG_PRINT("IR_IOCTL","Error !!! Offset:%X %d Size:%d %s\n",*data_buffer ,(int)*data_buffer - (int)org_data_buffer,Size,string);
		return 0;
	}

	strncpy(string,*data_buffer,Size);

    
	(*data_buffer) += Size;	
	offset = (int)*data_buffer - (int)org_data_buffer;

	return offset;
}

int _sio_set_argv(char** data_buffer,va_list list)
{
    int i;
    int iCountArgc;
    int data_length = 0;
	
    int ArgvType = -1;
    char *desc;
     	
    iCountArgc = 0;
	
    for(i=0;i < MAX_SIO_FIELDS;i++) {
        ArgvType= va_arg(list,int);
		
        if(ArgvType == SIO_ARGV_END)
            goto End;           

		desc = (char*)va_arg(list,int);
	
        switch(ArgvType) {
        case SIO_ARGV_INT8:
        case SIO_ARGV_INT16:
        case SIO_ARGV_INT32:
        {
            int Value;
            Value = (int)va_arg(list,int);
            data_length +=  sio_write_int(data_buffer,Value,ArgvType);						
        }
        break;
        case SIO_ARGV_STR:
        {
            char *string;
            string = (char*)va_arg(list,int);
            data_length += sio_write_string(data_buffer,string);						
        }
        break;
        case SIO_ARGV_DATA:
        {
            char *data;
            int data_size;
					
            data = (char*)va_arg(list,int);
            data_size = (int)va_arg(list,int);					
            data_length += sio_write_data(data_buffer,data,data_size);						
        }
        break;
        default:
        {
            char *string;
            string = (char*)va_arg(list,int);
        }

        break;
        break;
        }
    }  
	
End:
    return data_length;
}

int sio_set_argv(char** data_buffer,...)
{
    int data_length = 0; 	
    va_list list;
    
    va_start(list,data_buffer); 

	data_length=_sio_set_argv(data_buffer,list);	

    va_end(list);
    return data_length;
}

int ir_ioctl_open(struct inode *inode, struct file *filp)  
{
	return nonseekable_open(inode, filp);
}  
  
int ir_ioctl_release(struct inode *inode, struct file *filp)  
{  
    return 0;  
}  

extern int ir_ioctl_cmd(io_buffer_t *p_io_buffer);

unsigned long atoxi(char *s)
{
    int ishex=0;
    unsigned long res=0;
    int minux = 1;

    if(*s==0) return 0;

    if(*s=='0' && *(s+1)=='x') {
        ishex=1;
        s+=2;
    }
    
    if(*s=='-') {
        minux = -1;
        s++;
    }

    while(*s) {
        if(ishex)
            res*=16;
        else
            res*=10;

        if(*s>='0' && *s<='9')
            res+=*s-'0';
        else if(ishex && *s>='a' && *s<='f')
            res+=*s+10-'a';
        else if(ishex && *s>='A' && *s<='F')
            res+=*s+10-'A';
        else
            break;
      
        s++;
    }

    return res * minux;
}

int ir_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)  
{  
    int size;  
    int ret;
	int result_len;
 	io_buffer_t io_buffer;
	
    size = cmd;
		
	//DBG_PRINT("IR_IOCTL","size:%d\n",size);

	memset(&io_buffer,sizeof(io_buffer_t),0x0);
	ret = copy_from_user ( (void *)&io_buffer.io_buffer, (const void *) arg, size );	 

	result_len = ir_ioctl_cmd(&io_buffer);
	if(result_len > 0) {
		ret = copy_to_user ( (void *) arg, (const void *) &io_buffer.io_buffer, (unsigned long ) result_len );	 
	}
    
    return 0;  
}  


static ssize_t printk_show(struct device *dev,struct device_attribute *attr, char *buf)
{

	int count = 0; 
	u8 data; 
//	count += sprintf(buf,"%d",g_sleep_debug); 
	return count; 
}

extern int g_printk_disable;
extern int g_printk2_disable;

static ssize_t printk_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t size)
{
	int count=0;
	int onoff;

	if(size < 2) return -EINVAL;

	count = size;

	if(!strncmp(buf,"1",size-1)) {
		g_printk_disable = 0;
		g_printk2_disable = 0;

	} else {
		g_printk_disable = 1;
		g_printk2_disable = 1;
	}
  
	return count ;
}

static DEVICE_ATTR(printk, 0777, printk_show, printk_store); 


struct file_operations ir_ioctl_fops =  {  
    .owner    = THIS_MODULE,  
    .unlocked_ioctl    = ir_ioctl,  
    .open     = ir_ioctl_open,       
    .release  = ir_ioctl_release,    
};  

static struct miscdevice ir_ioctl_misc_device = {
    .minor =MISC_DYNAMIC_MINOR,
    .name = ITOOLS_IOCTL_DEV_NAME,
    .fops = &ir_ioctl_fops,
};

static int __init ir_ioctl_modinit(void)
{
	int ret;

    if ((ret = misc_register(&ir_ioctl_misc_device)) < 0 ) {
        printk(KERN_INFO "regist ir_ioctl drv failed \n");
        misc_deregister(&ir_ioctl_misc_device);
	}

	ret = sysfs_create_file(&(ir_ioctl_misc_device.this_device->kobj),&dev_attr_printk.attr);
	if (ret) {
		printk(KERN_ERR
			"Unable to register sysdev entry for sleep_debug sysfs.");
	}

	printk(KERN_INFO "IR_IOCTL interface driver. \n");

	return ret;
}
module_init(ir_ioctl_modinit);

static void __exit ir_ioctl_exit(void)
{

	sysfs_remove_file(&(ir_ioctl_misc_device.this_device->kobj), &dev_attr_printk.attr);

    misc_deregister(&ir_ioctl_misc_device);
}

module_exit(ir_ioctl_exit);


MODULE_DESCRIPTION("ir_ioctl driver");
MODULE_AUTHOR("iRiver");
MODULE_LICENSE("GPL");

