//
// Created by long.nguyenviet on 8/29/25.
//

#pragma once
#include "esp_console.h"

inline static std::vector<esp_console_cmd_t> COMMANDS;
inline void register_console_command(const esp_console_cmd_t& cmd)
{
    ESP_LOGI("CONSOLE", "Register command: %s", cmd.command);
    COMMANDS.push_back(cmd);
}