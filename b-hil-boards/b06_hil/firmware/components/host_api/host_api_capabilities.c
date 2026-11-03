#include "host_api_internal.h"

#include <stddef.h>

/*
 * Particular board catalog: mandatory ping / info / capabilities plus
 * B06_HOST_API_METERING commands. No echo/loopback on this board.
 */
static const char *const k_capabilities_json =
    "{"
    "\"api_version\":1,"
    "\"board_id\":\"b06_hil\","
    "\"summary\":\"Passive USB 5V 1-to-3 splitter with per-output INA219 metering\","
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
    "\"name\":\"meter_read\","
    "\"summary\":\"Read bus voltage, current, and power for one silk OUT channel\","
    "\"args\":["
    "{"
    "\"name\":\"channel\","
    "\"type\":\"int\","
    "\"required\":true,"
    "\"notes\":\"Silk OUT 1, 2, or 3\""
    "}"
    "],"
    "\"result\":["
    "{\"name\":\"channel\",\"type\":\"int\",\"notes\":\"Echo of requested silk channel\"},"
    "{\"name\":\"ok\",\"type\":\"bool\",\"notes\":\"false if I2C/sample failed\"},"
    "{\"name\":\"voltage_v\",\"type\":\"number\",\"notes\":\"Volts; present iff ok\"},"
    "{\"name\":\"current_a\",\"type\":\"number\",\"notes\":\"Amperes signed; present iff ok\"},"
    "{\"name\":\"power_w\",\"type\":\"number\",\"notes\":\"Watts = V*I; present iff ok\"}"
    "]"
    "},"
    "{"
    "\"name\":\"meter_read_all\","
    "\"summary\":\"Read voltage, current, and power for silk OUT 1..3\","
    "\"args\":[],"
    "\"result\":["
    "{"
    "\"name\":\"channels\","
    "\"type\":\"object\","
    "\"notes\":\"Array of three channel objects in order OUT1..OUT3\""
    "}"
    "]"
    "}"
    "],"
    "\"streams\":[]"
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
