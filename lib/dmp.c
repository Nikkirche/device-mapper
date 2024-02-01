
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>


struct my_dm_target
{
  struct dm_dev *dev;
};

struct kobject *mod_kobject;

typedef struct stat_field
{
  size_t count;
  size_t sum;
} stat_field;

// because it's globally var - no need to set fields to 0
stat_field read_stat, write_stat, summary_stat;

static ssize_t show_stat(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
  size_t rc = read_stat.count;
  size_t wc = write_stat.count;
  size_t sc = summary_stat.count;

  return sysfs_emit(buf, "read:\n\tregs: %ld\n\tavg_size: %ld\n"
                         "write:\n\tregs: %ld\n\tavg_size: %ld\n"
                         "total:\n\tregs: %ld\n\tavg_size: %ld\n",
                    rc, rc != 0 ? read_stat.sum / rc : 0,
                    wc, wc != 0 ? write_stat.sum / wc : 0,
                    sc, sc != 0 ? summary_stat.sum / sc : 0);
}

static ssize_t store_stat(struct device *dev, struct device_attribute *attr,
                          const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(volumes, S_IRUGO, show_stat, store_stat);

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
  struct my_dm_target *mdt = (struct my_dm_target *)ti->private;
  switch (bio_op(bio))
  {
  case REQ_OP_WRITE:
    write_stat.count++;
    write_stat.sum += bio_sectors(bio) * SECTOR_SIZE;
    break;
  case REQ_OP_READ:
    read_stat.count++;
    read_stat.sum += bio_sectors(bio) * SECTOR_SIZE;
    break;
  default:
    return DM_MAPIO_KILL;
  }
  bio_set_dev(bio, mdt->dev->bdev);
  submit_bio(bio);
  summary_stat.count++;
  summary_stat.sum += bio_sectors(bio) * SECTOR_SIZE;
  return DM_MAPIO_SUBMITTED;
}

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
  struct my_dm_target *mdt;

  if (argc != 1)
  {
    pr_info("\n Invalid no.of arguments.\n");
    ti->error = "Invalid argument count";
    return -EINVAL;
  }

  mdt = kmalloc(sizeof(struct my_dm_target), GFP_KERNEL);

  if (mdt == NULL)
  {
    return -ENOMEM;
  }

  if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &mdt->dev))
  {
    ti->error = "dm-basic_target: Device lookup failed";
    goto bad;
  }

  ti->private = mdt;

  return 0;
bad:
  kfree(mdt);
  printk(KERN_CRIT "\n>>out function basic_target_ctr with error \n");
  return -EINVAL;
}

static void dmp_dtr(struct dm_target *ti)
{
  struct my_dm_target *mdt = (struct my_dm_target *)ti->private;
  dm_put_device(ti, mdt->dev);
  kfree(mdt);
}

static struct target_type dmp = {

    .name = "dmp",
    .version = {1, 0, 0},
    .module = THIS_MODULE,
    .ctr = dmp_ctr,
    .dtr = dmp_dtr,
    .map = dmp_map,
};


static int init_dmp(void)
{
  int result;
  result = dm_register_target(&dmp);
  if (result < 0)
  {
    pr_info("\n Error in registering target \n");
    return -EINVAL;
  }
  mod_kobject = kobject_create_and_add("stat", &THIS_MODULE->mkobj.kobj);
  if (mod_kobject == NULL)
  {
    pr_err("Couldn't inizialize kobject\n");
    return -ENOMEM;
  }
  int r = sysfs_create_file(mod_kobject, &dev_attr_volumes.attr);
  if (r != 0)
  {
    pr_err("Couldn't create stat file\n");
    return r;
  }
  return 0;
}

static void cleanup_dmp(void)
{
  kobject_put(mod_kobject);
  dm_unregister_target(&dmp);
}

module_init(init_dmp);
module_exit(cleanup_dmp);

MODULE_AUTHOR("Nikolay Cherkashin");
MODULE_DESCRIPTION(DM_NAME "dummy proxy mapper");
MODULE_LICENSE("GPL");