//
// Created by long.nguyenviet on 8/24/25.
//

#include "SDCard.h"

#include <cstring>
#include <sdmmc_cmd.h>
#include <sys/dirent.h>
#include <sys/stat.h>

#include "components/display/Display.h"

static char *SDCARD_SD_TAG= "SDCard";

SDCard::SDCard(Registry& registry, const gpio_num_t& mosi, const gpio_num_t& miso, const gpio_num_t& clk, const gpio_num_t& cs): BaseService(registry), _miso(miso), _mosi(mosi), _clk(clk), _cs(cs)
{
}

void SDCard::setup()
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 50,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    constexpr char mount_point[] = MOUNT_POINT;
    ESP_LOGI(SDCARD_SD_TAG, "Initializing SD card");
    ESP_LOGI(SDCARD_SD_TAG, "Using SPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_CARD_SPI_HOST;
    const auto host_id = static_cast<spi_host_device_t>(host.slot);


#if CONFIG_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(SDCARD_SD_TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = _mosi,
        .miso_io_num = _miso,
        .sclk_io_num = _clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host_id, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(SDCARD_SD_TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = _cs;
    slot_config.host_id = host_id;

    ESP_LOGI(SDCARD_SD_TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SDCARD_SD_TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SDCARD_SD_TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return;
    }
    ESP_LOGI(SDCARD_SD_TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);
}

esp_err_t SDCard::readBitmapImage(const char* path, uint8_t* buffer, size_t width, size_t height)
{
    ESP_LOGI(SDCARD_SD_TAG, "Reading file %s", path);
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(SDCARD_SD_TAG, "Failed to open BMP: %s", path);
        return ESP_FAIL;
    }

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    fread(&file_header, sizeof(file_header), 1, f);
    fread(&info_header, sizeof(info_header), 1, f);

    if (file_header.bfType != 0x4D42 || info_header.biBitCount != 1) {
        ESP_LOGE(SDCARD_SD_TAG, "Not a valid 1-bit BMP");
        fclose(f);
        return ESP_FAIL;
    }

    int w = info_header.biWidth;
    int h = info_header.biHeight;
    int row_size = ((w + 31) / 32) * 4;   // rows padded to 4 bytes

    fseek(f, file_header.bfOffBits, SEEK_SET);

    auto *row = static_cast<uint8_t *>(malloc(row_size));

    // read bottom→top
    for (int y = 0; y < h; y++) {
        fread(row, 1, row_size, f);
        for (int x = 0; x < w; x++) {
            int byte_idx = x / 8;
            int bit_idx  = 7 - (x % 8);
            int pixel    = (row[byte_idx] >> bit_idx) & 1;
            int draw_x = x;
            int draw_y = h - 1 - y;
            if (draw_x < width && draw_y < height) {
                buffer[draw_y * width + draw_x] = pixel ? 0 : 1;
            }
        }
    }
    free(row);
    fclose(f);
    return ESP_OK;
}

std::vector<std::string> list_directories(const char* path)
{
    std::vector<std::string> dirs;
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Failed to open dir: %s\n", path);
        return dirs;
    }

    dirent *entry;
    struct stat st{};

    while ((entry = readdir(dir)) != NULL) {
        char fullpath[512];
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("Directory: %s\n", entry->d_name);
            dirs.emplace_back(entry->d_name);
        }
    }

    closedir(dir);
    return dirs;
}


char *read_string_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == nullptr) {
        ESP_LOGE(SD_TAG, "Failed to open file: %s", path);
        return nullptr;
    }

    // seek to end to get size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = (char *)malloc(size + 1);
    if (buffer == nullptr) {
        ESP_LOGE(SD_TAG, "Failed to malloc %ld bytes", size + 1);
        fclose(f);
        return nullptr;
    }

    size_t read_size = fread(buffer, 1, size, f);
    buffer[read_size] = '\0';  // null terminate

    fclose(f);

    ESP_LOGI(SD_TAG, "Read file %s (%ld bytes)", path, size);
    return buffer;
}