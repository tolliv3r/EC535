// EC535 Lab 2

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/string.h>

#define DRV_NAME "mytimer"
#define MYTIMER_MAJOR 61
#define MYTIMER_MINOR 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackson Clary");
MODULE_DESCRIPTION("Kernel timers via /dev/mytimer");
MODULE_VERSION("1.0");

#define KTIMER_IOC_MAGIC 'k'
#define KTIMER_MAX 5
#define KTIMER_MSG_MAX 128

// set or update a timer
struct ktimer_set {
    __u32 sec;                   // 1..86400
    char  msg[KTIMER_MSG_MAX+1];
};

//  return values for SET ioctl:
//      0 = new timer created
//      1 = existing timer updated
#define KTIMER_IOC_SET  _IOW(KTIMER_IOC_MAGIC, 1, struct ktimer_set)

// list active timers
struct ktimer_entry {
    __u32 sec_remaining;
    char  msg[KTIMER_MSG_MAX+1];
};

struct ktimer_list {
    __u32 count;
    struct ktimer_entry entries[KTIMER_MAX];
};

#define KTIMER_IOC_LIST   _IOR(KTIMER_IOC_MAGIC, 2, struct ktimer_list)

// max supported active timers
#define KTIMER_IOC_SETMAX _IOW(KTIMER_IOC_MAGIC, 3, __u32)

// current maximum allowed timers
#define KTIMER_IOC_GETMAX _IOR(KTIMER_IOC_MAGIC, 4, __u32)


struct my_timer {
    struct timer_list t;
    bool active;
    unsigned long expires_j; // in jiffies
    char msg[KTIMER_MSG_MAX+1];
};

static dev_t devno = MKDEV(MYTIMER_MAJOR, MYTIMER_MINOR);
static struct cdev my_cdev;

static spinlock_t timers_lock;
static struct my_timer timers[KTIMER_MAX];
static unsigned int max_supported = 1;


// helper functions
static int find_timer_by_msg_locked(const char *msg)
{
    int i;
    for (i = 0; i < KTIMER_MAX; i++) {
        if (timers[i].active && strcmp(timers[i].msg, msg) == 0)
            return i;
    }
    return -1;
}

static int find_free_slot_locked(void)
{
    int i;
    unsigned int active = 0;
    for (i = 0; i < KTIMER_MAX; i++) {
        if (timers[i].active) active++;
    }
    if (active >= max_supported)
        return -ENOSPC;
    for (i = 0; i < KTIMER_MAX; i++) {
        if (!timers[i].active)
            return i;
    }
    return -ENOSPC;
}

static void timer_cb(struct timer_list *tlist)
{
    struct my_timer *mt = from_timer(mt, tlist, t);
    unsigned long flags;

    // print only the message as required by the spec
    printk(KERN_INFO "%s\n", mt->msg);

    spin_lock_irqsave(&timers_lock, flags);
    mt->active = false;
    mt->expires_j = 0;
    spin_unlock_irqrestore(&timers_lock, flags);
}

// file ops

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned long flags;

    if (_IOC_TYPE(cmd) != KTIMER_IOC_MAGIC)
        return -ENOTTY;

    switch (cmd) 
    {
        case KTIMER_IOC_SET: 
        {
            struct ktimer_set ks;
            int idx, ret = 0;
            unsigned long when;

            if (copy_from_user(&ks, (void __user *)arg, sizeof(ks)))
                return -EFAULT;

            if (ks.sec < 1 || ks.sec > 86400)
                return -EINVAL;
            ks.msg[KTIMER_MSG_MAX] = '\0';

            when = jiffies + ks.sec * HZ;

            spin_lock_irqsave(&timers_lock, flags);
            idx = find_timer_by_msg_locked(ks.msg);
            
            if (idx >= 0)
            {
                // update existing
                timers[idx].expires_j = when;
                mod_timer(&timers[idx].t, when);
                ret = 1; // updated?
            }
            else 
            {
                // create new
                idx = find_free_slot_locked();
                if (idx < 0) {
                    spin_unlock_irqrestore(&timers_lock, flags);
                    return -ENOSPC;
                }
                strlcpy(timers[idx].msg, ks.msg, sizeof(timers[idx].msg));
                timers[idx].expires_j = when;
                timers[idx].active = true;
                mod_timer(&timers[idx].t, when);
                ret = 0; // new?
            }

            spin_unlock_irqrestore(&timers_lock, flags);
            return ret;
        }

    case KTIMER_IOC_LIST:
    {
        struct ktimer_list kl = {0};
        int i;

        spin_lock_irqsave(&timers_lock, flags);
        for (i = 0; i < KTIMER_MAX; i++)
        {
            if (!timers[i].active) continue;
            if (kl.count >= KTIMER_MAX) break;

            kl.entries[kl.count].sec_remaining =
                (timers[i].expires_j > jiffies)
                ? (u32)((timers[i].expires_j - jiffies) / HZ)
                : 0;

            strlcpy(kl.entries[kl.count].msg, timers[i].msg,
                    sizeof(kl.entries[kl.count].msg));

            kl.count++;
        }

        spin_unlock_irqrestore(&timers_lock, flags);

        if (copy_to_user((void __user *)arg, &kl, sizeof(kl)))
            return -EFAULT;

        return 0;
    }

    case KTIMER_IOC_SETMAX:
    {
        __u32 newmax;
        unsigned int active = 0;
        int i;

        if (copy_from_user(&newmax, (void __user *)arg, sizeof(newmax)))
            return -EFAULT;
        if (newmax < 1 || newmax > KTIMER_MAX)
            return -EINVAL;

        spin_lock_irqsave(&timers_lock, flags);

        for (i = 0; i < KTIMER_MAX; i++)
            if (timers[i].active) active++;

        if (newmax < active) {
            spin_unlock_irqrestore(&timers_lock, flags);
            return -EBUSY; // cant go below current active
        }

        max_supported = newmax;
        spin_unlock_irqrestore(&timers_lock, flags);

        return 0;
    }

    case KTIMER_IOC_GETMAX:
    {
        __u32 val = max_supported;

        if (copy_to_user((void __user *)arg, &val, sizeof(val)))
            return -EFAULT;

        return 0;
    }

    default:
        return -ENOTTY;
    }
}

static int my_open(struct inode *inode, struct file *filp) { return 0; }
static int my_release(struct inode *inode, struct file *filp) { return 0; }

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = my_ioctl,
    .open           = my_open,
    .release        = my_release,
};

static int __init my_init(void)
{
    int i, rc;

    spin_lock_init(&timers_lock);

    // preinit timers
    for (i = 0; i < KTIMER_MAX; i++) {
        timers[i].active = false;
        timers[i].expires_j = 0;
        timers[i].msg[0] = '\0';
        timer_setup(&timers[i].t, timer_cb, 0);
    }

    rc = register_chrdev_region(devno, 1, DRV_NAME);
    if (rc) return rc;

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    rc = cdev_add(&my_cdev, devno, 1);

    if (rc) {
        unregister_chrdev_region(devno, 1);
        return rc;
    }

    max_supported = 1;
    
    return 0;
}

static void __exit my_exit(void)
{
    int i;

    // remove device then stop timers
    cdev_del(&my_cdev);
    unregister_chrdev_region(devno, 1);

    for (i = 0; i < KTIMER_MAX; i++) {
        del_timer_sync(&timers[i].t);
        timers[i].active = false;
    }
}

module_init(my_init);
module_exit(my_exit);