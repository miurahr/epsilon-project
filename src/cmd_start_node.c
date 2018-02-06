/*
 * $Id: cmd_start_node.c,v 1.23 2010/03/19 22:57:29 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
 *
 * This file is part of EPSILON
 *
 * EPSILON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EPSILON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
 *
 * http://epsilon-project.sourceforge.net
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_CLUSTER

#include <cmd_start_node.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <epsilon.h>
#include <options.h>
#include <misc.h>

/*
 * Set signal handler 'sig_handler' for signal 'sig'.
 * Try to restart interupted system calls automaticaly.
 */

void set_signal(int sig, void (*sig_handler)(int))
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;

    if (sigaction(sig, &sa, 0) == -1) {
        syslog(LOG_ERR, "sigaction(): %m");
        exit(1);
    }
}

/*
 * Read at most 'nbytes' to 'buf' from 'fd'. Return
 * actual number of read bytes. Restart read() if it
 * was interupted by signal. Function also returns
 * when gets EOF.
 */

ssize_t readn(int fd, void *buf, size_t nbytes)
{
    size_t nleft;
    ssize_t nread;
    char *p;

    p = buf;
    nleft = nbytes;

    while (nleft > 0) {
        if ((nread = read(fd, p, nleft)) == -1) {
            if (errno == EINTR) {
                nread = 0;
            } else {
                return -1;
            }
        } else if (nread == 0) {
            break;
        }

        nleft -= nread;
        p += nread;
    }

    return (nbytes - nleft);
}

/*
 * Write 'nbytes' of 'buf' into 'fd'. Restart write() if
 * it was interupted by signal.
 */

ssize_t writen(int fd, void *buf, size_t nbytes)
{
    size_t nleft;
    ssize_t nwritten;
    char *p;

    p = buf;
    nleft = nbytes;

    while (nleft > 0) {
        if ((nwritten = write(fd, p, nleft)) == -1) {
            if (errno == EINTR) {
                nwritten = 0;
            } else {
                return -1;
            }
        }

        nleft -= nwritten;
        p += nwritten;
    }

    return nbytes;
}

/* Send integer value using network byte order */
int send_value(int fd, int value) {
    uint32_t x = htonl(value);

    if (writen(fd, &x, sizeof(uint32_t)) != sizeof(uint32_t)) {
        return -1;
    }

    return 0;
}

/* Receive integer value using network byte order */
int receive_value(int fd, int *value) {
    ssize_t nbytes;
    uint32_t x = 0;
    *value = 0;

    nbytes = readn(fd, &x, sizeof(uint32_t));

    if (nbytes == 0) {
        return 0;
    }

    if (nbytes == -1 || nbytes != sizeof(uint32_t)) {
        return -1;
    }

    *value = ntohl(x);
    return 1;
}

/* Daemonize cluster node */
static void daemon_init()
{
    int i, max_fd;
    pid_t pid;

    openlog("epsilon", LOG_PID, LOG_DAEMON);

    if ((pid = fork()) == -1) {
        syslog(LOG_ERR, "fork(): %m");
        exit(1);
    }

    if (pid) {
        exit(0);
    }

    if (setsid() == -1) {
        syslog(LOG_ERR, "setsid(): %m");
        exit(1);
    }

    set_signal(SIGHUP, SIG_IGN);

    if ((pid = fork()) == -1) {
        syslog(LOG_ERR, "fork(): %m");
        exit(1);
    }

    if (pid) {
        exit(0);
    }

    for (i = 0, max_fd = getdtablesize(); i < max_fd; i++) {
        close(i);
    }
}

/* Shutdown cluster node */
static void sigterm_handler(int sig)
{
    syslog(LOG_INFO, "EPSILON cluster node is DOWN");
    exit(0);
}

/* Wait for children processes */
static void sigchld_handler(int sig)
{
    pid_t pid;
    int stat;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
}

/*
 * Listen incoming client connections. For each client new
 * process is forked and called client_request().
 */

static void start_server(int port)
{
    struct sockaddr_in cli_addr, srv_addr;
    int listen_fd, conn_fd, opt;
    struct timeval timeout;
    socklen_t cli_len;
    sigset_t sig_block;
    pid_t pid;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_ERR, "socket(): %m");
        exit(1);
    }

    opt = 1;

    /* Re-use local address */
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        syslog(LOG_ERR, "setsockopt(): %m");
        exit(1);
    }

    memset(&srv_addr, 0, sizeof(srv_addr));

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (bind(listen_fd, (SA *) &srv_addr, sizeof(srv_addr)) == -1) {
        syslog(LOG_ERR, "bind(): %m");
        exit(1);
    }

    if (listen(listen_fd, MAX_QUEUE) == -1) {
        syslog(LOG_ERR, "listen(): %m");
        exit(1);
    }

    /* Set signal handlers */
    set_signal(SIGTERM, sigterm_handler);
    set_signal(SIGCHLD, sigchld_handler);
    set_signal(SIGPIPE, SIG_IGN);

    /* Choose signals to block before forking child process.
     * Those signals will be unblocked for server process
     * after forking new child. For child process those
     * signals have no sence and will be reset to default
     * (i.e. process termination). */
    sigemptyset(&sig_block);
    sigaddset(&sig_block, SIGTERM);
    sigaddset(&sig_block, SIGCHLD);

    timeout.tv_sec = IO_TIMEOUT;
    timeout.tv_usec = 0;

    syslog(LOG_INFO, "EPSILON cluster node is UP");

    for (;;) {
        cli_len = sizeof(cli_addr);

        if ((conn_fd = accept(listen_fd, (SA *) &cli_addr, &cli_len)) == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                syslog(LOG_ERR, "accept(): %m");
                exit(1);
            }
        }

        /* Block SIGTERM, SIGHUP and SIGCHLD */
        sigprocmask(SIG_BLOCK, &sig_block, NULL);

        /* Set receive time-out */
        if (setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
            syslog(LOG_ERR, "setsockopt(): %m");
            exit(1);
        }

        /* Set send time-out */
        if (setsockopt(conn_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
            syslog(LOG_ERR, "setsockopt(): %m");
            exit(1);
        }

        /* Fork new process for client */
        if ((pid = fork()) == -1) {
            syslog(LOG_ERR, "fork(): %m");
            exit(1);
        }

        if (pid == 0) {
            /* Reset signal handlers to defaults and unblock them */
            set_signal(SIGTERM, SIG_DFL);
            set_signal(SIGCHLD, SIG_DFL);
            sigprocmask(SIG_UNBLOCK, &sig_block, NULL);

            /* Close unnecessary (for child) descriptor */
            close(listen_fd);
            client_request(conn_fd, &cli_addr);

            exit(0);
        }

        /* Unblock 'sig_block' signals for server */
        sigprocmask(SIG_UNBLOCK, &sig_block, NULL);

        close(conn_fd);
    }
}

/* Handle client request */
static void client_request(int sock_fd, struct sockaddr_in *cli_addr)
{
    char ip_addr[INET_ADDRSTRLEN];
    int port;
    int action;

    inet_ntop(AF_INET, &cli_addr->sin_addr, ip_addr, INET_ADDRSTRLEN);
    port = ntohs(cli_addr->sin_port);

    syslog(LOG_INFO, "Connection from %s port %d", ip_addr, port);

    /* Get core parameters */
    RECV_VALUE_FROM_MASTER(&action);

    switch (action) {
        case ACTION_ENCODE_GS:
        {
            action_encode_gs(sock_fd, ip_addr, port);
            break;
        }
        case ACTION_ENCODE_TC:
        {
            action_encode_tc(sock_fd, ip_addr, port);
            break;
        }
        case ACTION_DECODE_GS:
        {
            action_decode_gs(sock_fd, ip_addr, port);
            break;
        }
        case ACTION_DECODE_TC:
        {
            action_decode_tc(sock_fd, ip_addr, port);
            break;
        }
        default:
        {
            syslog(LOG_ERR,
                "Unsupported action (%d) from %s port %d",
                action, ip_addr, ntohs(cli_addr->sin_port));
            return;
        }
    }
}

/* Encode GS blocks */
static void action_encode_gs(int sock_fd, char *ip_addr, int port)
{
    int bytes_per_block;
    int block_size;
    int mode;

    int W, H;
    int w, h;
    int x, y;

    char filter[64];
    int filter_len;

    unsigned char *Y0;
    unsigned char **Y;
    unsigned char *buf;

    /* Receive general parameters */
    RECV_VALUE_FROM_MASTER(&W);
    RECV_VALUE_FROM_MASTER(&H);
    RECV_VALUE_FROM_MASTER(&block_size);
    RECV_VALUE_FROM_MASTER(&bytes_per_block);
    RECV_VALUE_FROM_MASTER(&mode);
    RECV_VALUE_FROM_MASTER(&filter_len);

    if (filter_len >= sizeof(filter)) {
        syslog(LOG_ERR,
            "Filter length (%d) from from %s port %d too large",
            filter_len, ip_addr, port);
        return;
    }

    RECV_BUF_FROM_MASTER(&filter, filter_len);
    filter[filter_len] = 0;

    /* Allocate memory (it will be released implicitly at exit) */
    Y0 = (unsigned char *) xmalloc(block_size * block_size,
        sizeof(unsigned char));
    Y = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    buf = (unsigned char *) xmalloc(bytes_per_block,
        sizeof(unsigned char));

    /* Process all incoming blocks */
    while (1) {
        /* Read/Write/Encode timers */
        struct timeval r_time_start, r_time_stop;
        struct timeval w_time_start, w_time_stop;
        struct timeval e_time_start, e_time_stop;

        /* Total timers */
        double read_time = 0.0;
        double write_time = 0.0;
        double encode_time = 0.0;

        int buf_size;
        int rc;

        /* Get block-specific parameters */
        RECV_VALUE_FROM_MASTER(&x);
        TIMER_START(r_time_start);
        RECV_VALUE_FROM_MASTER(&y);
        RECV_VALUE_FROM_MASTER(&w);
        RECV_VALUE_FROM_MASTER(&h);
        RECV_BUF_FROM_MASTER(Y0, w * h);
        TIMER_STOP(r_time_start, r_time_stop, read_time);

        /* Transform Y channel */
        TIMER_START(e_time_start);
        transform_1D_to_2D(Y0, Y, w, h);
        buf_size = bytes_per_block - 1;

        /* Encode GS block */
        rc = eps_encode_grayscale_block(Y, W, H, w, h, x, y,
            buf, &buf_size, filter, mode);

        assert(rc == EPS_OK);
        TIMER_STOP(e_time_start, e_time_stop, encode_time);

        /* Send encoded data to the MASTER */
        TIMER_START(w_time_start);
        SEND_VALUE_TO_MASTER(buf_size);
        SEND_BUF_TO_MASTER(buf, buf_size);
        TIMER_STOP(w_time_start, w_time_stop, write_time);

        /* Syslog statistics */
        syslog(LOG_INFO,
            "[ENCODE GS] W=%d H=%d w=%d h=%d x=%d y=%d rtime=%.3f "
            "wtime=%.3f etime=%.3f bs=%d bpb=%d m=%s f=%s",
            W, H, w, h, x, y, read_time, write_time, encode_time,
            block_size, bytes_per_block,
            mode == EPS_MODE_NORMAL ? "normal" : "otlpf", filter);
    }
}

/* Encode TC blocks */
static void action_encode_tc(int sock_fd, char *ip_addr, int port)
{
    int bytes_per_block;
    int block_size;
    int resample;
    int mode;

    int W, H;
    int x, y;
    int w, h;

    char filter[64];
    int filter_len;

    int Y_ratio;
    int Cb_ratio;
    int Cr_ratio;

    unsigned char *Y0;
    unsigned char **R;
    unsigned char **G;
    unsigned char **B;
    unsigned char *buf;

    /* Receive general parameters */
    RECV_VALUE_FROM_MASTER(&W);
    RECV_VALUE_FROM_MASTER(&H);
    RECV_VALUE_FROM_MASTER(&block_size);
    RECV_VALUE_FROM_MASTER(&bytes_per_block);
    RECV_VALUE_FROM_MASTER(&mode);
    RECV_VALUE_FROM_MASTER(&filter_len);

    if (filter_len >= sizeof(filter)) {
        syslog(LOG_ERR,
            "Filter length (%d) from from %s port %d too large",
            filter_len, ip_addr, port);
        return;
    }

    RECV_BUF_FROM_MASTER(&filter, filter_len);
    filter[filter_len] = 0;

    RECV_VALUE_FROM_MASTER(&resample);
    RECV_VALUE_FROM_MASTER(&Y_ratio);
    RECV_VALUE_FROM_MASTER(&Cb_ratio);
    RECV_VALUE_FROM_MASTER(&Cr_ratio);

    /* Allocate memory (it will be released implicitly at exit) */
    Y0 = (unsigned char *) xmalloc(block_size * block_size,
        sizeof(unsigned char));
    R = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    G = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    B = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    buf = (unsigned char *) xmalloc(bytes_per_block,
        sizeof(unsigned char));

    /* Process all incoming blocks */
    while (1) {
        /* Read/Write/Encode timers */
        struct timeval r_time_start, r_time_stop;
        struct timeval w_time_start, w_time_stop;
        struct timeval e_time_start, e_time_stop;

        /* Total timers */
        double read_time = 0.0;
        double write_time = 0.0;
        double encode_time = 0.0;

        int buf_size;
        int rc;

        /* Receive block-specific parameters */
        RECV_VALUE_FROM_MASTER(&x);
        TIMER_START(r_time_start);
        RECV_VALUE_FROM_MASTER(&y);
        RECV_VALUE_FROM_MASTER(&w);
        RECV_VALUE_FROM_MASTER(&h);
        RECV_BUF_FROM_MASTER(Y0, w * h);
        TIMER_STOP(r_time_start, r_time_stop, read_time);

        /* Transform R channel */
        TIMER_START(e_time_start);
        transform_1D_to_2D(Y0, R, w, h);
        TIMER_STOP(e_time_start, e_time_stop, encode_time);

        /* Receive G channel */
        TIMER_START(r_time_start);
        RECV_BUF_FROM_MASTER(Y0, w * h);
        TIMER_STOP(r_time_start, r_time_stop, read_time);

        /* Transform G channel */
        TIMER_START(e_time_start);
        transform_1D_to_2D(Y0, G, w, h);
        TIMER_STOP(e_time_start, e_time_stop, encode_time);

        /* Receive B channel */
        TIMER_START(r_time_start);
        RECV_BUF_FROM_MASTER(Y0, w * h);
        TIMER_STOP(r_time_start, r_time_stop, read_time);

        /* Transform B channel */
        TIMER_START(e_time_start);
        transform_1D_to_2D(Y0, B, w, h);

        buf_size = bytes_per_block - 1;

        /* Encode TC block */
        rc = eps_encode_truecolor_block(R, G, B, W, H, w, h,
            x, y, resample, buf, &buf_size, Y_ratio,
            Cb_ratio, Cr_ratio, filter, mode);

        assert(rc == EPS_OK);
        TIMER_STOP(e_time_start, e_time_stop, encode_time);

        /* Send encoded data to the MASTER */
        TIMER_START(w_time_start);
        SEND_VALUE_TO_MASTER(buf_size);
        SEND_BUF_TO_MASTER(buf, buf_size);
        TIMER_STOP(w_time_start, w_time_stop, write_time);

        /* Syslog statistics */
        syslog(LOG_INFO,
            "[ENCODE TC] W=%d H=%d w=%d h=%d x=%d y=%d rtime=%.3f "
            "wtime=%.3f etime=%.3f bs=%d bpb=%d m=%s f=%s "
            "Y_rt=%d Cb_rt=%d Cr_rt=%d resample=%s",
            W, H, w, h, x, y, read_time, write_time, encode_time,
            block_size, bytes_per_block,
            mode == EPS_MODE_NORMAL ? "normal" : "otlpf", filter,
            Y_ratio, Cb_ratio, Cr_ratio,
            resample == EPS_RESAMPLE_444 ? "no" : "yes");
    }
}

/* Decode GS blocks */
static void action_decode_gs(int sock_fd, char *ip_addr, int port)
{
    unsigned char *Y0;
    unsigned char **Y;
    unsigned char *buf;
    int block_size;
    int buf_size;

    /* Receive general parameters */
    RECV_VALUE_FROM_MASTER(&buf_size);
    RECV_VALUE_FROM_MASTER(&block_size);

    /* Allocate memory (it will be released implicitly at exit) */
    Y0 = (unsigned char *) xmalloc(block_size * block_size,
        sizeof(unsigned char));
    Y = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    buf = (unsigned char *) xmalloc(buf_size,
        sizeof(unsigned char));

    /* Process all incoming blocks */
    while (1) {
        /* Read/Write/Decode timers */
        struct timeval r_time_start, r_time_stop;
        struct timeval w_time_start, w_time_stop;
        struct timeval d_time_start, d_time_stop;

        /* Total timers */
        double read_time = 0.0;
        double write_time = 0.0;
        double decode_time = 0.0;

        /* PSI header */
        eps_block_header hdr;

        int real_buf_size;
        int rc;

        /* Receive block-specific parameters */
        RECV_VALUE_FROM_MASTER(&real_buf_size);
        TIMER_START(r_time_start);
        RECV_BUF_FROM_MASTER(buf, real_buf_size);
        TIMER_STOP(r_time_start, r_time_stop, read_time);

        /* Parse header (also it should be checked at MASTER side) */
        TIMER_START(d_time_start);
        rc = eps_read_block_header(buf, real_buf_size, &hdr);
        assert(rc == EPS_OK);

        /* Decode GS block */
        rc = eps_decode_grayscale_block(Y, buf, &hdr);
        assert(rc == EPS_OK);

        /* Transform Y channel */
        transform_2D_to_1D(Y, Y0, hdr.hdr_data.gs.w, hdr.hdr_data.gs.h);
        TIMER_STOP(d_time_start, d_time_stop, decode_time);

        /* Send decoded data to the MASTER */
        TIMER_START(w_time_start);
        SEND_BUF_TO_MASTER(Y0, hdr.hdr_data.gs.w * hdr.hdr_data.gs.h);
        TIMER_STOP(w_time_start, w_time_stop, write_time);

        /* Syslog statistics */
        syslog(LOG_INFO,
            "[DECODE GS] W=%d H=%d w=%d h=%d x=%d y=%d rtime=%.3f "
            "wtime=%.3f dtime=%.3f bpb=%d m=%s f=%s ",
            hdr.hdr_data.gs.W, hdr.hdr_data.gs.H,
            hdr.hdr_data.gs.w, hdr.hdr_data.gs.h,
            hdr.hdr_data.gs.x, hdr.hdr_data.gs.y,
            read_time, write_time, decode_time,
            hdr.hdr_size + hdr.data_size,
            hdr.hdr_data.gs.mode == EPS_MODE_NORMAL ? "normal" : "otlpf",
            hdr.hdr_data.gs.fb_id);
    }
}

/* Decode TC blocks */
static void action_decode_tc(int sock_fd, char *ip_addr, int port)
{
    unsigned char *Y0;
    unsigned char **R;
    unsigned char **G;
    unsigned char **B;
    unsigned char *buf;
    int buf_size;
    int block_size;

    /* Receive general parameters */
    RECV_VALUE_FROM_MASTER(&buf_size);
    RECV_VALUE_FROM_MASTER(&block_size);

    /* Allocate memory (it will be released implicitly at exit) */
    Y0 = (unsigned char *) xmalloc(block_size * block_size,
        sizeof(unsigned char));
    R = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    G = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    B = (unsigned char **) eps_malloc_2D(block_size, block_size,
        sizeof(unsigned char));
    buf = (unsigned char *) xmalloc(buf_size,
        sizeof(unsigned char));

    /* Process all incoming blocks */
    while (1) {
        /* Read/Write/Decode timers */
        struct timeval r_time_start, r_time_stop;
        struct timeval w_time_start, w_time_stop;
        struct timeval d_time_start, d_time_stop;

        /* Total timers */
        double read_time = 0.0;
        double write_time = 0.0;
        double decode_time = 0.0;

        /* PSI header */
        eps_block_header hdr;

        int real_buf_size;
        int rc;

        /* Receive block-specific parameters */
        RECV_VALUE_FROM_MASTER(&real_buf_size);
        TIMER_START(r_time_start);
        RECV_BUF_FROM_MASTER(buf, real_buf_size);
        TIMER_STOP(r_time_start, r_time_stop, read_time);

        /* Parse header (also it should be checked at MASTER side) */
        TIMER_START(d_time_start);
        rc = eps_read_block_header(buf, real_buf_size, &hdr);
        assert(rc == EPS_OK);

        /* Decode TC block */
        rc = eps_decode_truecolor_block(R, G, B, buf, &hdr);
        assert(rc == EPS_OK || rc == EPS_FORMAT_ERROR);

        /* Transform R channel */
        transform_2D_to_1D(R, Y0, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
        TIMER_STOP(d_time_start, d_time_stop, decode_time);

        /* Send R channel to the MASTER */
        TIMER_START(w_time_start);
        SEND_BUF_TO_MASTER(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);
        TIMER_STOP(w_time_start, w_time_stop, write_time);

        /* Transform G channel */
        TIMER_START(d_time_start);
        transform_2D_to_1D(G, Y0, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
        TIMER_STOP(d_time_start, d_time_stop, decode_time);

        /* Send G channel to the MASTER */
        TIMER_START(w_time_start);
        SEND_BUF_TO_MASTER(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);
        TIMER_STOP(w_time_start, w_time_stop, write_time);

        /* Transform B channel */
        TIMER_START(d_time_start);
        transform_2D_to_1D(B, Y0, hdr.hdr_data.tc.w, hdr.hdr_data.tc.h);
        TIMER_STOP(d_time_start, d_time_stop, decode_time);

        /* Send B channel to the MASTER */
        TIMER_START(w_time_start);
        SEND_BUF_TO_MASTER(Y0, hdr.hdr_data.tc.w * hdr.hdr_data.tc.h);
        TIMER_STOP(w_time_start, w_time_stop, write_time);

        /* Syslog statistics */
        syslog(LOG_INFO,
            "[DECODE TC] W=%d H=%d w=%d h=%d x=%d y=%d rtime=%.3f "
            "wtime=%.3f dtime=%.3f bpb=%d m=%s f=%s resample=%s",
            hdr.hdr_data.tc.W, hdr.hdr_data.tc.H,
            hdr.hdr_data.tc.w, hdr.hdr_data.tc.h,
            hdr.hdr_data.tc.x, hdr.hdr_data.tc.y,
            read_time, write_time, decode_time,
            hdr.hdr_size + hdr.data_size,
            hdr.hdr_data.tc.mode == EPS_MODE_NORMAL ? "normal" : "otlpf",
            hdr.hdr_data.tc.fb_id,
            hdr.hdr_data.tc.resample == EPS_RESAMPLE_444 ? "no" : "yes");
    }
}

/* Check for file existence */
int file_exists(char *pathname) {
    struct stat st;

    if (stat((const char *) pathname, &st) != 0) {
        return 0;
    }

    return 1;
}

/* Load list of cluster nodes */
int load_cluster_nodes(char *pathname, struct sockaddr_in *nodes) {
    char buf[MAX_NODE_LINE];
    int line = 1;
    int i = 0;
    FILE *f;

    if ((f = fopen(pathname, "rb")) == NULL) {
        printf("Cannot open %s: %m\n", pathname);
        exit(1);
    }

    while (fgets(buf, sizeof(buf), f)) {
        struct hostent *he;
        char user[32];
        char host[64];
        int port;
        int cpu;
        int n, j;

        n = sscanf(buf, "%31[a-zA-Z0-9._-]@%63[a-zA-Z0-9._-]:%d^%d",
            &user, &host, &port, &cpu);

        if (n != 4) {
            printf("Error in %s on line %d: %s", pathname, line, buf);
            printf("Format: user@host:port^cpu\n");
            exit(1);
        }

        if (!(he = gethostbyname((const char *) &host))) {
            printf("Cannot resolve host: %s\n", host);
            exit(1);
        }

        for (j = 0; j < cpu; j++, i++) {
            if (i >= MAX_CLUSTER_NODES) {
                printf("Too many cluster nodes. You should recompile\n");
                printf("EPSILON with larger MAX_CLUSTER_NODES value.\n");
                exit(1);
            }

            /* Fill next socket address structure */
            memset(&nodes[i], 0, sizeof(struct sockaddr_in));
            nodes[i].sin_family = AF_INET;
            nodes[i].sin_port = htons(port);
            memcpy(&nodes[i].sin_addr, he->h_addr_list[0], he->h_length);
        }

        line++;
    }

    fclose(f);
    return i;
}

/* Start cluster node */
void cmd_start_node(int port)
{
    daemon_init();
    start_server(port == OPT_NA ? DEFAULT_PORT : port);
}

#endif /* ENABLE_CLUSTER */
