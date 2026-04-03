#ifndef __METAL_IOCTL_
#define __METAL_IOCTL_

//kernel

typedef struct 
{ 
	uint16_t	picid;
	uint16_t	sw_version;
	uint16_t  	program_memory_max_used_address;
	uint8_t   	has_configuration_data;
	uint16_t	*hexdata;
	uint8_t		*filldata;
} __attribute__ ((packed)) ioctl_metalt_info; 
 
#define METALT_IOCTL_MAGIC    'M'  
#define METALT_IO_READ_VERSION	   	_IOR(  METALT_IOCTL_MAGIC, 0 , ioctl_metalt_info )  
#define METALT_IO_FILE_WRITE   			_IOW(  METALT_IOCTL_MAGIC, 1 , ioctl_metalt_info )  
#define METALT_IO_BULK_ERASE	   	_IOR(  METALT_IOCTL_MAGIC, 2 , ioctl_metalt_info )  
#define METALT_IO_FILE_READ   			_IOW(  METALT_IOCTL_MAGIC, 3 , ioctl_metalt_info )  


#endif
