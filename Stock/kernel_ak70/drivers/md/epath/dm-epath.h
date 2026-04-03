/*
 * Copyright (C) 2012-2013 Jungmo Ahn <jman@elixirflash.com>
 *
 * This file is released under the GPL.
 */

#ifndef DM_EPATH_H
#define DM_EPATH_H

/*----------------------------------------------------------------*/

#define DM_MSG_PREFIX "epath"

#include <linux/module.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/device-mapper.h>
#include <linux/dm-io.h>
#include <linux/bio.h>

#define epdebug(f, args...) \
	DMINFO("debug@%s() L.%d" f, __func__, __LINE__, ## args)

#define EPERR(f, args...) \
	DMERR("err@%s() " f, __func__, ## args)
#define EPWARN(f, args...) \
	DMWARN("warn@%s() " f, __func__, ## args)
#define EPINFO(f, args...) \
	DMINFO("info@%s() " f, __func__, ## args)

#define ARG_EXIST(n) { \
	if (argc <= (n)) { \
		goto exit_parse_arg; \
	} }

#define IO_TO_MASTER   0
#define IO_TO_SLAVE    1

struct ep_master;
struct ep_slave {
	struct ep_master *master;
	struct dm_dev *device;
	u64 nr_sects; /* Const */                
};

struct ep_master {
	struct dm_target *ti;
	struct dm_dev *device;
	struct ep_slave *slave;
	u64 nr_sects; /* Const */                        
};

sector_t dm_devsize(struct dm_dev *);

#endif
