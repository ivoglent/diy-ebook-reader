//
// Created by long.nguyenviet on 8/24/25.
//

#pragma once
#include <esp_err.h>
#include <esp_vfs_fat.h>

#include "service.h"
#include "components/system/Registry.h"

#define MOUNT_POINT "/sdcard"
#define SD_CARD_SPI_HOST SPI3_HOST

class SDCard: public BaseService<SDCard, SERVICE_SD_CARD> {
private:
  gpio_num_t _miso, _mosi, _clk, _cs;
public:
  SDCard(Registry& registry, const gpio_num_t& mosi, const gpio_num_t& miso, const gpio_num_t& clk, const gpio_num_t& cs);
  ~SDCard() = default;
  void setup();
  static esp_err_t readBitmapImage(const char* path, uint8_t* buffer, size_t width, size_t height);
};
