#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#define DRIVER_AUTHOR "RedBull"
#define DRIVER_DESC   "A simplest driver"

#define BUF_SIZE 64
#define PROC_NAME "kmodule"

static char *msg;
static int msg_len;
static DEFINE_MUTEX(proc_mutex);

static ssize_t read_proc(struct file *filp, char __user *buf, 
                         size_t count, loff_t *offp)
{
    ssize_t ret = 0;
    
    mutex_lock(&proc_mutex);      // захват мьютекса перед чтением
    
    if (*offp >= msg_len) {
        ret = 0;
        goto out;
    }
    
    if (count > msg_len - *offp)
        count = msg_len - *offp;
    
    if (copy_to_user(buf, msg + *offp, count)) {
        ret = -EFAULT;
        goto out;
    }
    
    *offp += count;
    ret = count;
    
out:
    mutex_unlock(&proc_mutex);    // освобождение мьютекса
    return ret;
}

static ssize_t write_proc(struct file *filp, const char __user *buf, 
                          size_t count, loff_t *offp)
{
    ssize_t ret = 0;
    
    mutex_lock(&proc_mutex);      // захват мьютекса перед записью
    
    if (count >= BUF_SIZE)
        count = BUF_SIZE - 1;
    
    if (copy_from_user(msg, buf, count)) {
        ret = -EFAULT;
        goto out;
    }
    
    msg[count] = '\0';
    msg_len = count;
    *offp = 0;  // Сбросить позицию чтения
    ret = count;
    
out:
    mutex_unlock(&proc_mutex);    // освобождение мьютекса
    return ret;
}

// синтаксис для 5+ версии ядра вместо proc_read: read_proc, proc_write: write_proc
static const struct proc_ops proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

static int __init proc_init(void)
{
    msg = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!msg)
        return -ENOMEM;
    
    if (!proc_create(PROC_NAME, 0666, NULL, &proc_fops)) {
        kfree(msg);
        return -ENOMEM;
    }
    
    msg[0] = '\0';
    msg_len = 0;
    
    pr_info("Модуль %s загружен\n", PROC_NAME);
    return 0;
}

static void __exit proc_cleanup(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    kfree(msg);
    pr_info("Модуль %s выгружен\n", PROC_NAME);
}

MODULE_LICENSE("BSD");
MODULE_AUTHOR(DRIVER_AUTHOR);    /* Автор модуля */
MODULE_DESCRIPTION(DRIVER_DESC); /* Назначение модуля */
module_init(proc_init);
module_exit(proc_cleanup);