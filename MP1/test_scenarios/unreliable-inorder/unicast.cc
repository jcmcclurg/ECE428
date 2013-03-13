#include "mp1.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

int *mcast_members;
int mcast_num_members;
int my_id;
static int mcast_mem_alloc;
pthread_mutex_t member_lock = PTHREAD_MUTEX_INITIALIZER;

static int sock;

pthread_t receive_thread;

/* will contain member ids in the added by new_member(), including
 * myself. can't use mcast_members because it might be messed with by
 * students code.
 */
pthread_mutex_t _tester__lock__ = PTHREAD_MUTEX_INITIALIZER;

#define MAXNUMMEMBERS (32)
static int mymembers[MAXNUMMEMBERS] = {0};
static int mynummembers = 0;


/* first dim: multicast number. 2nd dim: peer number/idx (to look up
 * in mymembers array)
 */
#define MAXMSGLEN (4096)
#define MAXNUMMSG (128)

static const char TESTMSG_BEGIN_MARKER[] = "T3xM3u6w_b:   ";
static const char TESTMSG_END_MARKER[] = "T3xM3u6w_e";

typedef struct msgdelay {
    char msg[MAXMSGLEN]; /* only the "app payload" part */
    int numdests; /* number of dests this msg will be sent to */
    int sentcounts[MAXNUMMEMBERS]; /* count the attempts to send this
                                    * same msg, to a particular member
                                    * peer.
                                    */
    double delays[MAXNUMMEMBERS]; /* -1 -> drop, 0 -> MINDELAY, or a
                                   * value in range [MINDELAY,
                                   * MAXDELAY].
                                   *
                                   * only used for the first
                                   * send. resends will be dropped,
                                   * since we are testing
                                   * reliable-outoforder.
                                   */
} msgdelay_t;

static msgdelay_t mydelays[MAXNUMMSG];
static int g_nummsgs = 0;

/* (test/multicast) msgs not originated by me, but sent after perhaps
 * being requested.
 */
static msgdelay_t others_msgs[MAXNUMMSG] = {0};
static int g_numothersmsgs = 0;

/* assume marker is nul-terminated, this is like strstr() but supports
 * nul chars in string (also thus requires len)
 */
const char *
mystrstr(const char *haystack, const int haystacklen,
         const char *needle)
{
    const int needlelen = strlen(needle);
    if (haystacklen < needlelen) {
        return NULL;
    }
    int i = 0;
    for (; i <= (haystacklen - needlelen); ++i) {
        if (!strncmp(haystack+i, needle, (needlelen))) {
            return haystack + i;
        }
    }
    return NULL;
}

/* Add a potential new member to the list */
void new_member(int member) {
    int i;
    pthread_mutex_lock(&member_lock);
    for (i = 0; i < mcast_num_members; i++) {
        if (member == mcast_members[i])
            break;
    }
    if (i == mcast_num_members) { /* really is a new member */
      mymembers[mynummembers++] = member;

        if (mcast_num_members == mcast_mem_alloc) { /* make sure there's enough space */
            mcast_mem_alloc *= 2;
            mcast_members = (int *) realloc(mcast_members, mcast_mem_alloc * sizeof(mcast_members[0]));
            if (mcast_members == NULL) {
                perror("realloc");
                exit(1);
            }
        }
        mcast_members[mcast_num_members++] = member;
        pthread_mutex_unlock(&member_lock);
        mcast_join(member);
    } else {
        pthread_mutex_unlock(&member_lock);
    }
}

int extractMsgText(const char *packet,
                   char *msgtext,
                   const int pktlen)
{
    const char *begin = mystrstr(packet, pktlen, TESTMSG_BEGIN_MARKER);
    if (!begin) {
        return 0;
    }

    const char *end = mystrstr(packet, pktlen, TESTMSG_END_MARKER);
    assert(end != NULL && end > begin);

    end = end + strlen(TESTMSG_END_MARKER);

    strncpy(msgtext, begin, end - begin);
    return 1;
}

void *receive_thread_main(void *discard) {
    struct sockaddr_in fromaddr;
    socklen_t len;
    int nbytes;
    char buf[1000];

    for (;;) {
        int source;
        len = sizeof(fromaddr);
        nbytes = recvfrom(sock,buf,1000,0,(struct sockaddr *)&fromaddr,&len);
        if (nbytes < 0) {
            if (errno == EINTR)
                continue;
            perror("recvfrom");
            exit(1);
        }

        source = ntohs(fromaddr.sin_port);

        char msgtext[256] = {0};
        if (extractMsgText(buf, msgtext, nbytes)) {
            debugprintf("debug: <%d> time= %ld,   rx from <%d>, msg     [%s]\n",
                        my_id,time(NULL), source, msgtext);
        }

        if (nbytes == 0) {
            /* A first message from someone */
            new_member(source);
        } else {
            receive(source, buf, nbytes);
        }
    }
}


void unicast_init(void) {
    static int called = 0;
    assert(called == 0);
    called = 1;

    struct sockaddr_in servaddr;
    socklen_t len;
    int fd;
    char buf[128];
    int i,n;

    /* set up UDP listening socket */
    sock = socket(AF_INET,SOCK_DGRAM,0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servaddr.sin_port=htons(0);     /* let the operating system choose a port for us */

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* obtain a port number */
    len = sizeof(servaddr);
    if (getsockname(sock, (struct sockaddr *) &servaddr, &len) < 0) {
        perror("getsockname");
        exit(1);
    }

    my_id = ntohs(servaddr.sin_port);
    fprintf(stdout, "Our port number: %d\n", my_id);

    /* allocate initial member arrary */
    mcast_mem_alloc = 16;
    mcast_members = (int *) malloc(sizeof(mcast_members[0]) * mcast_mem_alloc);
    if (mcast_members == NULL) {
        perror("malloc");
        exit(1);
    }
    mcast_num_members = 0;

    /* start receiver thread to make sure we obtain announcements from anyone who tries to contact us */
    if (pthread_create(&receive_thread, NULL, &receive_thread_main, NULL) != 0) {
        fprintf(stderr, "Error in pthread_create\n");
        exit(1);
    }

    /* now read in the rand file */
    FILE *delaydropspec_file = fopen(g_delaydropspec_filepath, "r");
    assert(delaydropspec_file);

    bzero(mydelays, sizeof mydelays);
    int numdsts = 0;
    int msgidx = 0;
    while (!feof(delaydropspec_file)) {
        msgdelay_t* msgdelay = &(mydelays[msgidx]);
        char line[256] = {0};

        if (NULL == fgets(line, sizeof line, delaydropspec_file)) {
            continue;
        }

        size_t linelen = strlen(line);
        assert (line[linelen] == 0);
        assert (line[linelen - 1] == '\n');
        line[linelen - 1] = 0;
        debugprintf("--------------------\nline: [%s]\n", line);
        static const char delims[] = "|";

        char *result = strtok(line, delims);
        strcpy(msgdelay->msg, result);
        debugprintf("msg [idx=%d]: [%s]\n", msgidx, msgdelay->msg);

        result = strtok(NULL, delims);
        int dstidx = 0;
        while (result) {
            assert (dstidx < MAXNUMMEMBERS);
            msgdelay->delays[dstidx] = strtod(result, NULL);
            if (msgdelay->delays[dstidx] == 0) {
                msgdelay->delays[dstidx] = MINDELAY;
            }
            assert ((msgdelay->delays[dstidx] == -1)
                    || (msgdelay->delays[dstidx] >= MINDELAY
                        && msgdelay->delays[dstidx] <= MAXDELAY));
            msgdelay->sentcounts[dstidx] = 0;
            debugprintf("    delay [dstidx=%d]: [%f]\n",
                        dstidx, msgdelay->delays[dstidx]);
            ++dstidx;
            result = strtok(NULL, delims);
        }
        if (msgidx == 0) {
            numdsts = dstidx;
        }
        else {
            /* different number of dsts across messages */
            assert (dstidx == numdsts);
        }
        msgdelay->numdests = numdsts;
        ++msgidx;
    }
    g_nummsgs = msgidx;
    fclose(delaydropspec_file);
    fprintf(stdout, "done with delaydropspec\n");

    bzero(others_msgs, sizeof others_msgs);
    g_numothersmsgs = 0;
    /**/



    /* add self to group file */

    fd = open(GROUP_FILE, O_WRONLY | O_APPEND | O_CREAT, 0600); /* read-write by the user */
    if (fd < 0) {
        perror("open(group file, 'a')");
        exit(1);
    }

    if (write(fd, &my_id, sizeof(my_id)) != sizeof(my_id)) {
        perror("write");
        exit(1);
    }
    close(fd);


    /* now read in the group file */
    fd = open(GROUP_FILE, O_RDONLY);
    if (fd < 0) {
        perror("open(group file, 'r')");
        exit(1);
    }

     do {
         int *member;
         n = read(fd, buf, sizeof(buf));
         if (n < 0) {
             break;
         }

         for (member = (int *) buf; ((char *) member) - buf < n; member++) {
             new_member(*member);
         }
     } while (n == sizeof(buf));

     close(fd);

     /* announce ourselves to everyone */

     pthread_mutex_lock(&member_lock);
     for (i = 0; i < mcast_num_members; i++) {
         struct sockaddr_in destaddr;

         if (mcast_members[i] == my_id)
             continue;  /* don't announce to myself */

         memset(&destaddr, 0, sizeof(destaddr));
         destaddr.sin_family = AF_INET;
         destaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
         destaddr.sin_port = htons(mcast_members[i]);

         sendto(sock, buf, 0, 0, (struct sockaddr *) &destaddr, sizeof(destaddr));
     }
     pthread_mutex_unlock(&member_lock);
}

struct message {
    struct message *next;
    double delivery_time;
    char *message;
    int len;
    int dest;
};

struct message *head = NULL;

void insert_message(struct message *newmsg) {
    struct message **p = &head;

    while (*p && (*p)->delivery_time < newmsg->delivery_time)
        p = &(*p)->next;

    newmsg->next = *p;
    *p = newmsg;
}

void catch_alarm(int sig);

void setsignal(void) {
    struct sigaction action;

    action.sa_handler = catch_alarm;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGALRM, &action, NULL);
}

void setalarm(useconds_t seconds) {
    struct itimerval timer;
    timer.it_value.tv_usec = seconds % 1000000;
    timer.it_value.tv_sec = seconds / 1000000;
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec = 0;

    assert(0 == setitimer(ITIMER_REAL, &timer, NULL));
}


void catch_alarm(int sig) {
    struct timeval tim;
    double now;

    gettimeofday(&tim, NULL);
    now = tim.tv_sec * 1000000.0 + tim.tv_usec;

    while (head && head->delivery_time <= now) {
        struct message *temp;
        struct sockaddr_in destaddr;

        memset(&destaddr, 0, sizeof(destaddr));
        destaddr.sin_family = AF_INET;
        destaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        destaddr.sin_port = htons(head->dest);

        sendto(sock, head->message, head->len, 0, (struct sockaddr *) &destaddr, sizeof(destaddr));

        temp = head;
        free(head->message);
        head = head->next;
        free(temp);
    }

    if (head) {
	setsignal();
        setalarm((useconds_t) head->delivery_time - now);
    }
}



void usend(int dest, const char *message, int len) {
    struct timeval tim;
    struct message *newmsg;
    double delay = -1;
    int memberidx = 0;
    int msgidx = 0;
    
    if (dest == my_id) {
        delay = 1;
        goto schedule;
    }

    pthread_mutex_lock(&_tester__lock__);

    /* is this a msg that we should control */
    for (; msgidx < g_nummsgs; ++msgidx) {
        if (mystrstr(message, len, mydelays[msgidx].msg)) {
            /* yes, it is such a msg -> now find the right delay for this
             * particular dst
             */
            msgdelay_t* msgdelay = &(mydelays[msgidx]);
            /* the number of actual members must equal those specified in
             * the spec file for this msg.
             */
            assert (mynummembers == msgdelay->numdests);
            int memberidx = 0;
            for (; memberidx < mynummembers; ++memberidx) {
                if (dest == mymembers[memberidx]) {
                    break;
                }
            }
            assert (memberidx < mynummembers);
            // now we have the index of the dest
//            debugprintf("dest idx: %d, delay: %f\n", memberidx, msgdelay->delays[memberidx]);
            if (msgdelay->sentcounts[memberidx] == 0) {
                msgdelay->sentcounts[memberidx] = msgdelay->sentcounts[memberidx] + 1;
                delay = msgdelay->delays[memberidx];
                if (delay == -1) {
                    debugprintf("debug: <%d> time= %ld, tx to <%d> DROPPED, attcnt %d, msg     [%s]\n",
                                my_id,time(NULL),dest,
                           msgdelay->sentcounts[memberidx], msgdelay->msg);
                    goto ret;
                }
                else {
                    // assume the value already passed sanity checks
                }
            }
            else {
                /* this is a resend */
                msgdelay->sentcounts[memberidx] = msgdelay->sentcounts[memberidx] + 1;
                delay = MAXDELAY;
            }

            debugprintf("debug: <%d> time= %ld, tx to <%d> del= %2.f, attcnt %d, msg     [%s]\n",
                        my_id,time(NULL),dest,  delay,
                        msgdelay->sentcounts[memberidx], msgdelay->msg);
            break;
        }
    }

    if (msgidx == g_nummsgs) {
        /* not our own multicast msg */
        delay = MINDELAY;

        /* is it a test multicast msg? */
        if (mystrstr(message, len, TESTMSG_BEGIN_MARKER) != NULL) {

            /* this is trying to resend someone else's message --> we
             * just drop to prevent affecting our expected transcripts
             * (i.e., it might arrive before the original message
             */
            goto ret;
            // figure out the memberidx
            int memberidx = 0;
            for (; memberidx < mynummembers; ++memberidx) {
                if (dest == mymembers[memberidx]) {
                    break;
                }
            }
            assert (memberidx < mynummembers);
            // figure out the msg idx
            msgdelay_t* msgdelay = NULL;
            int msgidx = 0;
            for (; msgidx < g_numothersmsgs; ++msgidx) {
                if (mystrstr(message, len, others_msgs[msgidx].msg)) {
                    msgdelay = &(others_msgs[msgidx]);
                    break;
                }
            }
            if (msgdelay == NULL) {
                /* newly seen -> add to array */
                ++g_numothersmsgs;
                assert (g_numothersmsgs <= MAXNUMMSG);
                msgdelay = &(others_msgs[g_numothersmsgs - 1]);
                strcpy(msgdelay->msg, message);
            }
            assert (msgdelay != NULL);
            char msgtext[256] = {0};
            extractMsgText(message, msgtext, len);
            msgdelay->sentcounts[memberidx] =
                msgdelay->sentcounts[memberidx] + 1;
            debugprintf("debug: <%d> time= %ld, re-tx to <%d> someone else's msg, del= %2.f, attcnt %d, msg     [%s]\n",
                        my_id, time(NULL),dest,  delay,
                        msgdelay->sentcounts[memberidx], msgtext);
        }
    }

schedule:
    newmsg = (struct message *) malloc(sizeof(struct message));
    newmsg->message = (char *) malloc(len);
    memcpy(newmsg->message, message, len);
    newmsg->len = len;
    newmsg->dest = dest;

    gettimeofday(&tim, NULL);
    newmsg->delivery_time = tim.tv_sec * 1000000.0  + tim.tv_usec + delay;

    insert_message(newmsg);

    if (newmsg == head) {
        setsignal();
        setalarm((useconds_t) delay);
    }
ret:
    pthread_mutex_unlock(&_tester__lock__);
}

void printstats()
{
    pthread_mutex_lock(&_tester__lock__);
    printf("\n--------------------- stats ---------------\n");

    printf("my own messages:\n");
    int msgidx = 0;
    for (; msgidx < g_nummsgs; ++msgidx) {
        const msgdelay_t* msgdelay = &(mydelays[msgidx]);
        printf("msg [%s]\n", msgdelay->msg);
        int dstidx = 0;
        assert(msgdelay->numdests == mynummembers);
        for (; dstidx < mynummembers; ++dstidx) {
            printf("  dst <%d>: %d,", mymembers[dstidx],
                   msgdelay->sentcounts[dstidx]);
        }
        printf("\n");
    }

    printf("\n");

    printf("others' messages:\n");
    msgidx = 0;
    for (; msgidx < g_numothersmsgs; ++msgidx) {
        const msgdelay_t* msgdelay = &(others_msgs[msgidx]);
        char msgtext[256] = {0};
        extractMsgText(msgdelay->msg, msgtext, strlen(msgdelay->msg));
        printf("msg [%s]\n", msgtext);
        int dstidx = 0;
        for (; dstidx < mynummembers; ++dstidx) {
            printf("  dst <%d>: %d,", mymembers[dstidx],
                   msgdelay->sentcounts[dstidx]);
        }
        printf("\n");
    }

    pthread_mutex_unlock(&_tester__lock__);
}
