#pragma once

// Connects to config::WIFI_SSID/WIFI_PASS and keeps reconnecting on drop.
namespace wifi_manager {

void init();

}  // namespace wifi_manager
