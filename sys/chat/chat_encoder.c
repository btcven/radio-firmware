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
 * @brief       Chat message encoder
 *
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "chat.h"

#include "cbor.h"

size_t chat_encode_msg(chat_msg_t *msg, uint8_t *buffer, size_t len)
{
    assert(msg != NULL && buffer != NULL && len != 0);

    CborEncoder encoder;
    cbor_encoder_init(&encoder, buffer, len, 0);

    CborEncoder map_encoder;
    cbor_encoder_create_map(&encoder, &map_encoder, 6);

    cbor_encode_text_stringz(&map_encoder, "fromUID");
    cbor_encode_byte_string(&map_encoder, msg->from_uid.u8, sizeof(chat_id_t));

    cbor_encode_text_stringz(&map_encoder, "toUID");
    cbor_encode_byte_string(&map_encoder, msg->to_uid.u8, sizeof(chat_id_t));

    cbor_encode_text_stringz(&map_encoder, "msgID");
    cbor_encode_byte_string(&map_encoder, msg->to_uid.u8, sizeof(chat_id_t));

    cbor_encode_text_stringz(&map_encoder, "msg");
    cbor_encode_byte_string(&map_encoder, msg->msg.buf, msg->msg.len);

    cbor_encode_text_stringz(&map_encoder, "timestamp");
    cbor_encode_uint(&map_encoder, msg->timestamp);

    cbor_encode_text_stringz(&map_encoder, "type");
    cbor_encode_uint(&map_encoder, msg->type);

    cbor_encoder_close_container(&encoder, &map_encoder);

    return cbor_encoder_get_buffer_size(&encoder, buffer);
}
