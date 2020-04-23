/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     chat
 * @{
 *
 * @file
 * @brief       Chat serial communication
 *
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "chat.h"

#include "cbor.h"
#include "cborjson.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static int _parse_chat_id(CborValue *map_it, const char *key, chat_id_t *id)
{
    DEBUG("_parse_chat_id(%p, %p, %p)\n", map_it, key, id);

    assert(map_it != NULL && key != NULL && id != NULL);

    /* Find value in map */
    CborValue id_it;
    cbor_value_map_find_value(map_it, key, &id_it);

    /* Verify we've got a valid byte string */
    if (!cbor_value_is_valid(&id_it) &&
        !cbor_value_is_byte_string(&id_it)) {
        return -1;
    }

    /* Verify length is okay */
    size_t len;
    cbor_value_calculate_string_length(&id_it, &len);
    if (len != sizeof(chat_id_t)) {
        return -1;
    }

    DEBUG("_parse_chat_id: len = %08lx\n", (uint32_t)len);

    /* Copy byte string */
    cbor_value_copy_byte_string(&id_it, id->u8, &len, NULL);
    return 0;
}

static int _parse_msg_content(CborValue *map_it, const char *key,
                              chat_msg_content_t *content)
{
    DEBUG("_parse_msg_content(%p, %p, %p)\n", map_it, key, content);

    assert(map_it != NULL && key != NULL && content != NULL);

    /* Find value in the map */
    CborValue content_it;
    cbor_value_map_find_value(map_it, key, &content_it);

    /* Verify we've got a byte string */
    if (!cbor_value_is_valid(&content_it) &&
        !cbor_value_is_byte_string(&content_it)) {
        DEBUG("it's not valid\n");
        return -1;
    }

    /* Verify length is okay */
    size_t len;
    cbor_value_calculate_string_length(&content_it, &len);

    /* Copy byte string */
    content->len = len;
    cbor_value_copy_byte_string(&content_it, content->buf, &len, NULL);
    return 0;
}

static int _parse_uint64(CborValue *map_it, const char *key, uint64_t *out)
{
    DEBUG("_parse_uint64(%p, %p, %p)\n", map_it, key, out);

    assert(map_it != NULL && key != NULL && out != NULL);

    /* Find value in the map */
    CborValue int_it;
    cbor_value_map_find_value(map_it, key, &int_it);

    /* Verify we've got a valid integer */
    if (!cbor_value_is_valid(&int_it) &&
        !cbor_value_is_integer(&int_it)) {
        return -1;
    }

    /* Copy byte string */
    cbor_value_get_uint64(&int_it, out);
    return 0;
}


int chat_parse_msg(chat_msg_t *msg, uint8_t *buffer, size_t len)
{
    DEBUG("chat_parse_msg(%p, %p, %d)\n", msg, buffer, len);

    assert(msg != NULL && buffer != NULL && len != 0);

    CborParser parser;
    CborValue it;

    if (cbor_parser_init(buffer, len, 0, &parser, &it) != CborNoError) {
        DEBUG("chat: couldn't parse chat cbor input\n");
        return -1;
    }

    if (!cbor_value_is_map(&it)) {
        DEBUG("chat: not a map\n");
        return -1;
    }

    /* Parse fromUID */
    if (_parse_chat_id(&it, "fromUID", &msg->from_uid) < 0) {
        DEBUG("chat: fromUID is invalid!\n");
        return -1;
    }

    /* Parse toUID */
    if (_parse_chat_id(&it, "toUID", &msg->to_uid) < 0) {
        DEBUG("chat: toUID is invalid!\n");
    }

    /* Parse msgID */
    if (_parse_chat_id(&it, "msgID", &msg->msg_id) < 0) {
        DEBUG("chat: msgID is invalid!\n");
        return -1;
    }

    /* Parse message content */
    if (_parse_msg_content(&it, "msg", &msg->msg) < 0) {
        DEBUG("chat: msg is invalid!\n");
        return -1;
    }

    /* Parse timestamp */
    if (_parse_uint64(&it, "timestamp", &msg->timestamp) < 0) {
        DEBUG("chat: invalid timestamp!\n");
        return -1;
    }

    /* Parse type */
    if (_parse_uint64(&it, "type", &msg->type) < 0) {
        DEBUG("chat: invalid type!\n");
        return -1;
    }

    return 0;
}
