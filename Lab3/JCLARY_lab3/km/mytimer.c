// EC535 Lab 3

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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/sched/signal.h>
#include <linux/device.h>

#define DRV_NAME        "mytimer"
#define MYTIMER_MAJOR   61
#define MYTIMER_MINOR   0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackson Clary");
MODULE_DESCRIPTION("Single kernel timer via /dev/mytimer with SIGIO and /proc status");
MODULE_VERSION("1.0");

#define MYTIMER_MSG_MAX 128

// device state
struct mytimer_dev {
    struct fasync_struct *async_queue;   // async SIGIO recipients
    struct timer_list t;                 // kernel timer
    bool active;                         // timer armed
    pid_t pid;                           // creator pid
    char comm[TASK_COMM_LEN];            // creator comm
    char msg[MYTIMER_MSG_MAX+1];         // message to emit/read
    unsigned long expires_j;             // expiration in jiffies
    unsigned long loaded_j;              // module load time also in jiffies
    struct proc_dir_entry *proc;         // proc entry
    struct class *class;                 // device class
    struct device *device;               // device instance
};

static dev_t devno = MKDEV(MYTIMER_MAJOR, MYTIMER_MINOR);
static struct cdev my_cdev;

static spinlock_t dev_lock;              // protects gdev fields in timer path
static struct mytimer_dev gdev;

/* --------------------- timer callback --------------------- */
static void mytimer_cb(struct timer_list *t)
{
    struct mytimer_dev *d = from_timer(d, t, t);
    unsigned long flags;

    // mark consumed under lock
    spin_lock_irqsave(&dev_lock, flags);
    d->active = false;
    d->expires_j = 0;
    spin_unlock_irqrestore(&dev_lock, flags);

    // print timer message to kernel log
    pr_info("TIMER: %s\n", d->msg);

    // async notification (SIGIO)
    if (d->async_queue)
        kill_fasync(&d->async_queue, SIGIO, POLL_IN);
}

/* --------------------- char device --------------------- */
static ssize_t my_read(struct file *filp, char __user *ubuf, size_t len, loff_t *ppos)
{
    char kbuf[MYTIMER_MSG_MAX + 48];
    int n = 0;
    unsigned long now = jiffies;
    long sec_left = 0;

    if (*ppos > 0)
        return 0;

    if (gdev.active) {
        sec_left = (long)max(0L, (long)((long)(gdev.expires_j - now) / HZ));
        n = scnprintf(kbuf, sizeof(kbuf), "%s %ld\n", gdev.msg, sec_left);
    } else {
        return 0; // no active timers
    }

    if (n > len) n = len;
    if (copy_to_user(ubuf, kbuf, n))
        return -EFAULT;
    *ppos += n;
    return n;
}

static int my_fasync(int fd, struct file *filp, int on)
{
    return fasync_helper(fd, filp, on, &gdev.async_queue);
}

static int my_open(struct inode *inode, struct file *file)
{
    nonseekable_open(inode, file);
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    my_fasync(-1, file, 0);
    return 0;
}

// Commands accepted via write():
//   "SET <sec> <msg>\n"  — create/update timer
//   "RESET\n"            — remove any active timer
//   "SETMAX <n>\n"       — return -ENOTSUPP (no multi-timer support)
static ssize_t my_write(struct file *filp, const char __user *ubuf, size_t len, loff_t *ppos)
{
    char *kbuf, *p;
    int ret = 0;
    unsigned long flags;

    if (len == 0 || len > 1024)
        return -EINVAL;

    kbuf = kmalloc(len + 1, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;
    if (copy_from_user(kbuf, ubuf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }
    kbuf[len] = '\0';

    // trim trailing newlines
    while (len && (kbuf[len-1] == '\n' || kbuf[len-1] == '\r'))
        kbuf[--len] = '\0';

    if (!strncmp(kbuf, "SETMAX", 6)) {
        // no multi-timer support
        ret = -EOPNOTSUPP;
        goto out_err;
    }

    if (!strncmp(kbuf, "RESET", 5)) {
        del_timer_sync(&gdev.t);
        spin_lock_irqsave(&dev_lock, flags);
        gdev.active = false;
        gdev.expires_j = 0;
        spin_unlock_irqrestore(&dev_lock, flags);
        ret = (ssize_t)strlen(kbuf);
        goto out;
    }

    if (!strncmp(kbuf, "SET", 3)) {
        unsigned long sec;
        char *sec_s;
        char *msg_s;
        unsigned long when;

        p = kbuf + 3;
        while (*p == ' ' || *p == '\t') p++;
        sec_s = p;
        // find end of number
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
        while (*p == ' ' || *p == '\t') p++;
        msg_s = p; // rest of line is message
        if (!*sec_s || !*msg_s) { ret = -EINVAL; goto out_err; }

        if (kstrtoul(sec_s, 10, &sec) || sec < 1 || sec > 86400) {
            ret = -EINVAL;
            goto out_err;
        }

        when = jiffies + sec * HZ;

        // evaluate add/update rules
        if (gdev.active) {
            if (strncmp(gdev.msg, msg_s, MYTIMER_MSG_MAX) == 0) {
                // update exsting timer with same message
                spin_lock_irqsave(&dev_lock, flags);
                gdev.expires_j = when;
                spin_unlock_irqrestore(&dev_lock, flags);
                mod_timer(&gdev.t, when);
                ret = -EALREADY; // signal to userspace: updated
                goto out_err;
            } else {
                // only one active timer supported
                ret = -ENOSPC;
                goto out_err;
            }
        }

        // create new timer
        strlcpy(gdev.msg, msg_s, sizeof(gdev.msg));
        gdev.pid = current->pid;
        get_task_comm(gdev.comm, current);
        spin_lock_irqsave(&dev_lock, flags);
        gdev.expires_j = when;
        gdev.active = true;
        spin_unlock_irqrestore(&dev_lock, flags);
        mod_timer(&gdev.t, when);
        ret = (ssize_t)strlen(kbuf);
        goto out;
    }

    ret = -EINVAL;

out_err:
    kfree(kbuf);
    return ret;
out:
    kfree(kbuf);
    return ret;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .read    = my_read,
    .write   = my_write,
    .open    = my_open,
    .release = my_release,
    .fasync  = my_fasync,
    .llseek  = no_llseek,
};

/* --------------------- /proc/mytimer --------------------- */
static int mytimer_proc_show(struct seq_file *m, void *v)
{
    unsigned long now = jiffies;
    long sec_left = 0;

    seq_printf(m, "[MODULE_NAME]: %s\n", DRV_NAME);
    seq_printf(m, "[MSEC]: %u\n", jiffies_to_msecs(now - gdev.loaded_j));

    if (gdev.active) {
        sec_left = (long)max(0L, (long)((long)(gdev.expires_j - now) / HZ));
        seq_printf(m, "[PID]: %d\n", gdev.pid);
        seq_printf(m, "[CMD]: %s\n", gdev.comm);
        seq_printf(m, "[SEC]: %ld\n", sec_left);
    }
    return 0;
}

static int mytimer_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, mytimer_proc_show, NULL);
}

static const struct file_operations mytimer_proc_ops = {
    .owner   = THIS_MODULE,
    .open    = mytimer_proc_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

/* --------------------- init/exit --------------------- */
static int __init my_init(void)
{
    int err;

    spin_lock_init(&dev_lock);
    gdev.active = false;
    gdev.loaded_j = jiffies;

    // Register fixed major/minor
    err = register_chrdev_region(devno, 1, DRV_NAME);
    if (err) {
        pr_err("mytimer: register_chrdev_region failed %d\n", err);
        return err;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    err = cdev_add(&my_cdev, devno, 1);
    if (err) {
        pr_err("mytimer: cdev_add failed %d\n", err);
        unregister_chrdev_region(devno, 1);
        return err;
    }

    // setup timer
    timer_setup(&gdev.t, mytimer_cb, 0);

    // /proc entry
    gdev.proc = proc_create(DRV_NAME, 0444, NULL, &mytimer_proc_ops);
    if (!gdev.proc) {
        pr_err("mytimer: proc_create failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(devno, 1);
        return -ENOMEM;
    }

    // Create device class
    gdev.class = class_create(THIS_MODULE, DRV_NAME);
    if (IS_ERR(gdev.class)) {
        pr_err("mytimer: class_create failed\n");
        proc_remove(gdev.proc);
        cdev_del(&my_cdev);
        unregister_chrdev_region(devno, 1);
        return PTR_ERR(gdev.class);
    }

    // Create device file automatically
    gdev.device = device_create(gdev.class, NULL, devno, NULL, DRV_NAME);
    if (IS_ERR(gdev.device)) {
        pr_err("mytimer: device_create failed\n");
        class_destroy(gdev.class);
        proc_remove(gdev.proc);
        cdev_del(&my_cdev);
        unregister_chrdev_region(devno, 1);
        return PTR_ERR(gdev.device);
    }

    pr_info("mytimer: loaded (major 61)\n");
    return 0;
}

static void __exit my_exit(void)
{
    del_timer_sync(&gdev.t);
    
    // Clean up in reverse order of creation
    if (gdev.device)
        device_destroy(gdev.class, devno);
    if (gdev.class)
        class_destroy(gdev.class);
    if (gdev.proc)
        proc_remove(gdev.proc);
    cdev_del(&my_cdev);
    unregister_chrdev_region(devno, 1);
    pr_info("mytimer: unloaded\n");
}

module_init(my_init);
module_exit(my_exit);