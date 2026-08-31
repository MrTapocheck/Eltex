#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#define DRIVER_AUTHOR "RedBull"
#define DRIVER_DESC   "A simplest driver"

static int __init init_simplest(void)
{
        printk(KERN_ALERT "This simplest driver was succesfully inited\n");
        return 0;
}

static void __exit cleanup_simplest(void)
{
        printk(KERN_ALERT "This simplest driver was succesfully cleaned up \n");
}

module_init(init_simplest);
module_exit(cleanup_simplest);


MODULE_LICENSE("BSD");
MODULE_AUTHOR(DRIVER_AUTHOR);    /* Автор модуля */
MODULE_DESCRIPTION(DRIVER_DESC); /* Назначение модуля */
