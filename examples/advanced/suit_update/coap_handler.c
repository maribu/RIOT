/*
 * SPDX-FileCopyrightText: 2019 Kaspar Schleiser <kaspar@schleiser.de>
 * SPDX-FileCopyrightText: 2019 Freie Universität Berlin
 * SPDX-FileCopyrightText: 2019 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include "log.h"
#include "suit.h"
#include "net/nanocoap.h"
#include "suit/transport/coap.h"
#include "kernel_defines.h"

#ifdef MODULE_RIOTBOOT_SLOT
#include "riotboot/slot.h"
#endif

static ssize_t _riot_board_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                   coap_request_ctx_t *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
}

static ssize_t _version_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                coap_request_ctx_t *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)"NONE", 4);
}

static size_t _trim_end(const char *uri, size_t uri_len)
{
    while (uri_len > 0) {
        switch (uri[uri_len - 1]) {
        case '\0':
        case '\r':
        case '\n':
            uri_len--;
            break;
        default:
            return uri_len;
        }
    }

    return uri_len;
}

static ssize_t _trigger_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                coap_request_ctx_t *context)
{
    (void)context;
    unsigned code;
    size_t payload_len = pkt->payload_len;
    if (payload_len) {
        const char *uri = (char *)pkt->payload;
        size_t uri_len = _trim_end(uri, pkt->payload_len);
        switch (suit_worker_trigger(uri, uri_len)) {
        case 0:
            code = COAP_CODE_CREATED;
            LOG_INFO("suit: received URL: \"%.*s\"\n", (int)uri_len, uri);
            break;
        case -EOVERFLOW:
            code = COAP_CODE_REQUEST_ENTITY_TOO_LARGE;
            break;
        case -EINVAL:
            code = COAP_CODE_BAD_REQUEST;
            break;
        case -EAGAIN:
            code = COAP_CODE_TOO_MANY_REQUESTS;
            break;
        default:
            code = COAP_CODE_INTERNAL_SERVER_ERROR;
            break;
        }
    }
    else {
        code = COAP_CODE_REQUEST_ENTITY_INCOMPLETE;
    }

    return coap_reply_simple(pkt, code, buf, len,
                             COAP_FORMAT_NONE, NULL, 0);
}

#ifdef MODULE_RIOTBOOT_SLOT
static ssize_t _slot_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                             coap_request_ctx_t *context)
{
    /* context is passed either as NULL or 0x1 for /active or /inactive */
    char c = '0';

    if (coap_request_ctx_get_context(context)) {
        c += riotboot_slot_other();
    }
    else {
        c += riotboot_slot_current();
    }

    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
                             COAP_FORMAT_TEXT, (uint8_t *)&c, 1);
}
#endif

#ifdef MODULE_RIOTBOOT_SLOT
NANOCOAP_RESOURCE(slot_active) {
    .path = "/suit/slot/active", .methods = COAP_METHOD_GET, .handler = _slot_handler,
};
NANOCOAP_RESOURCE(slot_inactive) {
   .path = "/suit/slot/inactive", .methods = COAP_METHOD_GET, .handler = _slot_handler, .context = (void *)0x1,
};
#endif
NANOCOAP_RESOURCE(trigger) {
    .path = "/suit/trigger", .methods = COAP_METHOD_PUT | COAP_METHOD_POST, .handler = _trigger_handler,
};
NANOCOAP_RESOURCE(version) {
    .path = "/suit/version", .methods = COAP_METHOD_GET, .handler = _version_handler,
};
NANOCOAP_RESOURCE(board) {
    .path = "/riot/board", .methods = COAP_GET, .handler = _riot_board_handler,
};
