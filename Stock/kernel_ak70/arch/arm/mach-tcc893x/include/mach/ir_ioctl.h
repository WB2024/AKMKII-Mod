#ifndef _ITOOLS_IOCTL_
#define _ITOOLS_IOCTL_

#include <linux/fs.h>
#include <linux/syscalls.h>
#include <asm/unistd.h> //#include <linux/unistd.h>
#include <asm/uaccess.h> //#include <linux/unistd.h>

//

#define SIO_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))


//
// 
//

#define MAX_SIO_FIELDS  (100)
#define MAX_SIO_LIST 30
#define MAX_SIO_STR (32)

enum MSG_ARGV_TYPE{
    SIO_ARGV_START = 0,
	SIO_ARGV_INT8,
	SIO_ARGV_INT16,
	SIO_ARGV_INT32,
	SIO_ARGV_STR,
	SIO_ARGV_DATA,	
	SIO_ARGV_END
};

int sio_tell(char** data_buffer);
int sio_set_argv(char** data_buffer,...);
int sio_write_int(char** data_buffer,int Value,int size);
int sio_write_data(char** data_buffer,char *data,int len);
int sio_write_string(char** data_buffer,char *string);
int sio_read_int(char** data_buffer,int *offset);
int sio_read_string(char** data_buffer,char *string);
int sio_read_data(char** data_buffer,char *data);



#define MAKE_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))


#define  ITOOLS_IOCTL_DEV_NAME            "ir_ioctl"  
#define   ITOOLS_IOCTL_DEV_MAJOR            240  

typedef struct 
{ 
	unsigned char io_buffer[128];
	int ioctl_fd;
	unsigned char *p_from_user_buffer;
	unsigned char *p_to_user_buffer;
} __attribute__ ((packed)) io_buffer_t; 
 
#define ITOOLS_IOCTL_MAGIC    't'  
#define ITOOLS_IO_CMD       		_IOWR( ITOOLS_IOCTL_MAGIC, 10 , io_buffer_t )  


#endif
