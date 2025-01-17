// Copyright 2021 IOTA Stiftung
// SPDX-License-Identifier: Apache-2.0

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdio.h>

#include "client/api/v1/get_message.h"
#include "client/api/v1/send_message.h"

#include "iota_conf.h"

/* Exported functions --------------------------------------------------------*/

int send_data_message(void)
{
  iota_client_conf_t ctx = {.host = NODE_HOST, .port = NODE_HOST_PORT, .use_tls = NODE_USE_TLS};

  // send Hello world to the Tangle
  printf("Sending data message to the Tangle...\n");
  res_send_message_t res;
  memset(&res, 0, sizeof(res_send_message_t));
  int ret = send_indexation_msg(&ctx, "iota.c\xF0\x9F\xA6\x8B", "Hello World", &res);
  if (ret == 0) {
    if (!res.is_error) {
      printf("message: https://explorer.iota.org/mainnet/message/%s\n", res.u.msg_id);

    } else {
      printf("Node response: %s\n", res.u.error->msg);
      res_err_free(res.u.error);
    }
  } else {
    printf("send_indexation_msg API failed\n");
  }

  // fetch message we just sent
  printf("Fetching data from the Tangle...\n");
  char data_buffer[128] = {0};
  res_message_t *msg = res_message_new();
  if (msg == NULL) {
    printf("new message response object failed\n");
    return -1;
  }

  // get message via the message ID
  if (get_message_by_id(&ctx, res.u.msg_id, msg) == 0) {
    if (msg->is_error) {
      printf("Node response: %s\n", msg->u.error->msg);
    } else {
      if (msg->u.msg->type == MSG_PAYLOAD_INDEXATION) {
        payload_index_t *index = (payload_index_t *)msg->u.msg->payload;
        hex_2_bin((char *)index->data->data, strlen((char *)index->data->data), (byte_t *)data_buffer,
                  sizeof(data_buffer));
        printf("Hex: %s\n", index->data->data);
        printf("data: %s\n", data_buffer);
      }
    }
  }

  res_message_free(msg);
  return 0;
}
