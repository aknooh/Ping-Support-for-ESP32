/* Console example — WiFi commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "cmd_decl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "esp_event.h"
#include "cmd_wifi.h"
#include "lwip/inet.h"

#define DEFAULT_COUNT 5
#define DEFAULT_TIMEOUT 1000    // unit = ms
#define DEFAULT_DELAY 500       // unit = ms
#define FAILURE -1

#define JOIN_TIMEOUT_MS (10000)

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

/** Arguments used by 'join' function */
static struct {
    struct arg_int *timeout;
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

/** Arguments used by 'send_icmp' function */
static struct {
    struct arg_int *ipAddress;
    struct arg_int *count;
    struct arg_int *timeout;
    struct arg_int *delay;
    struct arg_end *end;
} ping_args;


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

static void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_WARN);
    static bool initialized = false;
    if (initialized) {
        return;
    }
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    initialized = true;
}

static bool wifi_join(const char *ssid, const char *pass, int timeout_ms)
{
    initialise_wifi();
    wifi_config_t wifi_config = { 0 };
    strlcpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass) {
        strlcpy((char *) wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_connect() );

    int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                   pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
    return (bits & CONNECTED_BIT) != 0;
}

static void parse_args(ip4_addr_t *ip4, ip6_addr_t *ip6, uint32_t *count, uint32_t *timeout, uint32_t *delay)
{
    if (ping_args.ipAddress->count > 0)
    {
        // Prepare the IP address into correct struct
        // target_ip.addr = *ping_args.ipAddress->ival;
        printf("IP address enterd!\n");
    }

    if (ping_args.count->count > 0)
    {
        *count= *ping_args.count->ival;
    }

    if (ping_args.timeout->count > 0)
    {
        *timeout = *ping_args.timeout->ival;
    }
    if (ping_args.delay->count > 0)
    {
        *delay = *ping_args.delay->ival;
    }
}

static int send_icmp(int argc, char **argv)
{
    ip4_addr_t target_ipv4;
    // initialize optional arguments to default values
    uint32_t ping_count     = DEFAULT_COUNT;
    uint32_t ping_timeout   = DEFAULT_TIMEOUT;
    uint32_t ping_delay     = DEFAULT_DELAY;
    // TO DO: IPv6
    ip6_addr_t target_ipv6;

    int nerrors = arg_parse(argc, argv, (void **) &ping_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ping_args.end, argv[0]);
        return FAILURE;
    }

    parse_args(&target_ipv4, &target_ipv6, &ping_count, &ping_timeout, &ping_delay);

    // For compiler to stop complaining about 'UNSED Variables'
    printf("All params: target_ip4(%d), target_ipv6(%d), count(%d), timeout(%d), delay(%d)\n",
            target_ipv4.addr,target_ipv6.addr[0], ping_count, ping_timeout, ping_delay);

    return 0;
}

static int connect(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    ESP_LOGI(__func__, "Connecting to '%s'",
             join_args.ssid->sval[0]);

    /* set default value*/
    if (join_args.timeout->count == 0) {
        join_args.timeout->ival[0] = JOIN_TIMEOUT_MS;
    }

    bool connected = wifi_join(join_args.ssid->sval[0],
                               join_args.password->sval[0],
                               join_args.timeout->ival[0]);
    if (!connected) {
        ESP_LOGW(__func__, "Connection timed out");
        return 1;
    }
    ESP_LOGI(__func__, "Connected");
    return 0;
}

void register_wifi(void)
{
    join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    join_args.end = arg_end(2);

    const esp_console_cmd_t join_cmd = {
        .command = "join",
        .help = "Join WiFi AP as a station",
        .hint = NULL,
        .func = &connect,
        .argtable = &join_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );
}

void register_ping(void)
{
    ping_args.ipAddress = arg_intn(NULL, "ip", "<IPv4/IPv6>", 0, INT_MAX, "Target IP Address");
    ping_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    ping_args.count = arg_int0(NULL, "count", "<n>", "Number of messages");
    ping_args.delay= arg_int0(NULL, "delay", "<t>", "Delay between messges, ms");
    ping_args.end = arg_end(1);
    const esp_console_cmd_t ping_cmd = {
        .command = "ping",
        .help = "Send an ICMP message to an IPv4/IPv6 address",
        .hint = "<IP address>",
        .func = &send_icmp,
        .argtable = &ping_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&ping_cmd) );
}
