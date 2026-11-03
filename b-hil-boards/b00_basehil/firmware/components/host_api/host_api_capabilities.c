#include "host_api_internal.h"

#include <stddef.h>

/* Exact catalog from HOST_API_CAPABILITIES_ECHO_LOOPBACK (field order may vary). */
static const char *const k_capabilities_json =
    "{"
    "\"api_version\":1,"
    "\"board_id\":\"b00_basehil\","
    "\"summary\":\"Family base HIL bench with host API demos (echo, loopback)\","
    "\"commands\":["
    "{"
    "\"name\":\"ping\","
    "\"summary\":\"Liveness check\","
    "\"args\":[],"
    "\"result\":[{\"name\":\"pong\",\"type\":\"bool\"}]"
    "},"
    "{"
    "\"name\":\"info\","
    "\"summary\":\"Identity and STA network snapshot\","
    "\"args\":[],"
    "\"result\":["
    "{\"name\":\"api_version\",\"type\":\"int\"},"
    "{\"name\":\"board_id\",\"type\":\"string\"},"
    "{\"name\":\"board_number\",\"type\":\"string\"},"
    "{\"name\":\"identity\",\"type\":\"string\"},"
    "{\"name\":\"sta_ipv4\",\"type\":\"string\"},"
    "{\"name\":\"streams_supported\",\"type\":\"bool\"}"
    "]"
    "},"
    "{"
    "\"name\":\"capabilities\","
    "\"summary\":\"Return this capability manifest\","
    "\"args\":[],"
    "\"result\":["
    "{\"name\":\"api_version\",\"type\":\"int\"},"
    "{\"name\":\"board_id\",\"type\":\"string\"},"
    "{\"name\":\"summary\",\"type\":\"string\"},"
    "{\"name\":\"commands\",\"type\":\"object\"},"
    "{\"name\":\"streams\",\"type\":\"object\"}"
    "]"
    "},"
    "{"
    "\"name\":\"echo\","
    "\"summary\":\"Round-trip a short text string\","
    "\"args\":["
    "{\"name\":\"text\",\"type\":\"string\",\"required\":true,\"notes\":\"max 128 bytes\"}"
    "],"
    "\"result\":[{\"name\":\"text\",\"type\":\"string\"}]"
    "}"
    "],"
    "\"streams\":["
    "{"
    "\"stream_id\":\"loopback\","
    "\"summary\":\"Echo written frames back on read (no DUT UART)\","
    "\"params\":[],"
    "\"payload\":\"opaque bytes; write enqueues identical payload for read; max 1024 per frame\""
    "}"
    "]"
    "}";

cJSON *host_api_capabilities_build_result(void)
{
    cJSON *root = cJSON_Parse(k_capabilities_json);
    if (root == NULL || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}
