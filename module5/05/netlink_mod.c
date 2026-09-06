#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include "constants.h"

#define DRIVER_AUTHOR   "RedBull"
#define DRIVER_DESC     "Netlink kernel module"

static struct sock *nl_sk = NULL;

// обработчик входящих сообщений от пользователя
static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    char *reply_msg = "Hello from kernel!";
    int reply_size = strlen(reply_msg) + 1;
    int res;

    // проверка длины skb, чтобы повреждённый пакет всё не сломал
    if (skb->len < sizeof(struct nlmsghdr)) {
        pr_err("netlink: skb too short\n");
        return;
    }

    nlh = nlmsg_hdr(skb);
    pid = nlh->nlmsg_pid;

    pr_info("netlink: received payload: %s from pid %d\n", (char *)nlmsg_data(nlh), pid);   

    // подготовка ответного сообщения
    skb_out = nlmsg_new(reply_size, GFP_KERNEL);
    if (!skb_out) {
        pr_err("netlink: failed to allocate skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, reply_size, 0);
    strncpy(nlmsg_data(nlh), reply_msg, reply_size);

    // птправка ответа
    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0)
        pr_err("netlink: failed to send reply (err=%d)\n", res);
}

static int __init netlink_init(void)
{
    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
        .flags = NL_CFG_F_NONROOT_RECV | NL_CFG_F_NONROOT_SEND, // чтобы читать/писать без рута
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        pr_err("netlink: failed to create socket\n");
        return -ENOMEM;
    }

    pr_info("netlink: module loaded\n");
    return 0;
}

static void __exit netlink_exit(void)
{
    if (nl_sk)
        netlink_kernel_release(nl_sk);
    pr_info("netlink: module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);   
MODULE_DESCRIPTION(DRIVER_DESC);
module_init(netlink_init);
module_exit(netlink_exit);