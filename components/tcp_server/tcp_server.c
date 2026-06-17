#include "tcp_server.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

static const char *TAG = "tcp_server";

#define TCP_SERVER_RX_BUFFER_SIZE 512
#define TCP_SERVER_TASK_STACK_SIZE 4096
#define TCP_SERVER_TASK_PRIORITY 5
#define TCP_SERVER_MAX_PENDING_CLIENTS 4

static int s_server_port = 5000;

static void tcp_server_task(void *arg)
{
    char rx_buffer[TCP_SERVER_RX_BUFFER_SIZE];

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (listen_sock < 0) {
        ESP_LOGE(TAG, "socket() failed, errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "socket() success, listen_sock=%d", listen_sock);

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(s_server_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    int err = bind(listen_sock,
                   (struct sockaddr *)&server_addr,
                   sizeof(server_addr));

    if (err != 0) {
        ESP_LOGE(TAG, "bind() failed, errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "bind() success, port=%d", s_server_port);

    err = listen(listen_sock, TCP_SERVER_MAX_PENDING_CLIENTS);

    if (err != 0) {
        ESP_LOGE(TAG, "listen() failed, errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "listen() success. Waiting for client...");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_sock = accept(listen_sock,
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_len);

        if (client_sock < 0) {
            ESP_LOGE(TAG, "accept() failed, errno=%d", errno);
            continue;
        }

        ESP_LOGI(TAG, "Client connected, client_sock=%d", client_sock);

        while (1) {
            int len = recv(client_sock,
                           rx_buffer,
                           sizeof(rx_buffer) - 1,
                           0);

            if (len < 0) {
                ESP_LOGE(TAG, "recv() failed, errno=%d", errno);
                break;
            }

            if (len == 0) {
                ESP_LOGI(TAG, "Client disconnected");
                break;
            }

            rx_buffer[len] = '\0';

            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            send(client_sock, "OK\n", 3, 0);
        }

        close(client_sock);
        ESP_LOGI(TAG, "client_sock closed");
    }

    close(listen_sock);
    vTaskDelete(NULL);
}

void tcp_server_start(int port)
{
    s_server_port = port;

    xTaskCreate(tcp_server_task,
                "tcp_server_task",
                TCP_SERVER_TASK_STACK_SIZE,
                NULL,
                TCP_SERVER_TASK_PRIORITY,
                NULL);
}