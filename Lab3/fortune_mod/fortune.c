#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Fortune Cookie Kernel Module");
MODULE_AUTHOR("M. Tim Jones");

#define MAX_COOKIE_LENGTH       PAGE_SIZE

static struct proc_dir_entry *proc_entry;

static char *cookie_pot;  // Space for fortune strings
static int cookie_index;  // Index to write next fortune
static int next_fortune;  // Index to read next fortune

static ssize_t fortune_write(struct file*, const char __user*, size_t, loff_t*);
static int fortune_proc_open(struct inode*, struct file*);
static int fortune_proc_show(struct seq_file*, void*);
static int fortune_init(void);
static void fortune_exit(void);

static const struct file_operations fortune_proc_fops = {
    .owner = THIS_MODULE,
    .open = fortune_proc_open,
    .read = seq_read,
    .write = fortune_write,
    .llseek = seq_lseek,
    .release = single_release,
};

module_init(fortune_init);
module_exit(fortune_exit);

static int fortune_init(void)
{
    int ret = 0;
    cookie_pot = (char *)vmalloc(MAX_COOKIE_LENGTH);

    if (!cookie_pot) {
        ret = -ENOMEM;
    } else {
        memset(cookie_pot, 0, MAX_COOKIE_LENGTH);
        proc_entry = proc_create("fortune", 0644, NULL, &fortune_proc_fops);
        if (proc_entry == NULL) {
            ret = -ENOMEM;
            vfree(cookie_pot);
            printk(KERN_INFO "fortune: Couldn't create proc entry\n");
        } else {
            cookie_index = 0;
            next_fortune = 0;
            printk(KERN_INFO "fortune: Module loaded.\n");
        }
    }
    return ret;
}

static int fortune_proc_show(struct seq_file *m, void *v) {
    /* Wrap-around */
    if (next_fortune >= cookie_index) {
        next_fortune = 0;
    }

    /* Use seq_files to paginate the output (just in case it's very large) */
    seq_printf(m, "%s\n", &(cookie_pot[next_fortune]));

    next_fortune += strlen(&(cookie_pot[next_fortune])) + 1;
    return 0;
}


static int fortune_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, fortune_proc_show, NULL);
}


static ssize_t fortune_write(struct file *filp, const char __user *buff, size_t len, loff_t *fpos)
{
    int space_available = (MAX_COOKIE_LENGTH - cookie_index) + 1;
    if (len > space_available) {
        printk(KERN_INFO "fortune: cookie pot is full!\n");
        return -ENOSPC;
    }

    if (copy_from_user(&(cookie_pot[cookie_index]), buff, len)) {
        return -EFAULT;
    }

    cookie_index += len;
    cookie_pot[cookie_index-1] = 0;
    *fpos = len;
    return len;
}


static void fortune_exit(void)
{
    remove_proc_entry("fortune", NULL);
    vfree(cookie_pot);
    printk(KERN_INFO "fortune: Module unloaded.\n");
}
