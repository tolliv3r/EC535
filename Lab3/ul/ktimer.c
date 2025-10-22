#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_PATH "/dev/mytimer"

#define KTIMER_MSG_MAX 128

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s -l\n"
        "  %s -s <SEC> <MSG>\n"
        "  %s -r\n"
        "  %s -m <COUNT>\n", prog, prog, prog, prog);
}

static int open_dev(int flags)
{
    int fd = open(DEV_PATH, flags);
    if (fd < 0) {
        perror("open /dev/mytimer");
        exit(1);
    }
    return fd;
}

static void join_msg(int argc, char **argv, int start_idx, char out[KTIMER_MSG_MAX+1])
{
    size_t pos = 0;
    out[0] = '\0';

    for (int i = start_idx; i < argc; i++) {
        const char *part = argv[i];
        size_t part_len = strlen(part);

        if (pos < KTIMER_MSG_MAX) {
            size_t copy_len = part_len;
            if (pos + copy_len > KTIMER_MSG_MAX) copy_len = KTIMER_MSG_MAX - pos;
            strncat(out, part, copy_len);
            pos = strlen(out);
        }

        if (i + 1 < argc && pos < KTIMER_MSG_MAX) {
            strncat(out, " ", KTIMER_MSG_MAX - pos);
            pos = strlen(out);
        }

        if (pos >= KTIMER_MSG_MAX) break;
    }
    out[KTIMER_MSG_MAX] = '\0';
}

int main(int argc, char **argv)
{
    if (argc < 2) { usage(argv[0]); return 1; }

    // -l : list current timer
    if (strcmp(argv[1], "-l") == 0)
    {
        int fd = open_dev(O_RDONLY);
        char buf[512];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            // driver prints "<MSG> <SEC>\n" when active
            fputs(buf, stdout);
        }
        close(fd);
        return 0;
    }

    // -s SEC MSG : set or update the single timer
    if (strcmp(argv[1], "-s") == 0)
    {
        if (argc < 4) { usage(argv[0]); return 1; }

        char *end = NULL;
        long sec = strtol(argv[2], &end, 10);
        if (*end != '\0' || sec < 1 || sec > 86400) {
            fprintf(stderr, "Invalid SEC (1..86400)\n");
            return 1;
        }

        char msg[KTIMER_MSG_MAX+1];
        join_msg(argc, argv, 3, msg);

        int fd = open_dev(O_RDWR);

        // build command: "SET <sec> <msg>\n"
        char cmd[256 + KTIMER_MSG_MAX];
        int m = snprintf(cmd, sizeof(cmd), "SET %ld %s\n", sec, msg);
        if (m < 0 || m >= (int)sizeof(cmd)) {
            fprintf(stderr, "Message too long\n");
            close(fd);
            return 1;
        }

        ssize_t rc = write(fd, cmd, (size_t)m);
        if (rc < 0)
        {
            if (errno == ENOSPC) {
                // only one timer supported
                printf("1 timer already exists!\n");
                close(fd);
                return 1;
            } else if (errno == EALREADY) {
                // kernel signals update via -EALREADY
                printf("The timer %s was updated!\n", msg);
                close(fd);
                return 0;
            } else {
                perror("write SET");
                close(fd);
                return 1;
            }
        }

        close(fd);
        return 0;
    }

    // -r : reset/remove any active timer
    if (strcmp(argv[1], "-r") == 0)
    {
        int fd = open_dev(O_RDWR);
        const char *cmd = "RESET\n";
        if (write(fd, cmd, strlen(cmd)) < 0) {
            perror("write RESET");
            close(fd);
            return 1;
        }
        close(fd);
        return 0;
    }

    // -m COUNT : attempt to set capacity, atm not for this lab
    if (strcmp(argv[1], "-m") == 0)
    {
        if (argc != 3) { usage(argv[0]); return 1; }

        char *end = NULL;
        long count = strtol(argv[2], &end, 10);
        if (*end != '\0' || count < 1 || count > 5) {
            fprintf(stderr, "Invalid COUNT (1..5)\n");
            return 1;
        }

        int fd = open_dev(O_RDWR);
        char cmd[64];
        int m = snprintf(cmd, sizeof(cmd), "SETMAX %ld\n", count);
        ssize_t rc = write(fd, cmd, (size_t)m);
        if (rc < 0) {
            if (errno == EOPNOTSUPP) {
                // silently fail
                close(fd);
                return 1;
            }
            perror("write SETMAX");
            close(fd);
            return 1;
        }
        close(fd);
        return 0;
    }

    usage(argv[0]);
    return 1;
}