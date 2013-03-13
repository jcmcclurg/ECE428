#include "mp1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>               
#include <unistd.h>

static pthread_mutex_t stderr_lock = PTHREAD_MUTEX_INITIALIZER;

int debugprintf(const char *format, ...)
{
    int retval = 0;
#ifndef DISABLE_DEBUGPRINTF
    pthread_mutex_lock(&stderr_lock);
    va_list argptr;
    va_start(argptr, format);
    retval = vfprintf(stderr, format, argptr);
    va_end(argptr);
    pthread_mutex_unlock(&stderr_lock);
#endif
    return retval;
}

int g_crashAfterSecs = -1;

char *g_delaydropspec_filepath = NULL;

void *crash_after(void *discard) {
    assert(g_crashAfterSecs > 0);
    sleep(g_crashAfterSecs);
    pid_t mypid = getpid();
    debugprintf("*** Physical time (seconds since epoch): %ld\n", time(NULL));
    kill(mypid, 9);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    int opt;
    int long_index;

    struct option long_options[] = {
        {"crashAfterSecs", required_argument, 0, 1000},
        {"delaydropspec", required_argument, 0, 1001},
        {0, 0, 0, 0},
    };
    while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1)
    {
        switch (opt) {
        case 0:
            if (long_options[long_index].flag != 0) {
                break;
            }
            printf("option %s ", long_options[long_index].name);
            if (optarg) {
              printf("with arg %s\n", optarg);
            }
            printf("\n");
            break;

        case 1000:
            g_crashAfterSecs = strtol(optarg, NULL, 10);
            assert(g_crashAfterSecs > 0);
            break;

        case 1001:
            g_delaydropspec_filepath = optarg;
            break;

        default:
            exit(1);
        }
    }

    assert(g_delaydropspec_filepath);

    multicast_init();

    if (g_crashAfterSecs > 0) {
        pthread_t t;
        if (pthread_create(&t, NULL, &crash_after, NULL)) {
            printf("Error in pthread_create\n");
            exit(1);
        }
        else {
            printf("Will crash after %d seconds\n", g_crashAfterSecs);
        }
    }

    /* printf("*** Physical time (seconds since epoch): %ld\n", time(NULL)); */
    /* printf("*** Chat started, my ID is %d\n", my_id); */

    while (1) {
        char str[256] = {0};
        int len;

        if (fgets(str, sizeof(str), stdin) == NULL) {
            if (feof(stdin)) {
              break;
            } else {
                if (errno != EINTR) {
                    perror("fgets");
                    break;
                } else {
                    continue;
                }
            }
        }
        len = strlen(str);
        /* trim newline */
        if (str[len-1] == '\n') {
            str[len-1] = 0;
        }
        if (strcmp(str, "/quit") == 0) {
            printstats();
            break;
        }
        if (strstr(str, "/quitafter") == str) {
            g_crashAfterSecs = strtol(str + 11, NULL, 10);
            assert(g_crashAfterSecs > 0);
            pthread_t t;
            if (pthread_create(&t, NULL, &crash_after, NULL)) {
                printf("Error in pthread_create\n");
                exit(1);
            }
            else {
                debugprintf("Will crash after %d seconds\n", g_crashAfterSecs);
            }
            continue;
        }

        multicast(str);
    }

    return 0;
}

static pthread_mutex_t stdout_lock = PTHREAD_MUTEX_INITIALIZER;
void deliver(int source, const char *message) {
    pthread_mutex_lock(&stdout_lock);
    printf("<%d> %s\n", source, message);
    pthread_mutex_unlock(&stdout_lock);
}
