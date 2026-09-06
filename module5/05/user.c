#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "constants.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;
    int sock_fd;
    int ret;

    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    // привязка к своему PID
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    // адрес ядра
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;      // ядро
    dest_addr.nl_groups = 0;   // unicast

    // подготовка сообщения
    size_t payload_len = strlen(argv[1]) + 1;
    size_t msg_len = NLMSG_SPACE(payload_len);
    nlh = (struct nlmsghdr *)malloc(msg_len);
    if (!nlh) {
        perror("malloc");
        close(sock_fd);
        return EXIT_FAILURE;
    }
    memset(nlh, 0, msg_len);
    nlh->nlmsg_len = msg_len;
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    strncpy(NLMSG_DATA(nlh), argv[1], payload_len);

    //отправка
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Sending to kernel: %s\n", argv[1]);
    ret = sendmsg(sock_fd, &msg, 0);
    if (ret < 0) {
        perror("sendmsg");
        free(nlh);
        close(sock_fd);
        return EXIT_FAILURE;
    }

    // ожидание ответа 
    char recv_buf[8192];
    struct iovec recv_iov = {
        .iov_base = recv_buf,
        .iov_len = sizeof(recv_buf),
    };
    struct msghdr recv_msg = {
        .msg_name = &dest_addr,
        .msg_namelen = sizeof(dest_addr),
        .msg_iov = &recv_iov,
        .msg_iovlen = 1,
    };

    ret = recvmsg(sock_fd, &recv_msg, 0);
    if (ret < 0) {
        perror("recvmsg");
        free(nlh);
        close(sock_fd);
        return EXIT_FAILURE;
    }
}