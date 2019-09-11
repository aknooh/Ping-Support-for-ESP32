/* Console example — WiFi commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
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
#include <netinet/in.h>
#include "esp_ping.h"
#include "ping/ping.h"

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
    struct arg_str *ip_address;
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

static bool parse_ip_address(const char *address, ip4_addr_t *ipv4, ip6_addr_t *ipv6)
{
    // Determine if the address is IPv4 or IPv6
    char *indicator = address;
    while(*indicator++)
    {
        if (*indicator == '.') {
			return inet_aton(address, ipv4);
        }
        else if (*indicator == ':') {
            return inet6_aton(address, &ipv6);
        }
    }
    return false;
}

static esp_err_t parse_args(ip4_addr_t *ip4, ip6_addr_t *ip6, uint32_t *count, uint32_t *timeout, uint32_t *delay)
{
    if (ping_args.ip_address->count > 0)
    {
        // Parse IP address (Handle v4 and v6)
        bool rc = parse_ip_address(ping_args.ip_address->sval[0], ip4, ip6);
		if (!rc) {
			fprintf(stderr, "Error parsing provided IP address...aborting!\n");
			return ESP_FAIL;
		}
    }

    if (ping_args.count->count > 0)
        *count= *ping_args.count->ival;

    if (ping_args.timeout->count > 0)
        *timeout = *ping_args.timeout->ival;

    if (ping_args.delay->count > 0)
		*delay = *ping_args.delay->ival;

	return ESP_OK;
}

esp_err_t ping_results(ping_target_id_t message_type, esp_ping_found *found_val)
{
    static int ctr;
    // Print everything for now
	/*
    printf("PING RESULTS\n");
    printf("Message Type    : %d\n", message_type);
    printf("resp_time       : %d\n", found_val->resp_time);
    printf("timeout_count   : %d\n", found_val->timeout_count);
    printf("send_count      : %d\n", found_val->send_count);
    printf("recv_count      : %d\n", found_val->recv_count);
    printf("err_count       : %d\n", found_val->err_count);
    printf("bytes           : %d\n", found_val->bytes);
    printf("total_bytes     : %d\n", found_val->total_bytes);
    printf("total_time      : %d\n", found_val->total_time);
    printf("min_time        : %d\n", found_val->min_time);
    printf("max_time        : %d\n", found_val->max_time);
    printf("ping_err        : %d\n\n", found_val->ping_err);
	*/
    if (ctr++ >= *ping_args.count->ival)
        ping_deinit();

    return ESP_OK;
}
static esp_err_t send_icmp(int argc, char **argv)
{
	esp_err_t ret;
    int counter;
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

    ret = parse_args(&target_ipv4, &target_ipv6, &ping_count, &ping_timeout, &ping_delay);

	if (ret == ESP_OK) {
		printf("Pinging IP Address \'%s\'\n", inet_ntoa(target_ipv4));
		for (counter = 0; counter < ping_count; counter++)
		{
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &ping_count, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RCV_TIMEO, &ping_timeout, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_DELAY_TIME, &ping_delay, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_IP_ADDRESS, &target_ipv4.addr, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RES_FN, &ping_results, sizeof(ping_results));
			ping_init();
		}
	} else
		return ESP_FAIL;
    return ESP_OK;
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
    ping_args.ip_address = arg_str0(NULL, NULL, "<IPv4/IPv6>", "Target IP Address");
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
