#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device-mapper.h>
#include <linux/bio.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/spinlock.h>
#include <linux/math.h>

#define DM_MSG_PREFIX "dmp"

struct dmp_statistic {
	unsigned long long read_reqs;
	unsigned long long write_reqs;
	unsigned long long read_bytes;
	unsigned long long write_bytes;
};

struct dmp_c {
	struct dm_dev *dev;
	// ...
};

static struct dmp_statistic dmp_stat;
static struct kobject *dmp_kobj;
static spinlock_t dmp_lock;

static ssize_t dmp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned long long read_reqs, write_reqs, read_bytes, write_bytes;
	unsigned long long read_avg, write_avg, total_reqs, total_avg;

	spin_lock(&dmp_lock);
	read_reqs = dmp_stat.read_reqs;
	write_reqs = dmp_stat.write_reqs;
	read_bytes = dmp_stat.read_bytes;
	write_bytes = dmp_stat.write_bytes;
	spin_unlock(&dmp_lock);

	read_avg = read_reqs != 0 ? DIV_ROUND_UP(read_bytes, read_reqs) : 0;
	write_avg = write_reqs != 0 ? DIV_ROUND_UP(write_bytes, write_reqs) : 0;
	total_reqs = read_reqs + write_reqs;
	total_avg = total_reqs != 0 ? DIV_ROUND_UP(read_bytes + write_bytes, total_reqs) : 0;

	return sprintf(buf, "read:\n\treqs: %llu\n\tavg size: %llu\n"
						"write:\n\treqs: %llu\n\tavg size: %llu\n"
						"total:\n\treqs: %llu\n\tavg size: %llu\n",
						read_reqs, read_avg, write_reqs, write_avg,
						total_reqs, total_avg);
}

static struct kobj_attribute dmp_stat_attr = __ATTR(volumes, 0660, dmp_show, NULL);

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
	struct dmp_c *mdt = (struct dmp_c *) ti->private;
	unsigned int size = bio->bi_iter.bi_size;

	spin_lock(&dmp_lock);
	switch (bio_op(bio))
	{
	case REQ_OP_READ:
		++dmp_stat.read_reqs;
		dmp_stat.read_bytes += size;
		break;

	case REQ_OP_WRITE:
		++dmp_stat.write_reqs;
		dmp_stat.write_bytes += size;
		break;

	default:
		DMCRIT("Unsupported bio operation");
		return DM_MAPIO_KILL;
	}
	spin_unlock(&dmp_lock);
	
	bio_set_dev(bio, mdt->dev->bdev);
	return DM_MAPIO_REMAPPED;
}

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	int ret = 0;
	struct dmp_c *mdt;

	if (argc != 1)
	{
		DMCRIT("Invalid number of arguments.");
		ti->error = "Invalid argument count";
		ret = -EINVAL;
		goto err;
	}

	mdt = kzalloc(sizeof(struct dmp_c), GFP_KERNEL);
	
	if (mdt == NULL)
	{
		DMCRIT("Failed to allocate memory for dmp_c");
		ti->error = "Failed to allocate memory";
		ret = -ENOMEM;
		goto err;
	}

	if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &mdt->dev) < 0)
	{
		DMCRIT("Failed to lookup device");
		ti->error = "Failed to lookup device";
		kfree(mdt);
		ret = -EINVAL;
		goto err;
	}

	ti->private = mdt;

err:
	return ret;
}

static void dmp_dtr(struct dm_target *ti)
{
	struct dmp_c *mdt = (struct dmp_c *) ti->private;
	dm_put_device(ti, mdt->dev);
	kfree(mdt);
}

static struct target_type dmp_target = {
	.name = "dmp",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr = dmp_ctr,
	.dtr = dmp_dtr,
	.map = dmp_map
};

static int __init dmp_init(void)
{
	int ret = 0;

	spin_lock_init(&dmp_lock);

	ret = dm_register_target(&dmp_target);
	if (ret < 0)
	{
		DMCRIT("Failed to register target");
		goto err;
	}

	dmp_kobj = kobject_create_and_add("stat", &THIS_MODULE->mkobj.kobj);
	if (dmp_kobj == NULL)
	{
		DMCRIT("Failed to create and add kobject");
		ret = -ENOMEM;
		goto err;
	}

	ret = sysfs_create_file(dmp_kobj, &dmp_stat_attr.attr);
	if (ret < 0)
	{
		kobject_put(dmp_kobj);
		DMCRIT("Failed to sysfs create file");
		goto err;
	}

	return ret;

err:
	dm_unregister_target(&dmp_target);
	return ret;
}

static void __exit dmp_exit(void)
{
	kobject_put(dmp_kobj);
	dm_unregister_target(&dmp_target);
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_AUTHOR("Artem Didenko <icxcmchnk@gmail.com>");
MODULE_DESCRIPTION(DM_NAME " proxy target for collecting I/O statistical data");
MODULE_LICENSE("GPL");