/*
 * Device-mapper, epath
 *
 * Copyright (C) 2014 Jungmo Ahn <jman@elixirflash.com>
 *
 * This file is released under the GPL.
 */

#include "dm-epath.h"

static mempool_t *bio_split_pool __read_mostly;

sector_t dm_devsize(struct dm_dev *dev)
{
	return i_size_read(dev->bdev->bd_inode) >> SECTOR_SHIFT;
}

static void bio_remap(struct bio *bio, struct dm_dev *dev, sector_t sector)
{
	bio->bi_bdev = dev->bdev;
	bio->bi_sector = sector;
}

static void bio_pair_end_1(struct bio *bi, int err)
{
	struct bio_pair *bp = container_of(bi, struct bio_pair, bio1);

	if (err)
		bp->error = err;

	bio_pair_release(bp);
}

static void bio_pair_end_2(struct bio *bi, int err)
{
	struct bio_pair *bp = container_of(bi, struct bio_pair, bio2);

	if (err)
		bp->error = err;

	bio_pair_release(bp);
}

struct bio_pair *epath_bio_split(struct bio *bi, sector_t slave_start_sector)
{
	struct bio_pair *bp;
	struct bio_vec *bv;
        struct bio *bio1;
        int nbytes;
	unsigned idx;

        if(!(bi->bi_sector < slave_start_sector &&
             slave_start_sector <= bi->bi_sector + bio_sectors(bi) - 1))        
                return NULL;

        bp = mempool_alloc(bio_split_pool, GFP_NOIO);                        

	if (!bp)
		return bp;

	atomic_set(&bp->cnt, 3);
	bp->error = 0;
	bp->bio1 = *bi;
        bp->bio2 = *bi;
	bp->bio2.bi_sector = slave_start_sector;;                
	bp->bio2.bi_size = ((bi->bi_sector << 9) + bi->bi_size) - (slave_start_sector << 9);        
	bp->bio1.bi_size = (slave_start_sector - bi->bi_sector) << 9;

        BUG_ON(bp->bio1.bi_size % PAGE_SIZE);
        BUG_ON(bp->bio2.bi_size % PAGE_SIZE);        

        bp->bv1 = *bio_iovec(bi);
        bp->bv2 = *bio_iovec(bi);
        
        nbytes = bp->bio1.bi_size;
        idx = bp->bio1.bi_idx;
        bio1 = &bp->bio1;

	bio_for_each_segment(bv, bio1, idx) {
		if (nbytes) {
                        nbytes -= bv->bv_len;
                } else {
                        bp->bio2.bi_idx = idx;
                        bp->bio1.bi_vcnt = idx;
                        break;
                }
        }

	bp->bio1.bi_end_io = bio_pair_end_1;
	bp->bio2.bi_end_io = bio_pair_end_2;

	bp->bio1.bi_private = bi;
	bp->bio2.bi_private = bio_split_pool;

	if (bio_integrity(bi))
		bio_integrity_split(bi, bp, slave_start_sector);

	return bp;
}
EXPORT_SYMBOL(epath_bio_split);

static int _epath_map(struct dm_target *ti, struct bio *bio)
{
	struct ep_master *master = ti->private;
	struct ep_slave *slave = master->slave;
	sector_t slave_start_sector = master->nr_sects;
        sector_t slave_bi_sector;
        int path_select = IO_TO_MASTER;

        if(bio->bi_sector >= slave_start_sector) {
                slave_bi_sector = bio->bi_sector - slave_start_sector;
                path_select = IO_TO_SLAVE;
        }

	if (bio->bi_rw & REQ_DISCARD || bio->bi_rw & REQ_FUA) {
                if(path_select == IO_TO_SLAVE) 
                        bio_remap(bio, slave->device, slave_bi_sector);
                else                   
                        bio_remap(bio, master->device, bio->bi_sector);                

                return DM_MAPIO_REMAPPED;                
        }

	if (bio->bi_rw & REQ_FLUSH) {
   		BUG_ON(bio->bi_size);

                if(slave)                
                        bio_remap(bio, slave->device, 0);
                bio_remap(bio, master->device, 0);                        
                        
		return DM_MAPIO_REMAPPED;
        }

        if(path_select == IO_TO_SLAVE) {
                bio_remap(bio, slave->device, slave_bi_sector);
                return DM_MAPIO_REMAPPED;
        }

        bio_remap(bio, master->device, bio->bi_sector);
        return DM_MAPIO_REMAPPED;        
}

static int epath_map(struct dm_target *ti, struct bio *bio, union map_info *map_context)
{
	struct ep_master *master = ti->private;        
        struct bio_pair *bp;
	sector_t slave_start_sector = master->nr_sects;

        bp = epath_bio_split(bio, slave_start_sector);

        if(bp) {
                _epath_map(ti, &bp->bio1);
                generic_make_request(&bp->bio1);
                
                _epath_map(ti, &bp->bio2);
                generic_make_request(&bp->bio2);                

                bio_pair_release(bp);
                
                return DM_MAPIO_SUBMITTED;                
        } else 
                return _epath_map(ti, bio);
}

static int epath_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	int r = 0;
	struct ep_master *master;
	struct ep_slave *slave;
	struct dm_dev *masterdev, *slavedev;

	master = kzalloc(sizeof(*master), GFP_KERNEL);
	if (!master) {
		EPERR("couldn't allocate master");
		return -ENOMEM;
	}

	slave = kzalloc(sizeof(*slave), GFP_KERNEL);
	if (!slave) {
		r = -ENOMEM;
		EPERR("couldn't allocate slave");
		goto bad_alloc_slave;
	}
	master->slave = slave;
	master->slave->master = master;

	r = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table),
			  &masterdev);
	if (r) {
		EPERR("couldn't get backing dev err(%d)", r);
		goto bad_get_device_orig;
	}
	master->device = masterdev;

        ARG_EXIST(1);        

	r = dm_get_device(ti, argv[1], dm_table_get_mode(ti->table),
			  &slavedev);
	if (r) {
		EPERR("couldn't get slave dev err(%d)", r);
		goto bad_get_device_slave;
	}

        slave->device = slavedev;

 exit_parse_arg:                
	master->ti = ti;
	ti->private = master;

	ti->discard_zeroes_data_unsupported = true;

	bio_split_pool = mempool_create_kmalloc_pool(BIO_SPLIT_ENTRIES,
						     sizeof(struct bio_pair));
	if (!bio_split_pool)
		panic("bio: can't create split pool\n");        

	return 0;

bad_get_device_slave:
	dm_put_device(ti, masterdev);
bad_get_device_orig:
	kfree(slave);
bad_alloc_slave:
	kfree(master);
	return r;
}

static void epath_dtr(struct dm_target *ti)
{
	struct ep_master *master = ti->private;
	struct ep_slave *slave = master->slave;

	kfree(slave);

	dm_put_device(master->ti, slave->device);
	dm_put_device(ti, master->device);

	ti->private = NULL;
	kfree(master);
}

static int epath_message(struct dm_target *ti, unsigned argc, char **argv)
{
	struct ep_master *master = ti->private;
	struct ep_slave *slave = master->slave;

	char *cmd = argv[0];
	unsigned long tmp;

	if (kstrtoul(argv[1], 10, &tmp))
		return -EINVAL;

	if (!strcasecmp(cmd, "slave")) {
		if (tmp < 1) 
			return -EINVAL;

		slave->nr_sects = tmp;
		return 0;
	}

	if (!strcasecmp(cmd, "master")) {
		if (tmp < 1)
			return -EINVAL;
                
		master->nr_sects = tmp;
		return 0;
	}                

	return -EINVAL;
}

static int epath_merge(struct dm_target *ti, struct bvec_merge_data *bvm,
			    struct bio_vec *biovec, int max_size)
{
	struct ep_master *master = ti->private;
	struct dm_dev *device = master->device;
	struct request_queue *q = bdev_get_queue(device->bdev);

	if (!q->merge_bvec_fn)
		return max_size;

	bvm->bi_bdev = device->bdev;
        
	return min(max_size, q->merge_bvec_fn(q, bvm, biovec));
}

static int epath_iterate_devices(struct dm_target *ti,
				      iterate_devices_callout_fn fn, void *data)
{
	struct ep_master *master = ti->private;
	struct dm_dev *orig = master->device;
	sector_t start = 0;
	sector_t len = dm_devsize(orig);
        
	return fn(ti, orig, start, len, data);
}

static struct target_type epath_target = {
	.name = "epath",
	.version = {1, 0, 2},
	.module = THIS_MODULE,
	.map = epath_map,
	.ctr = epath_ctr,
	.dtr = epath_dtr,
	.merge = epath_merge,
	.message = epath_message,
	.iterate_devices = epath_iterate_devices,
};

static int __init epath_module_init(void)
{
	int r = 0;

	r = dm_register_target(&epath_target);
	if (r < 0) {
		EPERR("%d", r);
		return r;
	}

	return 0;
}

static void __exit epath_module_exit(void)
{
	dm_unregister_target(&epath_target);
}

module_init(epath_module_init);
module_exit(epath_module_exit);

MODULE_AUTHOR("Jungmo Ahn <jman@elixirflash.com>");
MODULE_DESCRIPTION(DM_NAME "epath target");
MODULE_LICENSE("GPL");
