/*
 * SPI master driver using generic bitbanged GPIO
 *
 * Copyright (C) 2006,2008 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/spi/spi_gpio.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <mach/bsp.h>
#include <mach/io.h>
#include <mach/gpio.h>

#include <linux/spi/spi_cs48l1x.h>

/*
 * This bitbanging SPI master driver should help make systems usable
 * when a native hardware SPI engine is not available, perhaps because
 * its driver isn't yet working or because the I/O pins it requires
 * are used for other purposes.
 *
 * platform_device->driver_data ... points to spi_gpio
 *
 * spi->controller_state ... reserved for bitbang framework code
 * spi->controller_data ... holds chipselect GPIO
 *
 * spi->master->dev.driver_data ... points to spi_gpio->bitbang
 */

struct spi_gpio {
	struct spi_bitbang		bitbang;
	struct spi_gpio_platform_data	pdata;
	struct platform_device		*pdev;
};

/*
 * FIRST PART (OPTIONAL):  word-at-a-time spi_transfer support.
 * Use this for GPIO or shift-register level hardware APIs.
 *
 * spi_bitbang_cs is in spi_device->controller_state, which is unavailable
 * to glue code.  These bitbang setup() and cleanup() routines are always
 * used, though maybe they're called from controller-aware code.
 *
 * chipselect() and friends may use use spi_device->controller_data and
 * controller registers as appropriate.
 *
 *
 * NOTE:  SPI controller pins can often be used as GPIO pins instead,
 * which means you could use a bitbang driver either to get hardware
 * working quickly, or testing for differences that aren't speed related.
 */

struct spi_bitbang_cs {
	unsigned	nsecs;	/* (clock cycle time)/2 */
	u32		(*txrx_word)(struct spi_device *spi, unsigned nsecs,
					u32 word, u8 bits);
	unsigned	(*txrx_bufs)(struct spi_device *,
					u32 (*txrx_word)(
						struct spi_device *spi,
						unsigned nsecs,
						u32 word, u8 bits),
					unsigned, struct spi_transfer *);
};

/*----------------------------------------------------------------------*/

/*
 * Because the overhead of going through four GPIO procedure calls
 * per transferred bit can make performance a problem, this code
 * is set up so that you can use it in either of two ways:
 *
 *   - The slow generic way:  set up platform_data to hold the GPIO
 *     numbers used for MISO/MOSI/SCK, and issue procedure calls for
 *     each of them.  This driver can handle several such busses.
 *
 *   - The quicker inlined way:  only helps with platform GPIO code
 *     that inlines operations for constant GPIOs.  This can give
 *     you tight (fast!) inner loops, but each such bus needs a
 *     new driver.  You'll define a new C file, with Makefile and
 *     Kconfig support; the C code can be a total of six lines:
 *
 *		#define DRIVER_NAME	"myboard_spi2"
 *		#define	SPI_MISO_GPIO	119
 *		#define	SPI_MOSI_GPIO	120
 *		#define	SPI_SCK_GPIO	121
 *		#define	SPI_N_CHIPSEL	4
 *		#include "spi-gpio.c"
 */

#ifndef DRIVER_NAME
#define DRIVER_NAME	"spi_cs48l1x"

#define GENERIC_BITBANG	/* vs tight inlines */

/* all functions referencing these symbols must define pdata */
#define SPI_MISO_GPIO	((pdata)->miso)
#define SPI_MOSI_GPIO	((pdata)->mosi)
#define SPI_SCK_GPIO	((pdata)->sck)

#define SPI_N_CHIPSEL	((pdata)->num_chipselect)

#endif

/*----------------------------------------------------------------------*/

static inline const struct spi_gpio_platform_data * __pure
spi_to_pdata(const struct spi_device *spi)
{
	const struct spi_bitbang	*bang;
	const struct spi_gpio		*spi_gpio;

	bang = spi_master_get_devdata(spi->master);
	spi_gpio = container_of(bang, struct spi_gpio, bitbang);
	return &spi_gpio->pdata;
}

/* this is #defined to avoid unused-variable warnings when inlining */
#define pdata		spi_to_pdata(spi)

static inline void setsck(const struct spi_device *spi, int is_on)
{
	gpio_set_value(SPI_SCK_GPIO, is_on);
}

static inline void setmosi(const struct spi_device *spi, int is_on)
{
	gpio_set_value(SPI_MOSI_GPIO, is_on);
}

static inline int getmiso(const struct spi_device *spi)
{
	return !!gpio_get_value(SPI_MISO_GPIO);
}

#undef pdata

/*
 * NOTE:  this clocks "as fast as we can".  It "should" be a function of the
 * requested device clock.  Software overhead means we usually have trouble
 * reaching even one Mbit/sec (except when we can inline bitops), so for now
 * we'll just assume we never need additional per-bit slowdowns.
 */
//#define spidelay(nsecs)	do {} while (0)
#define spidelay(nsecs)		cs48l1x_delay(nsecs)	

extern int cs48l1x_suspended;

void cs48l1x_delay(unsigned long nsecs) 
{
	if (cs48l1x_suspended) {
		return;
	} else {
		ndelay(nsecs);
	}
}

#include "spi-bitbang-txrx.h"

/*
 * These functions can leverage inline expansion of GPIO calls to shrink
 * costs for a txrx bit, often by factors of around ten (by instruction
 * count).  That is particularly visible for larger word sizes, but helps
 * even with default 8-bit words.
 *
 * REVISIT overheads calling these functions for each word also have
 * significant performance costs.  Having txrx_bufs() calls that inline
 * the txrx_word() logic would help performance, e.g. on larger blocks
 * used with flash storage or MMC/SD.  There should also be ways to make
 * GCC be less stupid about reloading registers inside the I/O loops,
 * even without inlined GPIO calls; __attribute__((hot)) on GCC 4.3?
 */

static u32 spi_gpio_txrx_word_mode0(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode1(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha1(spi, nsecs, 0, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode2(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha0(spi, nsecs, 1, 0, word, bits);
}

static u32 spi_gpio_txrx_word_mode3(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	return bitbang_txrx_be_cpha1(spi, nsecs, 1, 0, word, bits);
}

/*
 * These functions do not call setmosi or getmiso if respective flag
 * (SPI_MASTER_NO_RX or SPI_MASTER_NO_TX) is set, so they are safe to
 * call when such pin is not present or defined in the controller.
 * A separate set of callbacks is defined to get highest possible
 * speed in the generic case (when both MISO and MOSI lines are
 * available), as optimiser will remove the checks when argument is
 * constant.
 */

static u32 spi_gpio_spec_txrx_word_mode0(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	unsigned flags = spi->master->flags;
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode1(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	unsigned flags = spi->master->flags;
	return bitbang_txrx_be_cpha1(spi, nsecs, 0, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode2(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	unsigned flags = spi->master->flags;
	return bitbang_txrx_be_cpha0(spi, nsecs, 1, flags, word, bits);
}

static u32 spi_gpio_spec_txrx_word_mode3(struct spi_device *spi,
		unsigned nsecs, u32 word, u8 bits)
{
	unsigned flags = spi->master->flags;
	return bitbang_txrx_be_cpha1(spi, nsecs, 1, flags, word, bits);
}

/*----------------------------------------------------------------------*/
static unsigned bitbang_txrx_8(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t
) {
	unsigned		bits = t->bits_per_word ? : spi->bits_per_word;
	unsigned		count = t->len;
	const u8		*tx = t->tx_buf;
	u8			*rx = t->rx_buf;

	while (likely(count > 0)) {
		u8		word = 0;

		if (tx) 
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx)
			*rx++ = word;
		count -= 1;
	}
	return t->len - count;
}

static unsigned bitbang_txrx_16(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t
) {
	unsigned		bits = t->bits_per_word ? : spi->bits_per_word;
	unsigned		count = t->len;
	const u16		*tx = t->tx_buf;
	u16			*rx = t->rx_buf;

	while (likely(count > 1)) {
		u16		word = 0;

		if (tx)
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx)
			*rx++ = word;
		count -= 2;
	}
	return t->len - count;
}

static unsigned bitbang_txrx_32(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t
) {
	unsigned		bits = t->bits_per_word ? : spi->bits_per_word;
	unsigned		count = t->len;
	const u32		*tx = t->tx_buf;
	u32			*rx = t->rx_buf;

	while (likely(count > 3)) {
		u32		word = 0;

		if (tx)
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx)
			*rx++ = word;
		count -= 4;
	}
	return t->len - count;
}

static unsigned bitbang_fc_txrx_32(
	struct spi_device	*spi,
	u32			(*txrx_word)(struct spi_device *spi,
					unsigned nsecs,
					u32 word, u8 bits),
	unsigned		ns,
	struct spi_transfer	*t) 
{
	struct spi_gpio_ext_data *ext = spi->controller_data;
	unsigned		bits = 32;
	unsigned		count = t->len;
	const u32		*tx = t->tx_buf;
	u32			*rx = t->rx_buf;
	int 			timeout = 0;

	while (likely(count > 3)) {
		u32		word = 0;

		if (tx) 
			word = *tx++;
		word = txrx_word(spi, ns, word, bits);
		if (rx) 
			*rx++ = word;
		count -= 4;

		if (tx && count) {
			while (!gpio_get_value(ext->busy)) {
				if (timeout >= 1000)
					return -ETIMEDOUT;
				mdelay(1);
				timeout++;
			}
		}

		if (rx) {
			if (gpio_get_value(spi->irq)) 
				break;
		}
	}
	return t->len - count;
}

static int wait = 0;

static irqreturn_t spi_ext_isr(int irq, void *dev_id)
{
	struct spi_device *dev = dev_id;
	struct spi_gpio_ext_data *ext = dev->controller_data;

	disable_irq_nosync(gpio_to_irq(dev->irq));

	wake_up_interruptible(&(ext->rx_wait_q));
	wait = 1;

	return IRQ_HANDLED;
}

static void spi_cs48l1x_set_mode(struct spi_device *spi, int mode)
{
	struct spi_bitbang	*bitbang;
	struct spi_gpio_ext_data *ext = spi->controller_data;
	unsigned		nsecs = 100;
	
	bitbang = spi_master_get_devdata(spi->master);
	spin_lock(&ext->lock);
	if (mode == 1) {
		ext->mode &= (CS_MODE_CONTINUOUS_WRITE | CS_IN_CONTINUOUS_WRITING);
		ext->mode |= CS_MODE_CONTINUOUS_WRITE;

		ndelay(nsecs);
		bitbang->chipselect(spi, BITBANG_CS_ACTIVE);
		ndelay(nsecs);
	} else {
		ext->mode &= ~(CS_MODE_CONTINUOUS_WRITE | CS_IN_CONTINUOUS_WRITING);
		
		ndelay(nsecs);
		bitbang->chipselect(spi, BITBANG_CS_INACTIVE);
		ndelay(nsecs);
	}
	spin_unlock(&ext->lock);
}

int spi_cs48l1x_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct spi_bitbang_cs	*cs = spi->controller_state;
	u8			bits_per_word;
	u32			hz;

	if (t) {
		bits_per_word = t->bits_per_word;
		hz = t->speed_hz;
	} else {
		bits_per_word = 0;
		hz = 0;
	}

	/* spi_transfer level calls that work per-word */
	if (!bits_per_word)
		bits_per_word = spi->bits_per_word;
	if (bits_per_word <= 8)
		cs->txrx_bufs = bitbang_txrx_8;
	else if (bits_per_word <= 16)
		cs->txrx_bufs = bitbang_txrx_16;
	else if (bits_per_word <= 32)
		cs->txrx_bufs = bitbang_txrx_32;
	else
		return -EINVAL;

	/* nsecs = (clock period)/2 */
	if (!hz)
		hz = spi->max_speed_hz;
	if (hz) {
		cs->nsecs = (1000000000/2) / hz;
		if (cs->nsecs > (MAX_UDELAY_MS * 1000 * 1000))
			return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(spi_cs48l1x_setup_transfer);

/**
 * spi_cs48l1x_setup - default setup for per-word I/O loops
 */
int spi_cs48l1x_setup(struct spi_device *spi)
{
	struct spi_gpio_ext_data *ext = spi->controller_data;
	struct spi_bitbang_cs	*cs = spi->controller_state;
	struct spi_bitbang	*bitbang;
	int			retval;
	unsigned long		flags;

	bitbang = spi_master_get_devdata(spi->master);

	if (!cs) {
		cs = kzalloc(sizeof *cs, GFP_KERNEL);
		if (!cs)
			return -ENOMEM;
		spi->controller_state = cs;
	}

	/* per-word shift register access, in hardware or bitbanging */
	cs->txrx_word = bitbang->txrx_word[spi->mode & (SPI_CPOL|SPI_CPHA)];
	if (!cs->txrx_word)
		return -EINVAL;

	retval = bitbang->setup_transfer(spi, NULL);
	if (retval < 0)
		return retval;

	dev_dbg(&spi->dev, "%s, %u nsec/bit\n", __func__, 2 * cs->nsecs);

	/* NOTE we _need_ to call chipselect() early, ideally with adapter
	 * setup, unless the hardware defaults cooperate to avoid confusion
	 * between normal (active low) and inverted chipselects.
	 */

	/* deselect chip (low or high) */
	spin_lock_irqsave(&bitbang->lock, flags);
	if (!bitbang->busy) {
		bitbang->chipselect(spi, BITBANG_CS_INACTIVE);
		ndelay(cs->nsecs);
	}
	spin_unlock_irqrestore(&bitbang->lock, flags);

	init_waitqueue_head(&ext->rx_wait_q);
	spin_lock_init(&ext->lock);
	ext->set_mode = spi_cs48l1x_set_mode;
	
	tcc_gpio_config_ext_intr(gpio_to_irq(spi->irq), EXTINT_GPIOC_14);
	
	retval = request_irq(gpio_to_irq(spi->irq), spi_ext_isr, IRQF_SHARED, dev_name(&(spi->dev)), spi);
	if (retval)
		return retval;
	disable_irq(gpio_to_irq(spi->irq));

	return 0;
}
EXPORT_SYMBOL_GPL(spi_cs48l1x_setup);

/**
 * spi_cs48l1x_cleanup - default cleanup for per-word I/O loops
 */
void spi_cs48l1x_cleanup(struct spi_device *spi)
{
	kfree(spi->controller_state);
}
EXPORT_SYMBOL_GPL(spi_cs48l1x_cleanup);

static int spi_bitbang_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct spi_bitbang_cs	*cs = spi->controller_state;
	struct spi_gpio_ext_data *ext = spi->controller_data;
	unsigned		nsecs = cs->nsecs;

	if (t->rx_buf) {	
		struct spi_transfer	tx;
	
		memset(&tx, 0, sizeof(struct spi_transfer));
		tx.tx_buf = &ext->read;
		tx.len = 1;
		bitbang_txrx_8(spi, cs->txrx_word, nsecs, &tx);
	}

	if (t->tx_buf) {
		if (!(ext->mode & CS_IN_CONTINUOUS_WRITING)) {
			struct spi_transfer	tx;

			memset(&tx, 0, sizeof(struct spi_transfer));
			tx.tx_buf = &ext->write;
			tx.len = 1;
			bitbang_txrx_8(spi, cs->txrx_word, nsecs, &tx);

			if ((ext->mode & CS_MODE_CONTINUOUS_WRITE)
					&& !(ext->mode & CS_IN_CONTINUOUS_WRITING)) {
				spin_lock(&ext->lock);
				ext->mode |= CS_IN_CONTINUOUS_WRITING;
				spin_unlock(&ext->lock);
			}
		}
	}

	return bitbang_fc_txrx_32(spi, cs->txrx_word, nsecs, t);
}

/*----------------------------------------------------------------------*/

/*
 * SECOND PART ... simple transfer queue runner.
 *
 * This costs a task context per controller, running the queue by
 * performing each transfer in sequence.  Smarter hardware can queue
 * several DMA transfers at once, and process several controller queues
 * in parallel; this driver doesn't match such hardware very well.
 *
 * Drivers can provide word-at-a-time i/o primitives, or provide
 * transfer-at-a-time ones to leverage dma or fifo hardware.
 */
static void bitbang_start(struct spi_bitbang *bitbang, struct spi_message *m)
{
	struct spi_device	*spi;
	unsigned		nsecs;
	struct spi_transfer	*t = NULL;
	unsigned		tmp;
	unsigned		cs_change;
	int			status;
	int			do_setup = -1;
	struct spi_gpio_ext_data *ext;

	bitbang->busy = 1;

	/* FIXME this is made-up ... the correct value is known to
	 * word-at-a-time bitbang code, and presumably chipselect()
	 * should enforce these requirements too?
	 */
	nsecs = 100;

	spi = m->spi;
	ext = spi->controller_data;
	tmp = 0;
	cs_change = 1;
	status = 0;
			
	if (ext->mode & CS_MODE_CONTINUOUS_WRITE) {
		cs_change = 0;
	}

	list_for_each_entry (t, &m->transfers, transfer_list) {
		if (t->rx_buf) {
			enable_irq(gpio_to_irq(spi->irq));
//			status = wait_event_interruptible_timeout(ext->rx_wait_q, wait, HZ * 2);
			status = wait_event_interruptible(ext->rx_wait_q, wait);
			if (status == -ERESTARTSYS) {
//			if (status == 0) {
				disable_irq(gpio_to_irq(spi->irq));
//				status = -ETIMEDOUT;
				break;
			}
			wait = 0;
		}

		/* override speed or wordsize? */
		if (t->speed_hz || t->bits_per_word)
			do_setup = 1;

		/* init (-1) or override (1) transfer params */
		if (do_setup != 0) {
			status = bitbang->setup_transfer(spi, t);
			if (status < 0)
				break;
			if (do_setup == -1)
				do_setup = 0;
		}

		/* set up default clock polarity, and activate chip;
		 * this implicitly updates clock and spi modes as
		 * previously recorded for this device via setup().
		 * (and also deselects any other chip that might be
		 * selected ...)
		 */
		if (!(ext->mode & CS_MODE_CONTINUOUS_WRITE)) {
			bitbang->chipselect(spi, BITBANG_CS_ACTIVE);
			ndelay(nsecs);
		}

		if (!t->tx_buf && !t->rx_buf && t->len) {
			status = -EINVAL;
			break;
		}

		/* transfer data.  the lower level code handles any
		 * new dma mappings it needs. our caller always gave
		 * us dma-safe buffers.
		 */
		if (t->len) {
			/* REVISIT dma API still needs a designated
			 * DMA_ADDR_INVALID; ~0 might be better.
			 */
			if (!m->is_dma_mapped)
				t->rx_dma = t->tx_dma = 0;
			status = bitbang->txrx_bufs(spi, t);
		}
		if (status > 0)
			m->actual_length += status;
		else if (status < 0)
			break;

		status = 0;

		/* protocol tweaks before next transfer */
		if (t->delay_usecs)
			udelay(t->delay_usecs);

		/* sometimes a short mid-message deselect of the chip
		 * may be needed to terminate a mode or command
		 */

		if (!(ext->mode & CS_MODE_CONTINUOUS_WRITE)) {
			ndelay(nsecs);
			bitbang->chipselect(spi, BITBANG_CS_INACTIVE);
			ndelay(nsecs);
		}
	}

	m->status = status;
	m->complete(m->context);

	bitbang->busy = 0;
}

/**
 * spi_cs48l1x_transfer - default submit to transfer queue
 */
int spi_cs48l1x_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct spi_bitbang	*bitbang;
	int			status = 0;

	m->actual_length = 0;
	m->status = -EINPROGRESS;

	bitbang = spi_master_get_devdata(spi->master);

	if (!spi->max_speed_hz)
		status = -ENETDOWN;
	else 
		bitbang_start(bitbang, m);

	return status;
}
EXPORT_SYMBOL_GPL(spi_cs48l1x_transfer);

/*----------------------------------------------------------------------*/

/**
 * spi_cs48l1x_start - start up a polled/bitbanging SPI master driver
 * @bitbang: driver handle
 *
 * Caller should have zero-initialized all parts of the structure, and then
 * provided callbacks for chip selection and I/O loops.  If the master has
 * a transfer method, its final step should call spi_cs48l1x_transfer; or,
 * that's the default if the transfer routine is not initialized.  It should
 * also set up the bus number and number of chipselects.
 *
 * For i/o loops, provide callbacks either per-word (for bitbanging, or for
 * hardware that basically exposes a shift register) or per-spi_transfer
 * (which takes better advantage of hardware like fifos or DMA engines).
 *
 * Drivers using per-word I/O loops should use (or call) spi_cs48l1x_setup,
 * spi_cs48l1x_cleanup and spi_cs48l1x_setup_transfer to handle those spi
 * master methods.  Those methods are the defaults if the bitbang->txrx_bufs
 * routine isn't initialized.
 *
 * This routine registers the spi_master, which will process requests in a
 * dedicated task, keeping IRQs unblocked most of the time.  To stop
 * processing those requests, call spi_cs48l1x_stop().
 */
int spi_cs48l1x_start(struct spi_bitbang *bitbang)
{
	int	status;

	if (!bitbang->master || !bitbang->chipselect)
		return -EINVAL;

	spin_lock_init(&bitbang->lock);
	INIT_LIST_HEAD(&bitbang->queue);

	if (!bitbang->master->mode_bits)
		bitbang->master->mode_bits = SPI_CPOL | SPI_CPHA | bitbang->flags;

	if (!bitbang->master->transfer)
		bitbang->master->transfer = spi_cs48l1x_transfer;
	if (!bitbang->txrx_bufs) {
		bitbang->use_dma = 0;
		bitbang->txrx_bufs = spi_bitbang_bufs;
		if (!bitbang->master->setup) {
			if (!bitbang->setup_transfer)
				bitbang->setup_transfer =
					 spi_cs48l1x_setup_transfer;
			bitbang->master->setup = spi_cs48l1x_setup;
			bitbang->master->cleanup = spi_cs48l1x_cleanup;
		}
	} else if (!bitbang->master->setup)
		return -EINVAL;
	if (bitbang->master->transfer == spi_cs48l1x_transfer &&
			!bitbang->setup_transfer)
		return -EINVAL;

	/* this task is the only thing to touch the SPI bits */
	bitbang->busy = 0;
	bitbang->workqueue = create_singlethread_workqueue(
			dev_name(bitbang->master->dev.parent));
	if (bitbang->workqueue == NULL) {
		status = -EBUSY;
		goto err1;
	}

	/* driver may get busy before register() returns, especially
	 * if someone registered boardinfo for devices
	 */
	status = spi_register_master(bitbang->master);
	if (status < 0)
		goto err2;
	
	return status;

err2:
	destroy_workqueue(bitbang->workqueue);
err1:
	return status;
}
EXPORT_SYMBOL_GPL(spi_cs48l1x_start);

/**
 * spi_cs48l1x_stop - stops the task providing spi communication
 */
int spi_cs48l1x_stop(struct spi_bitbang *bitbang)
{
	spi_unregister_master(bitbang->master);

	WARN_ON(!list_empty(&bitbang->queue));

	destroy_workqueue(bitbang->workqueue);

	return 0;
}
EXPORT_SYMBOL_GPL(spi_cs48l1x_stop);

static void spi_gpio_chipselect(struct spi_device *spi, int is_active)
{
	struct spi_gpio_ext_data *ext = spi->controller_data;
	unsigned long cs = (unsigned long) ext->cs;
	/* set initial clock polarity */
	if (is_active)
		setsck(spi, spi->mode & SPI_CPOL);

	if (cs != SPI_GPIO_NO_CHIPSELECT) {
		/* SPI is normally active-low */
		gpio_set_value(cs, (spi->mode & SPI_CS_HIGH) ? is_active : !is_active);
	}
}

static int spi_gpio_setup(struct spi_device *spi)
{
	struct spi_gpio_ext_data *ext = (struct spi_gpio_ext_data*)spi->controller_data;
	unsigned long	cs = (unsigned long) ext->cs;
	int		status = 0;
	volatile PPIC pPIC = (volatile PPIC)tcc_p2v(HwPIC_BASE);

	if (spi->bits_per_word > 32)
		return -EINVAL;

	if (!spi->controller_state) {
		if (cs != SPI_GPIO_NO_CHIPSELECT) {
			status = gpio_request(cs, dev_name(&spi->dev));
			if (status)
				return status;
			status = gpio_direction_output(cs, spi->mode & SPI_CS_HIGH);
		}
	}
	if (!status)
		status = spi_cs48l1x_setup(spi);
	if (status) {
		if (!spi->controller_state && cs != SPI_GPIO_NO_CHIPSELECT)
			gpio_free(cs);
	}

	/* makes a interrupt pin active low */
	pPIC->POL0.bREG.EINT7 = 1;

	return status;
}

static void spi_gpio_cleanup(struct spi_device *spi)
{
	struct spi_gpio_ext_data *ext = (struct spi_gpio_ext_data*)spi->controller_data;
	unsigned long	cs = (unsigned long) ext->cs;

	if (cs != SPI_GPIO_NO_CHIPSELECT)
		gpio_free(cs);
	spi_cs48l1x_cleanup(spi);
}

static int __init spi_gpio_alloc(unsigned pin, const char *label, bool is_in)
{
	int value;

	value = gpio_request(pin, label);
	if (value == 0) {
		if (is_in)
			value = gpio_direction_input(pin);
		else
			value = gpio_direction_output(pin, 0);
	}
	return value;
}

static int __init
spi_gpio_request(struct spi_gpio_platform_data *pdata, const char *label,
	u16 *res_flags)
{
	int value;

	/* NOTE:  SPI_*_GPIO symbols may reference "pdata" */

	if (SPI_MOSI_GPIO != SPI_GPIO_NO_MOSI) {
		value = spi_gpio_alloc(SPI_MOSI_GPIO, label, false);
		if (value)
			goto done;
	} else {
		/* HW configuration without MOSI pin */
		*res_flags |= SPI_MASTER_NO_TX;
	}

	if (SPI_MISO_GPIO != SPI_GPIO_NO_MISO) {
		value = spi_gpio_alloc(SPI_MISO_GPIO, label, true);
		if (value)
			goto free_mosi;
	} else {
		/* HW configuration without MISO pin */
		*res_flags |= SPI_MASTER_NO_RX;
	}

	value = spi_gpio_alloc(SPI_SCK_GPIO, label, false);
	if (value)
		goto free_miso;

	goto done;

free_miso:
	if (SPI_MISO_GPIO != SPI_GPIO_NO_MISO)
		gpio_free(SPI_MISO_GPIO);
free_mosi:
	if (SPI_MOSI_GPIO != SPI_GPIO_NO_MOSI)
		gpio_free(SPI_MOSI_GPIO);
done:
	return value;
}

static int __init spi_gpio_probe(struct platform_device *pdev)
{
	int				status;
	struct spi_master		*master;
	struct spi_gpio			*spi_gpio;
	struct spi_gpio_platform_data	*pdata;
	u16 master_flags = 0;

	pdata = pdev->dev.platform_data;
#ifdef GENERIC_BITBANG
	if (!pdata || !pdata->num_chipselect)
		return -ENODEV;
#endif

	status = spi_gpio_request(pdata, dev_name(&pdev->dev), &master_flags);
	if (status < 0)
		return status;

	master = spi_alloc_master(&pdev->dev, sizeof *spi_gpio);
	if (!master) {
		status = -ENOMEM;
		goto gpio_free;
	}
	spi_gpio = spi_master_get_devdata(master);
	platform_set_drvdata(pdev, spi_gpio);

	spi_gpio->pdev = pdev;
	if (pdata)
		spi_gpio->pdata = *pdata;

	master->flags = master_flags;
	master->bus_num = pdev->id;
	master->num_chipselect = SPI_N_CHIPSEL;
	master->setup = spi_gpio_setup;
	master->cleanup = spi_gpio_cleanup;

	spi_gpio->bitbang.master = spi_master_get(master);
	spi_gpio->bitbang.chipselect = spi_gpio_chipselect;

	if ((master_flags & (SPI_MASTER_NO_TX | SPI_MASTER_NO_RX)) == 0) {
		spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_txrx_word_mode0;
		spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_txrx_word_mode1;
		spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_txrx_word_mode2;
		spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_txrx_word_mode3;
	} else {
		spi_gpio->bitbang.txrx_word[SPI_MODE_0] = spi_gpio_spec_txrx_word_mode0;
		spi_gpio->bitbang.txrx_word[SPI_MODE_1] = spi_gpio_spec_txrx_word_mode1;
		spi_gpio->bitbang.txrx_word[SPI_MODE_2] = spi_gpio_spec_txrx_word_mode2;
		spi_gpio->bitbang.txrx_word[SPI_MODE_3] = spi_gpio_spec_txrx_word_mode3;
	}
	spi_gpio->bitbang.setup_transfer = spi_cs48l1x_setup_transfer;
	spi_gpio->bitbang.flags = SPI_CS_HIGH;

	status = spi_cs48l1x_start(&spi_gpio->bitbang);
	if (status < 0) {
		spi_master_put(spi_gpio->bitbang.master);
gpio_free:
		if (SPI_MISO_GPIO != SPI_GPIO_NO_MISO)
			gpio_free(SPI_MISO_GPIO);
		if (SPI_MOSI_GPIO != SPI_GPIO_NO_MOSI)
			gpio_free(SPI_MOSI_GPIO);
		gpio_free(SPI_SCK_GPIO);
		spi_master_put(master);
	}

	return status;
}

static int __exit spi_gpio_remove(struct platform_device *pdev)
{
	struct spi_gpio			*spi_gpio;
	struct spi_gpio_platform_data	*pdata;
	int				status;

	spi_gpio = platform_get_drvdata(pdev);
	pdata = pdev->dev.platform_data;

	/* stop() unregisters child devices too */
	status = spi_cs48l1x_stop(&spi_gpio->bitbang);
	spi_master_put(spi_gpio->bitbang.master);

	platform_set_drvdata(pdev, NULL);

	if (SPI_MISO_GPIO != SPI_GPIO_NO_MISO)
		gpio_free(SPI_MISO_GPIO);
	if (SPI_MOSI_GPIO != SPI_GPIO_NO_MOSI)
		gpio_free(SPI_MOSI_GPIO);
	gpio_free(SPI_SCK_GPIO);

	return status;
}

MODULE_ALIAS("platform:" DRIVER_NAME);

static struct platform_driver spi_gpio_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.remove		= __exit_p(spi_gpio_remove),
};

static int __init spi_gpio_init(void)
{
	return platform_driver_probe(&spi_gpio_driver, spi_gpio_probe);
}
module_init(spi_gpio_init);

static void __exit spi_gpio_exit(void)
{
	platform_driver_unregister(&spi_gpio_driver);
}
module_exit(spi_gpio_exit);


MODULE_DESCRIPTION("SPI master driver using generic bitbanged GPIO ");
MODULE_AUTHOR("David Brownell");
MODULE_LICENSE("GPL");
