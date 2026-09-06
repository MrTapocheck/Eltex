#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#define DRIVER_AUTHOR   "RedBull"
#define DRIVER_DESC     "chardev module"
#define DEVICE_NAME     "chardev"
#define BUF_LEN 512

static dev_t major;
static struct cdev my_cdev;
static struct class *dev_class;
static char buffer[BUF_LEN];
static int buffer_len = 0;
static atomic_t already_open = ATOMIC_INIT(0);

static int dev_open(struct inode *inode, struct file *file) {
    if (atomic_cmpxchg(&already_open, 0, 1) != 0)
        return -EBUSY;
    try_module_get(THIS_MODULE);
    pr_info("Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
    atomic_set(&already_open, 0);
    module_put(THIS_MODULE);
    pr_info("Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    if (*off >= buffer_len)
        return 0;
    if (len > buffer_len - *off)
        len = buffer_len - *off;
    if (copy_to_user(buf, buffer + *off, len))
        return -EFAULT;
    *off += len;
    return len;
}

static ssize_t dev_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    if (len >= BUF_LEN)
        len = BUF_LEN - 1;
    if (copy_from_user(buffer, buf, len))
        return -EFAULT;
    buffer[len] = '\0';
    buffer_len = len;
    *off = 0; // после записи сброс позиции чтения
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

static int __init chardev_init(void) {
    if (alloc_chrdev_region(&major, 0, 1, DEVICE_NAME) < 0)
        return -1;

    cdev_init(&my_cdev, &fops);
    if (cdev_add(&my_cdev, major, 1) < 0) {
        unregister_chrdev_region(major, 1);
        return -1;
    }

    // В ядре 6+ class_create принимает только имя
    dev_class = class_create(DEVICE_NAME);
    if (IS_ERR(dev_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(major, 1);
        return PTR_ERR(dev_class);
    }

    if (!device_create(dev_class, NULL, major, NULL, DEVICE_NAME)) {
        class_destroy(dev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(major, 1);
        return -1;
    }

    pr_info("chardev loaded with major %d\n", MAJOR(major));
    return 0;
}

static void __exit chardev_exit(void) {
    device_destroy(dev_class, major);
    class_destroy(dev_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(major, 1);
    pr_info("chardev unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);   
MODULE_DESCRIPTION(DRIVER_DESC);
module_init(chardev_init);
module_exit(chardev_exit);