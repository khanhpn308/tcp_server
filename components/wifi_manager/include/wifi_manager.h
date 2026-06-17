#pragma once

#include <stdbool.h>

void wifi_manager_start_apsta(const char *ap_ssid,
                              const char *ap_password,
                              const char *sta_ssid,
                              const char *sta_password);

bool wifi_manager_is_sta_connected(void);
void wifi_manager_wait_sta_connected(void);