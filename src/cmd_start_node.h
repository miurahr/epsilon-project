/*
 * $Id: cmd_start_node.h,v 1.18 2010/02/05 23:50:22 simakov Exp $
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

#ifndef __START_NODE_H__
#define __START_NODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_CLUSTER

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/* Shutcuts for ease of casting */
#define SA                      struct sockaddr
#define CSA                     const struct sockaddr

/* Network timeout and queue size */
#define MAX_QUEUE               64
#define IO_TIMEOUT              15

/* Default node settings */
#define DEFAULT_PORT            2718
#define DEFAULT_NODE_LIST       ".epsilon.nodes"

/* Limits */
#define MAX_NODE_LINE           512
#define MAX_CLUSTER_NODES       512

/* Available node actions */
#define ACTION_ENCODE_GS        0
#define ACTION_ENCODE_TC        1
#define ACTION_DECODE_GS        2
#define ACTION_DECODE_TC        3

/* Start timer */
#define TIMER_START(_start) {                                           \
    gettimeofday(&_start, NULL);                                        \
}

/* Stop timer and increment total sum */
#define TIMER_STOP(_start,_stop,_total) {                               \
    gettimeofday(&_stop, NULL);                                         \
    _total += ((double) (_stop.tv_sec - _start.tv_sec) +                \
        (_stop.tv_usec - _start.tv_usec) / 1000000.0);                  \
}

/* Very helpful shutcuts to reduce typing */
#define RECV_VALUE_FROM_MASTER(_x) {                                    \
    switch (receive_value(sock_fd, _x)) {                               \
        case -1:                                                        \
            syslog(LOG_ERR, "Cannot receive value from %s port %d: %m", \
                ip_addr, port);                                         \
            return;                                                     \
        case 0:                                                         \
            syslog(LOG_INFO, "Connection to %s port %d closed",         \
                ip_addr, port);                                         \
            return;                                                     \
        case 1:                                                         \
            /* Nothing */                                               \
            break;                                                      \
        default:                                                        \
            assert(0);                                                  \
    }                                                                   \
}

#define RECV_BUF_FROM_MASTER(_x, _len) {                                \
    ssize_t _nread = readn(sock_fd, _x, _len);                          \
                                                                        \
    if (_nread == 0) {                                                  \
        syslog(LOG_INFO, "Connection to %s port %d closed",             \
            ip_addr, port);                                             \
    } else if (_nread != _len) {                                        \
        syslog(LOG_ERR, "Cannot receive buffer from %s port %d: %m",    \
            ip_addr, port);                                             \
        return;                                                         \
    }                                                                   \
}

#define SEND_VALUE_TO_MASTER(_x) {                                      \
    if (send_value(sock_fd, _x) != 0) {                                 \
        syslog(LOG_ERR, "Cannot send value to %s port %d: %m",          \
            ip_addr, port);                                             \
        return;                                                         \
    }                                                                   \
}

#define SEND_BUF_TO_MASTER(_x, _len) {                                  \
    if (writen(sock_fd, _x, _len) != _len) {                            \
        syslog(LOG_ERR, "Cannot send buffer to %s port %d: %m",         \
            ip_addr, port);                                             \
        return;                                                         \
    }                                                                   \
}

#define RECV_VALUE_FROM_SLAVE(_x) {                                     \
    if (receive_value(sock_fd, _x) != 1) {                              \
        LOCK(p_lock);                                                   \
        printf("%sCannot receive value from %s port %d: %m\n",          \
            QUIET, ip_addr, port);                                      \
        UNLOCK(p_lock);                                                 \
                                                                        \
        error_flag = 1;                                                 \
        goto error;                                                     \
    }                                                                   \
}

#define RECV_BUF_FROM_SLAVE(_x, _len) {                                 \
    if (readn(sock_fd, _x, _len) != _len) {                             \
        LOCK(p_lock);                                                   \
        printf("%sCannot receive buffer from %s port %d: %m\n",         \
            QUIET, ip_addr, port);                                      \
        UNLOCK(p_lock);                                                 \
                                                                        \
        error_flag = 1;                                                 \
        goto error;                                                     \
    }                                                                   \
}

#define SEND_VALUE_TO_SLAVE(_x) {                                       \
    if (send_value(sock_fd, _x) != 0) {                                 \
        LOCK(p_lock);                                                   \
        printf("%sCannot send value to %s port %d: %m\n",               \
            QUIET, ip_addr, port);                                      \
        UNLOCK(p_lock);                                                 \
                                                                        \
        error_flag = 1;                                                 \
        goto error;                                                     \
    }                                                                   \
}

#define SEND_BUF_TO_SLAVE(_x, _len) {                                   \
    if (writen(sock_fd, _x, _len) != _len) {                            \
        LOCK(p_lock);                                                   \
        printf("%sCannot send buffer to %s port %d: %m\n",              \
            QUIET, ip_addr, port);                                      \
        UNLOCK(p_lock);                                                 \
                                                                        \
        error_flag = 1;                                                 \
        goto error;                                                     \
    }                                                                   \
}

void set_signal(int sig, void (*sig_handler)(int));
ssize_t readn(int fd, void *buf, size_t nbytes);
ssize_t writen(int fd, void *buf, size_t nbytes);
int send_value(int fd, int value);
int receive_value(int fd, int *value);
static void daemon_init();
static void sigterm_handler(int sig);
static void sigchld_handler(int sig);
static void start_server(int port);
static void client_request(int sock_fd, struct sockaddr_in *cli_addr);
static void action_encode_gs(int sock_fd, char *ip_addr, int port);
static void action_encode_tc(int sock_fd, char *ip_addr, int port);
static void action_decode_gs(int sock_fd, char *ip_addr, int port);
static void action_decode_tc(int sock_fd, char *ip_addr, int port);
int file_exists(char *pathname);
int load_cluster_nodes(char *pathname, struct sockaddr_in *nodes);
void cmd_start_node(int port);

#endif /* ENABLE_CLUSTER */

#ifdef __cplusplus
}
#endif

#endif /* __START_NODE_H__ */
