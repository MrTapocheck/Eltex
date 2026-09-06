#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/kd.h>
#include <linux/file.h>

#define DRIVER_AUTHOR "RedBull"
#define DRIVER_DESC   "Keyboard LED control"
#define DEFAULT_DELAY HZ  // переменная = 300 в ядре (зависит от конфигурации), HZ - количество тиков в секунду

static struct kobject *kbleds_kobj;
static struct file *console_file;
static struct timer_list blink_timer;

static int led_mask = 0;
static int blink_state = 0;
static int blink_enabled = 0;
static int blink_delay = DEFAULT_DELAY;

// Функция для установки состояния светодиодов
static void set_leds(int mask)
{
    if (console_file && !IS_ERR(console_file))
        vfs_ioctl(console_file, KDSETLED, mask & 0x07);
}

// /dev/console потому что vc_cons[fg_console].d->port.tty->driver очень не хочет работать
static int open_console(void)
{
    console_file = filp_open("/dev/console", O_RDWR, 0);
    return IS_ERR(console_file) ? -ENODEV : 0;
}

static void timer_func(struct timer_list *ptr)
{
    if (!blink_enabled) {
        set_leds(0);
        return;
    }
    
    blink_state = !blink_state;
    set_leds(blink_state ? led_mask : 0);
    
    blink_timer.expires = jiffies + blink_delay;
    add_timer(&blink_timer);
}

//маска
static ssize_t mask_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", led_mask);
}

static ssize_t mask_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    int val;
    if (sscanf(buf, "%d", &val) != 1 || val < 0 || val > 7)
        return -EINVAL;
    
    led_mask = val;
    if (!blink_enabled)
        set_leds(led_mask);
    
    return count;
}

static struct kobj_attribute mask_attr = __ATTR(mask, 0644, mask_show, mask_store);

//мигание
static ssize_t blink_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", blink_enabled);
}

static ssize_t blink_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    int val;
    if (sscanf(buf, "%d", &val) != 1 || val < 0 || val > 1)
        return -EINVAL;
    
    if (val == 1 && !blink_enabled) {
        blink_enabled = 1;
        blink_state = 0;
        set_leds(0);
        timer_setup(&blink_timer, timer_func, 0);
        blink_timer.expires = jiffies + blink_delay;
        add_timer(&blink_timer);
    } else if (val == 0 && blink_enabled) {
        blink_enabled = 0;
        del_timer_sync(&blink_timer);
        set_leds(0);
    }
    
    return count;
}

static struct kobj_attribute blink_attr = __ATTR(blink, 0644, blink_show, blink_store);

//частота мигания
static ssize_t delay_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", blink_delay);  // В тиках
}

static ssize_t delay_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    int val;
    if (sscanf(buf, "%d", &val) != 1 || val <= 0)
        return -EINVAL;
    
    blink_delay = val;
    pr_info("kbleds: delay changed to %d ticks\n", blink_delay);
    
    return count;
}

static struct kobj_attribute delay_attr = __ATTR(delay, 0644, delay_show, delay_store);

static int __init kbleds_init(void)
{
    int err;
    
    if (open_console() < 0)
        return -ENODEV;
    
    kbleds_kobj = kobject_create_and_add("kbleds", kernel_kobj);
    if (!kbleds_kobj)
        return -ENOMEM;
    
    err = sysfs_create_file(kbleds_kobj, &mask_attr.attr);
    if (err) goto fail_mask;
    
    err = sysfs_create_file(kbleds_kobj, &blink_attr.attr);
    if (err) goto fail_blink;
    
    err = sysfs_create_file(kbleds_kobj, &delay_attr.attr);
    if (err) goto fail_delay;
    
    blink_delay = DEFAULT_DELAY;
    set_leds(0);
    return 0;

fail_delay:
    sysfs_remove_file(kbleds_kobj, &blink_attr.attr);
fail_blink:
    sysfs_remove_file(kbleds_kobj, &mask_attr.attr);
fail_mask:
    kobject_put(kbleds_kobj);
    return err;
}

// очистка
static void __exit kbleds_cleanup(void)
{
    if (blink_enabled) {
        blink_enabled = 0;
        del_timer_sync(&blink_timer);
    }
    
    set_leds(0);
    
    if (console_file && !IS_ERR(console_file))
        filp_close(console_file, NULL);
    
    sysfs_remove_file(kbleds_kobj, &delay_attr.attr);
    sysfs_remove_file(kbleds_kobj, &blink_attr.attr);
    sysfs_remove_file(kbleds_kobj, &mask_attr.attr);
    kobject_put(kbleds_kobj);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
module_init(kbleds_init);
module_exit(kbleds_cleanup);