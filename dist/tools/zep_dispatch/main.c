/*
 * Copyright (C) 2020 Benjamin Valentin
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2. See the file LICENSE for more details.
 */

#ifndef ZEP_DISPATCH_PDU
#define ZEP_DISPATCH_PDU    256
#endif

#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <linux/if.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>

#include "kernel_defines.h"
#include "topology.h"
#include "zep_parser.h"

#define ETH_P_IEEE802154 0x00F6

typedef struct {
    list_node_t node;
    struct sockaddr_in6 addr;
} zep_client_t;

typedef void (*dispatch_cb_t)(void *ctx, void *buffer, size_t len,
                              int sock, struct sockaddr_in6 *src_addr);

/* all nodes are directly connected */
static void _send_flat(void *ctx, void *buffer, size_t len,
                       int sock, struct sockaddr_in6 *src_addr)
{
    list_node_t *head = ctx;
    char addr_str[INET6_ADDRSTRLEN];

    /* send packet to all other clients */
    bool known_node = false;
    list_node_t *prev = head;

    for (list_node_t *n = head->next; n; n = n->next) {
        struct sockaddr_in6 *addr = &container_of(n, zep_client_t, node)->addr;

        /* don't echo packet back to sender */
        if (memcmp(src_addr, addr, sizeof(*addr)) == 0) {
            known_node = true;
            /* remove client if sending fails */
        }
        else if (sendto(sock, buffer, len, 0, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
            inet_ntop(src_addr->sin6_family, &addr->sin6_addr, addr_str, INET6_ADDRSTRLEN);
            printf("removing [%s]:%d\n", addr_str, ntohs(addr->sin6_port));
            prev->next = n->next;
            free(n);
            continue;
        }

        prev = n;
    }

    /* if the client new, add it to the broadcast list */
    if (!known_node) {
        inet_ntop(src_addr->sin6_family, &src_addr->sin6_addr, addr_str, INET6_ADDRSTRLEN);
        printf("adding [%s]:%d\n", addr_str, ntohs(src_addr->sin6_port));
        zep_client_t *client = malloc(sizeof(zep_client_t));
        memcpy(&client->addr, src_addr, sizeof(*src_addr));
        list_add(head, &client->node);
    }
}

/* nodes are connected as described by topology */
static void _send_topology(void *ctx, void *buffer, size_t len,
                           int sock, struct sockaddr_in6 *src_addr)
{
    uint8_t mac_src[8];
    uint8_t mac_src_len;

    if (zep_parse_mac(buffer, len, mac_src, &mac_src_len)) {
        /* a sniffer node has no MAC address and will receive every packet */
        if (mac_src_len == 0) {
            topology_set_sniffer(ctx, src_addr);
        } else {
            topology_add(ctx, mac_src, mac_src_len, src_addr);
        }
    }
    topology_send(ctx, sock, src_addr, buffer, len);
}

static void dispatch_loop(int sock, int tap, dispatch_cb_t dispatch, void *ctx)
{
    puts("entering loop…");
    while (1) {
        uint8_t buffer[ZEP_DISPATCH_PDU];
        struct sockaddr_in6 src_addr;
        socklen_t addr_len = sizeof(src_addr);

        /* receive incoming packet */
        ssize_t bytes_in = recvfrom(sock, buffer, sizeof(buffer), 0,
                                    (struct sockaddr *)&src_addr, &addr_len);

        if (bytes_in <= 0) {
            continue;
        }

        /* send packet to virtual 802.15.4 interface */
        if (tap) {
            size_t len = bytes_in;
            const void *payload = zep_get_payload(buffer, &len);
            if (payload) {
                if (write(tap, payload, len) < 0) {
                    puts("Can't write to virtual 802.15.4 device");
                    close(tap);
                    tap = 0;
                }
            }
        }

        /* send packet to the topology */
        dispatch(ctx, buffer, bytes_in, sock, &src_addr);
    }
}

static topology_t topology;
static const char *graphviz_file = "example.gv";
static const char *pidfile;
static void _info_handler(int signal)
{
    switch (signal) {
    case SIGUSR1:
        if (topology_print(graphviz_file, &topology)) {
            fprintf(stderr, "can't open %s\n", graphviz_file);
        }
        else {
            printf("graph written to %s\n", graphviz_file);
        }
        break;
    case SIGUSR2:
        topology_print_stats(&topology, true);
        break;
    case SIGINT:
    case SIGTERM:
        if (pidfile) {
            unlink(pidfile);
        }
        exit(0);
        break;
    }
}

/* open mac802154_hwsim device to send frames to it */
static int _open_mac802154_hwsim(const char *iface)
{
    int res;
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IEEE802154));

    /* get interface index for interface name */
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);

    if ((res = ioctl(fd, SIOCGIFINDEX, &ifr)) < 0) {
        goto error;
    }

    /* bind socket to the device */
    struct sockaddr_ll sll = {
        .sll_family = AF_PACKET,
        .sll_ifindex = ifr.ifr_ifindex,
        .sll_protocol = htons(ETH_P_IEEE802154),
    };

    if ((res = bind(fd, (struct sockaddr *)&sll, sizeof(sll))) < 0) {
        goto error;
    }

    return fd;

error:
    close(fd);
    fprintf(stderr, "open mac802154_hwsim: %s\n", strerror(res));
    return res;
}

static void _print_help(const char *progname)
{
    fprintf(stderr, "usage: %s [-t topology] [-s seed] "
                    "[-g graphviz_out] [-w interface] <address> <port>\n",
            progname);

    fprintf(stderr, "\npositional arguments:\n");
    fprintf(stderr, "\taddress\t\tlocal address to bind to\n");
    fprintf(stderr, "\tport\t\tlocal port to bind to\n");

    fprintf(stderr, "\noptional arguments:\n");
    fprintf(stderr, "\t-t <file>\tLoad topology from file\n");
    fprintf(stderr, "\t-p <file>\tStore PID in file\n");
    fprintf(stderr, "\t-s <seed>\tRandom seed used to simulate packet loss\n");
    fprintf(stderr, "\t-g <file>\tFile to dump topology as Graphviz visualisation on SIGUSR1\n");
    fprintf(stderr, "\t-w <interface>\tSend frames to virtual 802.15.4 "
                    "interface (mac802154_hwsim)\n");
}

int main(int argc, char **argv)
{
    int c, tap_fd = 0;
    unsigned int seed = time(NULL);
    const char *topo_file = NULL;
    const char *progname = argv[0];

    const struct addrinfo hint = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = IPPROTO_UDP,
        .ai_flags    = AI_NUMERICHOST,
    };

    while ((c = getopt(argc, argv, "t:s:g:w:p:")) != -1) {
        switch (c) {
        case 't':
            topo_file = optarg;
            break;
        case 's':
            seed = atoi(optarg);
            break;
        case 'g':
            graphviz_file = optarg;
            break;
        case 'w':
            tap_fd = _open_mac802154_hwsim(optarg);
            if (tap_fd < 0) {
                return tap_fd;
            }
            break;
        case 'p':
            pidfile = optarg;
            break;
        default:
            _print_help(progname);
            exit(1);
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 2) {
        _print_help(progname);
        exit(1);
    }

    srand(seed);

    if (topo_file) {
        if (topology_parse(topo_file, &topology)) {
            fprintf(stderr, "can't open '%s'\n", topo_file);
            return -1;
        }
        topology.flat = false;
    }
    else {
        topology.flat = true;
    }

    if (graphviz_file) {
        signal(SIGUSR1, _info_handler);
    }
    signal(SIGUSR2, _info_handler);
    signal(SIGTERM, _info_handler);
    signal(SIGINT, _info_handler);

    struct addrinfo *server_addr;
    int res = getaddrinfo(argv[0], argv[1],
                          &hint, &server_addr);

    if (res != 0) {
        perror("getaddrinfo()");
        exit(1);
    }

    int sock = socket(server_addr->ai_family, server_addr->ai_socktype,
                      server_addr->ai_protocol);

    if (sock < 0) {
        perror("socket() failed");
        exit(1);
    }

    if (bind(sock, server_addr->ai_addr, server_addr->ai_addrlen) < 0) {
        perror("bind() failed");
        exit(1);
    }

    freeaddrinfo(server_addr);

    if (pidfile) {
         FILE *pf = fopen(pidfile, "w");
        if (pf) {
            fprintf(pf, "%u", getpid());
            fclose(pf);
        }
    }

    if (topology.flat) {
        dispatch_loop(sock, tap_fd, _send_flat, &topology.nodes);
    }
    else {
        dispatch_loop(sock, tap_fd, _send_topology, &topology);
    }

    close(sock);

    return 0;
}
