#include "wifi_manager.h"

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "wifi_manager";

static bool s_sta_connected = false;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA started, connecting to router...");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_sta_connected = false;
        ESP_LOGW(TAG, "STA disconnected from router, reconnecting...");
        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

        s_sta_connected = true;

        ESP_LOGI(TAG, "STA got IP from router: " IPSTR,
                 IP2STR(&event->ip_info.ip));
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event =
            (wifi_event_ap_staconnected_t *) event_data;

        ESP_LOGI(TAG, "Node connected to Gateway SoftAP, AID=%d", event->aid);
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event =
            (wifi_event_ap_stadisconnected_t *) event_data;

        ESP_LOGW(TAG, "Node disconnected from Gateway SoftAP, AID=%d", event->aid);
    }
}

static void wifi_manager_init_base(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }
}

void wifi_manager_start_apsta(const char *ap_ssid,
                              const char *ap_password,
                              const char *sta_ssid,
                              const char *sta_password)
{
    wifi_manager_init_base();

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t ap_config = {0};

    strncpy((char *)ap_config.ap.ssid,
            ap_ssid,
            sizeof(ap_config.ap.ssid) - 1);

    strncpy((char *)ap_config.ap.password,
            ap_password,
            sizeof(ap_config.ap.password) - 1);

    ap_config.ap.ssid_len = strlen(ap_ssid);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(ap_password) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config_t sta_config = {0};

    strncpy((char *)sta_config.sta.ssid,
            sta_ssid,
            sizeof(sta_config.sta.ssid) - 1);

    strncpy((char *)sta_config.sta.password,
            sta_password,
            sizeof(sta_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Gateway APSTA started");
    ESP_LOGI(TAG, "SoftAP SSID: %s", ap_ssid);
    ESP_LOGI(TAG, "Node should connect to Gateway IP: 192.168.4.1");
    ESP_LOGI(TAG, "Gateway STA is connecting to router SSID: %s", sta_ssid);
}

bool wifi_manager_is_sta_connected(void)
{
    return s_sta_connected;
}

void wifi_manager_wait_sta_connected(void)
{
    while (!s_sta_connected) {
        ESP_LOGI(TAG, "Waiting for Gateway STA internet connection...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}