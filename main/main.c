#include "esp_log.h"

#include "wifi_manager.h"
#include "tcp_server.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Gateway started");

    wifi_manager_start_apsta(
        "gateway",
        "12345678",
        "Lamitek",
        "123456890"
    );

    tcp_server_start(5000);

    ESP_LOGI(TAG, "TCP server started on port 5000");

    /*
        Sau này khi thêm WebSocket:
        wifi_manager_wait_sta_connected();
        websocket_manager_start();
    */
}