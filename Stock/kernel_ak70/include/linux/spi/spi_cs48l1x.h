#ifndef __CS48L1X_EXT_SPI_H__
#define __CS48L1X_EXT_SPI_H__

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>

#define CS_MODE_CONTINUOUS_WRITE 	0x1
#define CS_IN_CONTINUOUS_WRITING	0x2

struct spi_gpio_ext_data {
	unsigned	cs;
	unsigned	busy;

	u8 read;
	u8 write;

	spinlock_t 	lock;
	unsigned int mode;		/* CS_MODE_CONTINUOUS_WRITE */

	void (*set_mode)(struct spi_device *spi, int mode);

	wait_queue_head_t rx_wait_q;
};

/* command number is started from <include/linux/spi/spidev.h> */
#define SPI_IOC_WR_MODE_CONT_WRITE		_IOW(SPI_IOC_MAGIC, 5, __u32)
#define SPI_IOC_SET_SLEEP_MODE			_IOW(SPI_IOC_MAGIC, 6, __u32)
#define SPI_IOC_SET_SUSPENDED			_IOW(SPI_IOC_MAGIC, 7, __u32)
#define SPI_IOC_GET_SUSPENDED			_IOW(SPI_IOC_MAGIC, 8, __u32)

#endif
