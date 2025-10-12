// EC535 Lab 2

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV_PATH "/dev/mytimer"

#define KTIMER_IOC_MAGIC 'k'
#define KTIMER_MAX 5
#define KTIMER_MSG_MAX 128

struct ktimer_set {
    uint32_t sec;
    char msg[KTIMER_MSG_MAX+1];
};
struct ktimer_entry {
    uint32_t sec_remaining;
    char msg[KTIMER_MSG_MAX+1];
};
struct ktimer_list {
    uint32_t count;
    struct ktimer_entry entries[KTIMER_MAX];
};

#define KTIMER_IOC_SET    _IOW(KTIMER_IOC_MAGIC, 1, struct ktimer_set)
#define KTIMER_IOC_LIST   _IOR(KTIMER_IOC_MAGIC, 2, struct ktimer_list)
#define KTIMER_IOC_SETMAX _IOW(KTIMER_IOC_MAGIC, 3, uint32_t)
#define KTIMER_IOC_GETMAX _IOR(KTIMER_IOC_MAGIC, 4, uint32_t)

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s -l\n"
        "  %s -s <SEC> <MSG>\n"
        "  %s -m <COUNT>\n", prog, prog, prog);
}

static int open_dev(void)
{
    int fd = open(DEV_PATH, O_RDWR);
    if (fd < 0) {
        perror("open /dev/mytimer");
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv)
{
    if (argc < 2) { usage(argv[0]); return 1; }

    // -l
    if (strcmp(argv[1], "-l") == 0)
    {
        int fd = open_dev();
        struct ktimer_list kl;
        memset(&kl, 0, sizeof(kl));

        if (ioctl(fd, KTIMER_IOC_LIST, &kl) != 0) {
            perror("ioctl LIST");
            close(fd);
            return 1;
        }

        // if no timers pending, exit w/o printing
        for (uint32_t i = 0; i < kl.count; i++) {
            // each line: <MSG> <SEC>
            printf("%s %u\n", kl.entries[i].msg, kl.entries[i].sec_remaining);
        }
        close(fd);
        return 0;
    }

    // -s SEC MSG
    if (strcmp(argv[1], "-s") == 0)
    {
        if (argc < 4) { usage(argv[0]); return 1; }

        char *end = NULL;
        long sec = strtol(argv[2], &end, 10);

        if (*end != '\0' || sec < 1 || sec > 86400) {
            fprintf(stderr, "Invalid SEC (1..86400)\n");
            return 1;
        }

        // join MSG (argv[3..])
        char msg[KTIMER_MSG_MAX+1];
        msg[0] = '\0';
        {
            size_t pos = 0;

            for (int i = 3; i < argc; i++) 
            {
                const char *part = argv[i];
                size_t need = strlen(part) + (i+1<argc ? 1 : 0);
                
                if (pos + need > KTIMER_MSG_MAX) {
                    need = (pos + need > KTIMER_MSG_MAX) ? (KTIMER_MSG_MAX - pos) : need;
                }

                strncat(msg, part, KTIMER_MSG_MAX - strlen(msg));
                pos = strlen(msg);

                if (i + 1 < argc && pos < KTIMER_MSG_MAX) {
                    strncat(msg, " ", KTIMER_MSG_MAX - strlen(msg));
                }

                if (strlen(msg) >= KTIMER_MSG_MAX) break;
            }
            msg[KTIMER_MSG_MAX] = '\0';
        }

        int fd = open_dev();
        struct ktimer_set ks;
        ks.sec = (uint32_t)sec;
        memset(ks.msg, 0, sizeof(ks.msg));
        
        // strncpy(ks.msg, msg, KTIMER_MSG_MAX);

        // snprintf(ks.msg, KTIMER_MSG_MAX, "%s", msg);
        
        strncpy(ks.msg, msg, KTIMER_MSG_MAX - 1);
        ks.msg[KTIMER_MSG_MAX - 1] = '\0';

        int rc = ioctl(fd, KTIMER_IOC_SET, &ks);
        
        if (rc < 0)
        {
            if (errno == ENOSPC)
            {
                // print: "[COUNT] timer(s) already exist(s)!"
                uint32_t maxc = 0;
                if (ioctl(fd, KTIMER_IOC_GETMAX, &maxc) != 0) {
                    maxc = 1;
                }

                printf("%u timer(s) already exist(s)!\n", maxc);
                close(fd);
                return 1;
            }
            else
            {
                perror("ioctl SET");
                close(fd);
                return 1;
            }
        } else if (rc == 1) {
            printf("The timer %s was updated!\n", msg);
        }

        close(fd);

        return 0;
    }

    // -m COUNT
    if (strcmp(argv[1], "-m") == 0)
    {
        if (argc != 3) { usage(argv[0]); return 1; }

        char *end = NULL;
        long count = strtol(argv[2], &end, 10);

        if (*end != '\0' || count < 1 || count > 5) {
            fprintf(stderr, "Invalid COUNT (1..5)\n");
            return 1;
        }

        // set capacity for multitimers 
        int fd = open_dev();
        uint32_t c = (uint32_t)count;
        if (ioctl(fd, KTIMER_IOC_SETMAX, &c) != 0) {
            close(fd);
            return 1;
        }
        close(fd);
        return 0;
    }

    // unknown option
    usage(argv[0]);
    return 1;
}