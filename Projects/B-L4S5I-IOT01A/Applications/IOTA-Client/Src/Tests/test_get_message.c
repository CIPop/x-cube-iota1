// Copyright 2020 IOTA Stiftung
// SPDX-License-Identifier: Apache-2.0

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>

#include "unity.h"
#include "test_config.h"

#include "client/api/v1/get_message.h"

/* Private functions -------------------------------------------------------- */
void test_get_msg_by_id(void)
{
  // char const* const msg_id = "7c58a5bab90219de0231293f40ff65ee7d42e64ae917cc5560f7becdcf6cb158";
  char const* const msg_id = "0f70e47835aaa63a87fd52c1dd556dd6f588e7f428413e8196c586eb05cfadaf";
  iota_client_conf_t ctx = {.host = TEST_NODE_HOST, .port = TEST_NODE_PORT, .use_tls = TEST_IS_HTTPS};
  payload_index_t* idx;

  res_message_t* msg = res_message_new();
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT(get_message_by_id(&ctx, msg_id, msg) == 0);
  if (msg->is_error) {
    printf("API response: %s\n", msg->u.error->msg);
  } else {
    switch (msg->u.msg->type) {
      case MSG_PAYLOAD_TRANSACTION:
        printf("it's a transaction message\n");
        break;
      case MSG_PAYLOAD_INDEXATION:
        printf("it's an indexation message\n");
        idx = (payload_index_t*)msg->u.msg->payload;
        printf("index=%s\n", idx->index->data);
        printf("data=%s\n", idx->data->data);
        break;
      case MSG_PAYLOAD_MILESTONE:
        printf("it's a milestone message\n");
        break;
      case MSG_PAYLOAD_UNKNOW:
      default:
        printf("Unknow message\n");
        break;
    }
  }
  res_message_free(msg);
}

void test_deser_indexation(void)
{
  char const* const idx_res =
      "{\"data\":{\"networkId\":\"9466822412763346725\",\"parentMessageIds\":["
      "\"4f73928a39988fe2d1d15b4aa161c6ba0a64e4d164c481f4cc67c51e316c034e\","
      "\"84cd7f307aecc96fe070a701fae586c95736a9dd6fee18df5319da422575f0f7\","
      "\"aea5b8d4844574a8b0b30d4796523d9012d10fdb32347145172a73a51fc9ed9d\","
      "\"f3b616c2669da3f3fbbafc56fb83213d58238e4a4504d360500b9c6f0c78738c\"],\"payload\":{\"type\":2,\"index\":"
      "\"Foo\",\"data\":"
      "\"426172\"},\"nonce\":\"567803\"}}";
  res_message_t* res = res_message_new();
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT(deser_get_message(idx_res, res) == 0);
  TEST_ASSERT(res->is_error == false);

  message_t* msg = res->u.msg;
  TEST_ASSERT_EQUAL_STRING("9466822412763346725", msg->net_id);
  TEST_ASSERT_EQUAL_STRING("567803", msg->nonce);
  TEST_ASSERT_EQUAL_INT(4, api_message_parent_count(msg));
  TEST_ASSERT_EQUAL_MEMORY("4f73928a39988fe2d1d15b4aa161c6ba0a64e4d164c481f4cc67c51e316c034e",
                           api_message_parent_id(msg, 0), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("84cd7f307aecc96fe070a701fae586c95736a9dd6fee18df5319da422575f0f7",
                           api_message_parent_id(msg, 1), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("aea5b8d4844574a8b0b30d4796523d9012d10fdb32347145172a73a51fc9ed9d",
                           api_message_parent_id(msg, 2), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("f3b616c2669da3f3fbbafc56fb83213d58238e4a4504d360500b9c6f0c78738c",
                           api_message_parent_id(msg, 3), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT(msg->type == MSG_PAYLOAD_INDEXATION);
  payload_index_t* idx = (payload_index_t*)msg->payload;
  TEST_ASSERT_EQUAL_STRING("Foo", idx->index->data);
  TEST_ASSERT_EQUAL_STRING("426172", idx->data->data);

  res_message_free(res);
}

void test_deser_milestone(void)
{
  char const* const ms_res =
      "{\"data\":{\"networkId\":\"9466822412763346725\",\"parentMessageIds\":["
      "\"7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3\","
      "\"9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c\","
      "\"ccf9bf6b76a2659f332e17bfdc20f278ce25bc45e807e89cc2ab526cd2101c52\","
      "\"ede431f8907b30c81eee57db80109af0b8b91683c0be2cc3b685bcdc14dbdca5\","
      "\"fe63a9194eadb45e456a3c618d970119dbcac25221dbf5f53e5a838ef6ef518a\"],\"payload\":{\"type\":1,\"index\":123519,"
      "\"timestamp\":1613651642,\"parentMessageIds\":["
      "\"7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3\","
      "\"9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c\","
      "\"ccf9bf6b76a2659f332e17bfdc20f278ce25bc45e807e89cc2ab526cd2101c52\","
      "\"ede431f8907b30c81eee57db80109af0b8b91683c0be2cc3b685bcdc14dbdca5\","
      "\"fe63a9194eadb45e456a3c618d970119dbcac25221dbf5f53e5a838ef6ef518a\"],\"inclusionMerkleProof\":"
      "\"0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8\",\"publicKeys\":["
      "\"7205c145525cee64f1c9363696811d239919d830ad964b4e29359e6475848f5a\","
      "\"e468e82df33d10dea3bd0eadcd7867946a674d207c39f5af4cc44365d268a7e6\"],\"receipt\":null,\"signatures\":["
      "\"2ef781713287ba11efd0f3be37a49c2a08a8fdd1099b36e6fb7c9cb290b1711dd4fe08489ecd3872ac663bebebedd27cd73325d5331542"
      "1d923b77ffd9ab3b0c\","
      "\"c42983ce8e619787bbb5aa89cb0987cf08a26a2e4080039614e3c56e766bc86dce50d6e7dc6907edf653e9cc92c89405389fbc71e759c2"
      "54fa2aa571a93d850f\"]},\"nonce\":\"10760600709663927622\"}}";
  res_message_t* res = res_message_new();
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT(deser_get_message(ms_res, res) == 0);
  TEST_ASSERT(res->is_error == false);

  message_t* msg = res->u.msg;
  TEST_ASSERT_EQUAL_STRING("9466822412763346725", msg->net_id);
  TEST_ASSERT_EQUAL_STRING("10760600709663927622", msg->nonce);
  TEST_ASSERT_EQUAL_INT(5, api_message_parent_count(msg));
  TEST_ASSERT_EQUAL_MEMORY("7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3",
                           api_message_parent_id(msg, 0), 64);
  TEST_ASSERT_EQUAL_MEMORY("9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c",
                           api_message_parent_id(msg, 1), 64);
  TEST_ASSERT_EQUAL_MEMORY("ccf9bf6b76a2659f332e17bfdc20f278ce25bc45e807e89cc2ab526cd2101c52",
                           api_message_parent_id(msg, 2), 64);
  TEST_ASSERT_EQUAL_MEMORY("ede431f8907b30c81eee57db80109af0b8b91683c0be2cc3b685bcdc14dbdca5",
                           api_message_parent_id(msg, 3), 64);
  TEST_ASSERT_EQUAL_MEMORY("fe63a9194eadb45e456a3c618d970119dbcac25221dbf5f53e5a838ef6ef518a",
                           api_message_parent_id(msg, 4), 64);
  TEST_ASSERT(msg->type == MSG_PAYLOAD_MILESTONE);

  payload_milestone_t* ms = (payload_milestone_t*)msg->payload;
  TEST_ASSERT(1613651642 == ms->timestamp);
  TEST_ASSERT(123519 == ms->index);
  TEST_ASSERT_EQUAL_MEMORY("0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8",
                           ms->inclusion_merkle_proof, 65);
  TEST_ASSERT(2 == get_message_milestone_signature_count(res));
  TEST_ASSERT_EQUAL_MEMORY(
      "2ef781713287ba11efd0f3be37a49c2a08a8fdd1099b36e6fb7c9cb290b1711dd4fe08489ecd3872ac663bebebedd27cd73325d53315421d"
      "923b77ffd9ab3b0c",
      get_message_milestone_signature(res, 0), API_SIGNATURE_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY(
      "c42983ce8e619787bbb5aa89cb0987cf08a26a2e4080039614e3c56e766bc86dce50d6e7dc6907edf653e9cc92c89405389fbc71e759c254"
      "fa2aa571a93d850f",
      get_message_milestone_signature(res, 1), API_SIGNATURE_HEX_STR_LEN);

  res_message_free(res);
}

void test_deser_tx1(void)
{
  // case 1: tx payload with 1 input, 1 output, 1 signature
  char const* const tx_res1 =
      "{\"data\":{\"networkId\":\"6530425480034647824\",\"parentMessageIds\":["
      "\"7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3\","
      "\"9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c\","
      "\"ccf9bf6b76a2659f332e17bfdc20f278ce25bc45e807e89cc2ab526cd2101c52\","
      "\"ede431f8907b30c81eee57db80109af0b8b91683c0be2cc3b685bcdc14dbdca5\"],\"payload\":{\"type\":0,\"essence\":{"
      "\"type\":0,\"inputs\":[{\"type\":0,\"transactionId\":"
      "\"2bfbf7463b008c0298103121874f64b59d2b6172154aa14205db2ce0ba553b03\",\"transactionOutputIndex\":0}],\"outputs\":"
      "[{\"type\":0,\"address\":{\"type\":0,\"address\":"
      "\"ad32258255e7cf927a4833f457f220b7187cf975e82aeee2e23fcae5056ab5f4\"},\"amount\":1000}],\"payload\":null},"
      "\"unlockBlocks\":[{\"type\":0,\"signature\":{\"type\":0,\"publicKey\":"
      "\"dd2fb44b9809782af5f31fdbf767a39303365449308f78d6c2652ac9766dbf1a\",\"signature\":"
      "\"e625a71351bbccf87eeaad7e98f6a545306423b2aaf444792a1be8ccfdfe50b358583483c3dbc536b5842eeec381750c6b4495c14932be"
      "47c439a1a8ad242606\"}}]},\"nonce\":\"6416754\"}}";
  res_message_t* res = res_message_new();
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT(deser_get_message(tx_res1, res) == 0);
  TEST_ASSERT(res->is_error == false);

  message_t* msg = res->u.msg;
  TEST_ASSERT_EQUAL_STRING("6530425480034647824", msg->net_id);
  TEST_ASSERT_EQUAL_STRING("6416754", msg->nonce);
  TEST_ASSERT_EQUAL_INT(4, api_message_parent_count(msg));
  TEST_ASSERT_EQUAL_MEMORY("7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3",
                           api_message_parent_id(msg, 0), 64);
  TEST_ASSERT_EQUAL_MEMORY("9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c",
                           api_message_parent_id(msg, 1), 64);
  TEST_ASSERT_EQUAL_MEMORY("ccf9bf6b76a2659f332e17bfdc20f278ce25bc45e807e89cc2ab526cd2101c52",
                           api_message_parent_id(msg, 2), 64);
  TEST_ASSERT_EQUAL_MEMORY("ede431f8907b30c81eee57db80109af0b8b91683c0be2cc3b685bcdc14dbdca5",
                           api_message_parent_id(msg, 3), 64);
  TEST_ASSERT(get_message_payload_type(res) == MSG_PAYLOAD_TRANSACTION);

  payload_tx_t* tx = (payload_tx_t*)msg->payload;
  // validate input transaction ID and transaction output index
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_inputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("2bfbf7463b008c0298103121874f64b59d2b6172154aa14205db2ce0ba553b03",
                           payload_tx_inputs_tx_id(tx, 0), 64);
  TEST_ASSERT_EQUAL_UINT32(0, payload_tx_inputs_tx_output_index(tx, 0));
  TEST_ASSERT_NULL(payload_tx_inputs_tx_id(tx, 1));

  // validate output address and amount
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_outputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("ad32258255e7cf927a4833f457f220b7187cf975e82aeee2e23fcae5056ab5f4",
                           payload_tx_outputs_address(tx, 0), 64);
  TEST_ASSERT(1000 == payload_tx_outputs_amount(tx, 0));

  TEST_ASSERT_NULL(tx->payload);

  // validate unlocked block
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_blocks_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("dd2fb44b9809782af5f31fdbf767a39303365449308f78d6c2652ac9766dbf1a",
                           payload_tx_blocks_public_key(tx, 0), 64);
  TEST_ASSERT_EQUAL_MEMORY(
      "e625a71351bbccf87eeaad7e98f6a545306423b2aaf444792a1be8ccfdfe50b358583483c3dbc536b5842eeec381750c6b4495c14932be47"
      "c439a1a8ad242606",
      payload_tx_blocks_signature(tx, 0), 128);

  res_message_free(res);
}

void test_deser_tx2(void)
{
  // case 2: tx payload with 2 inputs, 2 outputs, 2 signatures
  char const* const tx_res2 =
      "{\"data\":{\"networkId\":\"6530425480034647824\",\"parentMessageIds\":["
      "\"7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3\","
      "\"9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c\","
      "\"fe63a9194eadb45e456a3c618d970119dbcac25221dbf5f53e5a838ef6ef518a\"],\"payload\":{\"type\":0,\"essence\":{"
      "\"type\":0,\"inputs\":[{\"type\":0,\"transactionId\":"
      "\"2bfbf7463b008c0298103121874f64b59d2b6172154aa14205db2ce0ba553b03\",\"transactionOutputIndex\":0},{\"type\":0,"
      "\"transactionId\":\"0bfbf7463b008c0298103121874f64b59d2b6172154aa14205db2ce0ba553b03\","
      "\"transactionOutputIndex\":1}],\"outputs\":[{\"type\":0,\"address\":{\"type\":0,\"address\":"
      "\"ad32258255e7cf927a4833f457f220b7187cf975e82aeee2e23fcae5056ab5f4\"},\"amount\":1000},{\"type\":0,\"address\":{"
      "\"type\":0,\"address\":\"0000000000000000000000000000000000000000000000000000000000000000\"},\"amount\":5000}],"
      "\"payload\":null},\"unlockBlocks\":[{\"type\":0,\"signature\":{\"type\":0,\"publicKey\":"
      "\"dd2fb44b9809782af5f31fdbf767a39303365449308f78d6c2652ac9766dbf1a\",\"signature\":"
      "\"e625a71351bbccf87eeaad7e98f6a545306423b2aaf444792a1be8ccfdfe50b358583483c3dbc536b5842eeec381750c6b4495c14932be"
      "47c439a1a8ad242606\"}},{\"type\":0,\"signature\":{\"type\":0,\"publicKey\":"
      "\"dd2fb44b9809782af5f31fdbf767a39303365449308f78d6c2652ac9766dbf1a\",\"signature\":"
      "\"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000\"}}]},\"nonce\":\"6416754\"}}";

  res_message_t* res = res_message_new();
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT(deser_get_message(tx_res2, res) == 0);
  TEST_ASSERT(res->is_error == false);

  message_t* msg = res->u.msg;
  TEST_ASSERT_EQUAL_STRING("6530425480034647824", msg->net_id);
  TEST_ASSERT_EQUAL_STRING("6416754", msg->nonce);
  TEST_ASSERT_EQUAL_INT(3, api_message_parent_count(msg));
  TEST_ASSERT_EQUAL_MEMORY("7dabd008324378d65e607975e9f1740aa8b2f624b9e25248370454dcd07027f3",
                           api_message_parent_id(msg, 0), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("9f5066de0e3225f062e9ac8c285306f56815677fe5d1db0bbccecfc8f7f1e82c",
                           api_message_parent_id(msg, 1), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("fe63a9194eadb45e456a3c618d970119dbcac25221dbf5f53e5a838ef6ef518a",
                           api_message_parent_id(msg, 2), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT(get_message_payload_type(res) == MSG_PAYLOAD_TRANSACTION);

  payload_tx_t* tx = (payload_tx_t*)msg->payload;
  // validate input transaction ID and transaction output index
  TEST_ASSERT_EQUAL_UINT32(2, payload_tx_inputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("2bfbf7463b008c0298103121874f64b59d2b6172154aa14205db2ce0ba553b03",
                           payload_tx_inputs_tx_id(tx, 0), API_TX_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_UINT32(0, payload_tx_inputs_tx_output_index(tx, 0));
  TEST_ASSERT_NOT_NULL(payload_tx_inputs_tx_id(tx, 1));
  TEST_ASSERT_EQUAL_MEMORY("0bfbf7463b008c0298103121874f64b59d2b6172154aa14205db2ce0ba553b03",
                           payload_tx_inputs_tx_id(tx, 1), API_TX_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_inputs_tx_output_index(tx, 1));

  // validate output address and amount
  TEST_ASSERT_EQUAL_UINT32(2, payload_tx_outputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("0000000000000000000000000000000000000000000000000000000000000000",
                           payload_tx_outputs_address(tx, 1), API_ADDR_HEX_STR_LEN);
  TEST_ASSERT(5000 == payload_tx_outputs_amount(tx, 1));

  TEST_ASSERT_NULL(tx->payload);

  // validate unlocked block
  TEST_ASSERT_EQUAL_UINT32(2, payload_tx_blocks_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("dd2fb44b9809782af5f31fdbf767a39303365449308f78d6c2652ac9766dbf1a",
                           payload_tx_blocks_public_key(tx, 1), API_PUB_KEY_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY(
      "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000",
      payload_tx_blocks_signature(tx, 1), API_SIGNATURE_HEX_STR_LEN);

  res_message_free(res);
}

void test_deser_tx3(void)
{
  // case 3: tx payload with 2 inputs, 2 outputs, 1 signature, 1 reference
  char const* const tx_with_ref =
      "{\"data\":{\"networkId\":\"14379272398717627559\",\"parentMessageIds\":["
      "\"463d4c237c792f0fa049873b79ef30e6d8873208ec57b97a272cb9fdef1c3689\","
      "\"5800b7bfe01decf85609494fb177e95b47f89addbc78775a987405c99eb8ef71\","
      "\"580907add28cac6a40a07fa141fc03b531d5a61de0713ac05a648e79c64015c2\","
      "\"ffb88663d28855e64b5f8b00c27e6bdccffadb7b1c034518197547e619a22a61\"],\"payload\":{\"type\":0,\"essence\":{"
      "\"type\":0,\"inputs\":[{\"type\":0,\"transactionId\":"
      "\"17057e92991f836ff2f0f88f2abb93ba0d8eda37efc1312daad599c1326bce31\",\"transactionOutputIndex\":1},{\"type\":0,"
      "\"transactionId\":\"7f558c37e8b5d68e290a9269a77327eec9c564eba8f707ad3905de0f8fb04cba\","
      "\"transactionOutputIndex\":1}],\"outputs\":[{\"type\":0,\"address\":{\"type\":0,\"address\":"
      "\"663e6d9dc9955691ede73e1a81fef87af7b94f167524b5e6f92aa559b89185db\"},\"amount\":1000000},{\"type\":0,"
      "\"address\":{\"type\":0,\"address\":\"96f9de0989e77d0e150e850a5a600e83045fa57419eaf3b20225b763d4e23813\"},"
      "\"amount\":1200045}],\"payload\":null},\"unlockBlocks\":[{\"type\":0,\"signature\":{\"type\":0,\"publicKey\":"
      "\"2baaf3bca8ace9f862e60184bd3e79df25ff230f7eaaa4c7f03daa9833ba854a\",\"signature\":"
      "\"cb4ece3f2d7e4903b17d45d41c26685fae9ed04e61294c94095ba248e4eae8cbed60addbd57cabd2df633f0c3f51644fa141a612df81c1"
      "f18942e20bbaf4d102\"}},{\"type\":1,\"reference\":0}]},\"nonce\":\"9223372036857144820\"}}";

  res_message_t* res = res_message_new();
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT(deser_get_message(tx_with_ref, res) == 0);
  TEST_ASSERT(res->is_error == false);

  message_t* msg = res->u.msg;
  TEST_ASSERT_EQUAL_STRING("14379272398717627559", msg->net_id);
  TEST_ASSERT_EQUAL_STRING("9223372036857144820", msg->nonce);
  TEST_ASSERT_EQUAL_INT(4, api_message_parent_count(msg));
  TEST_ASSERT_EQUAL_MEMORY("463d4c237c792f0fa049873b79ef30e6d8873208ec57b97a272cb9fdef1c3689",
                           api_message_parent_id(msg, 0), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("5800b7bfe01decf85609494fb177e95b47f89addbc78775a987405c99eb8ef71",
                           api_message_parent_id(msg, 1), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_NULL(api_message_parent_id(msg, 5));

  TEST_ASSERT(get_message_payload_type(res) == MSG_PAYLOAD_TRANSACTION);

  payload_tx_t* tx = (payload_tx_t*)msg->payload;
  // validate input transaction ID and transaction output index
  TEST_ASSERT_EQUAL_UINT32(2, payload_tx_inputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("17057e92991f836ff2f0f88f2abb93ba0d8eda37efc1312daad599c1326bce31",
                           payload_tx_inputs_tx_id(tx, 0), API_TX_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_inputs_tx_output_index(tx, 0));
  TEST_ASSERT_NOT_NULL(payload_tx_inputs_tx_id(tx, 1));
  TEST_ASSERT_EQUAL_MEMORY("7f558c37e8b5d68e290a9269a77327eec9c564eba8f707ad3905de0f8fb04cba",
                           payload_tx_inputs_tx_id(tx, 1), API_TX_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_inputs_tx_output_index(tx, 1));

  // validate output address and amount
  TEST_ASSERT_EQUAL_UINT32(2, payload_tx_outputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("663e6d9dc9955691ede73e1a81fef87af7b94f167524b5e6f92aa559b89185db",
                           payload_tx_outputs_address(tx, 0), API_ADDR_HEX_STR_LEN);
  TEST_ASSERT(1000000 == payload_tx_outputs_amount(tx, 0));
  TEST_ASSERT_EQUAL_MEMORY("96f9de0989e77d0e150e850a5a600e83045fa57419eaf3b20225b763d4e23813",
                           payload_tx_outputs_address(tx, 1), API_ADDR_HEX_STR_LEN);
  TEST_ASSERT(1200045 == payload_tx_outputs_amount(tx, 1));

  TEST_ASSERT_NULL(tx->payload);

  // validate unlocked block
  TEST_ASSERT_EQUAL_UINT32(2, payload_tx_blocks_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("2baaf3bca8ace9f862e60184bd3e79df25ff230f7eaaa4c7f03daa9833ba854a",
                           payload_tx_blocks_public_key(tx, 0), API_PUB_KEY_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY(
      "cb4ece3f2d7e4903b17d45d41c26685fae9ed04e61294c94095ba248e4eae8cbed60addbd57cabd2df633f0c3f51644fa141a612df81c1f1"
      "8942e20bbaf4d102",
      payload_tx_blocks_signature(tx, 0), API_SIGNATURE_HEX_STR_LEN);

  TEST_ASSERT(payload_tx_blocks_reference(tx, 0) == UINT16_MAX);
  TEST_ASSERT_NULL(payload_tx_blocks_public_key(tx, 1));
  TEST_ASSERT_NULL(payload_tx_blocks_signature(tx, 1));

  TEST_ASSERT(payload_tx_blocks_reference(tx, 1) == 0);
  res_message_free(res);
}

void test_deser_tx_with_index(void)
{
  // case 3: tx payload with 1 input, 1 output, 1 signature, an indexation payload
  char const* const tx_with_index =
      "{\"data\":{\"networkId\":\"14379272398717627559\",\"parentMessageIds\":["
      "\"1b9b8b53fc65d85339f49c50a1b262a74304c1f9a40bd2fbf7b72e9d612fb154\","
      "\"33afb20dcc142717c2546d2a419261c5ac7d0f670c430122aa2508983f3bb7f9\","
      "\"52dc9d1059947ba472677acf74fb73390a908c56cc2fc7dd068c34829b3c9571\","
      "\"df61b0c351cb2cc6ba7ccb8b33562ec30fe49b60634ada9d40a61c60aa552a8f\"],\"payload\":{\"type\":0,\"essence\":{"
      "\"type\":0,\"inputs\":[{\"type\":0,\"transactionId\":"
      "\"cef5bb4c32788a48620eb6e7cf351eac6ad78d4ebec834bd87f64f339d207175\",\"transactionOutputIndex\":0}],\"outputs\":"
      "[{\"type\":0,\"address\":{\"type\":0,\"address\":"
      "\"de909573713212274463c792d61919ac02284497c4e2068ea273053ad087f1f6\"},\"amount\":1000000}],\"payload\":{"
      "\"type\":2,\"index\":\"45535033322057616c6c6574\",\"data\":"
      "\"73656e742066726f6d2065737033322076696120696f74612e6300\"}},\"unlockBlocks\":[{\"type\":0,\"signature\":{"
      "\"type\":0,\"publicKey\":\"87e9de7d4f65033503083b0e0ae9c6523f1e91d9481288aad5d090da289a3491\",\"signature\":"
      "\"0add947e74e3efe583b4f3e7ca01e85c4c242f5444c22bf32f0df764433fc2dfc665b9c76ea3fdcd787fb919084cc809bfbd85234795b7"
      "adb5b0240e0170b206\"}}]},\"nonce\":\"4611686018427716421\"}}";

  res_message_t* res = res_message_new();
  TEST_ASSERT_NOT_NULL(res);
  TEST_ASSERT(deser_get_message(tx_with_index, res) == 0);
  TEST_ASSERT(res->is_error == false);

  message_t* msg = res->u.msg;
  TEST_ASSERT_EQUAL_STRING("14379272398717627559", msg->net_id);
  TEST_ASSERT_EQUAL_STRING("4611686018427716421", msg->nonce);
  TEST_ASSERT_EQUAL_INT(4, api_message_parent_count(msg));
  TEST_ASSERT_EQUAL_MEMORY("1b9b8b53fc65d85339f49c50a1b262a74304c1f9a40bd2fbf7b72e9d612fb154",
                           api_message_parent_id(msg, 0), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("33afb20dcc142717c2546d2a419261c5ac7d0f670c430122aa2508983f3bb7f9",
                           api_message_parent_id(msg, 1), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("52dc9d1059947ba472677acf74fb73390a908c56cc2fc7dd068c34829b3c9571",
                           api_message_parent_id(msg, 2), API_MSG_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY("df61b0c351cb2cc6ba7ccb8b33562ec30fe49b60634ada9d40a61c60aa552a8f",
                           api_message_parent_id(msg, 3), API_MSG_ID_HEX_STR_LEN);
  // check payload type
  TEST_ASSERT(get_message_payload_type(res) == MSG_PAYLOAD_TRANSACTION);

  payload_tx_t* tx = (payload_tx_t*)msg->payload;
  // validate input transaction ID and transaction output index
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_inputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("cef5bb4c32788a48620eb6e7cf351eac6ad78d4ebec834bd87f64f339d207175",
                           payload_tx_inputs_tx_id(tx, 0), API_TX_ID_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_UINT32(0, payload_tx_inputs_tx_output_index(tx, 0));

  // validate output address and amount
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_outputs_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("de909573713212274463c792d61919ac02284497c4e2068ea273053ad087f1f6",
                           payload_tx_outputs_address(tx, 0), API_ADDR_HEX_STR_LEN);
  TEST_ASSERT(1000000 == payload_tx_outputs_amount(tx, 0));

  // transaction with indexation pyaload
  TEST_ASSERT_NOT_NULL(tx->payload);
  TEST_ASSERT(tx->type == MSG_PAYLOAD_INDEXATION);
  indexation_t* idx = (indexation_t*)tx->payload;
  TEST_ASSERT_EQUAL_MEMORY("45535033322057616c6c6574", idx->index->data, 25);
  TEST_ASSERT_EQUAL_MEMORY("73656e742066726f6d2065737033322076696120696f74612e6300", idx->data->data, 55);

  // validate unlocked block
  TEST_ASSERT_EQUAL_UINT32(1, payload_tx_blocks_count(tx));
  TEST_ASSERT_EQUAL_MEMORY("87e9de7d4f65033503083b0e0ae9c6523f1e91d9481288aad5d090da289a3491",
                           payload_tx_blocks_public_key(tx, 0), API_PUB_KEY_HEX_STR_LEN);
  TEST_ASSERT_EQUAL_MEMORY(
      "0add947e74e3efe583b4f3e7ca01e85c4c242f5444c22bf32f0df764433fc2dfc665b9c76ea3fdcd787fb919084cc809bfbd85234795b7ad"
      "b5b0240e0170b206",
      payload_tx_blocks_signature(tx, 0), API_SIGNATURE_HEX_STR_LEN);

  res_message_free(res);
}

/* Exported functions ------------------------------------------------------- */
int test_get_message(void)
{
  UNITY_BEGIN();

  RUN_TEST(test_deser_indexation);
  RUN_TEST(test_deser_milestone);
  RUN_TEST(test_deser_tx1);
  RUN_TEST(test_deser_tx2);
  RUN_TEST(test_deser_tx3);
  RUN_TEST(test_deser_tx_with_index);
#if TEST_TANGLE_ENABLE
  RUN_TEST(test_get_msg_by_id);
#endif
  return UNITY_END();
}
