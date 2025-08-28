//
// Created by long.nguyenviet on 8/28/25.
//

#pragma once
#include <cJSON.h>
#include <sys/types.h>
#include "esp_log.h"

/**
* {
  "title": "10 nghịch lý cuộc sống",
  "creator": "Kent M. Keith",
  "identifier": "8ef450a7-a567-41a5-b7be-dd032d484144",
  "cover": "cover.bmp"
}
 */
typedef struct {
    std::string title;
    std::string creator;
    std::string identifier;
    std::string cover;
    uint index = 1;
} bookinfo_t;

inline void parse_bookinfo(const char* str, bookinfo_t& book)
{
    cJSON* root = cJSON_Parse(str);
    if (!root) {
        ESP_LOGE("JSON", "JSON parse error");
        return;
    }

    // Extract fields safely
    cJSON* title = cJSON_GetObjectItemCaseSensitive(root, "title");
    if (cJSON_IsString(title) && (title->valuestring != nullptr)) {
        book.title = title->valuestring;
    }

    cJSON* creator = cJSON_GetObjectItemCaseSensitive(root, "creator");
    if (cJSON_IsString(creator) && (creator->valuestring != nullptr)) {
        book.creator = creator->valuestring;
    }

    cJSON* identifier = cJSON_GetObjectItemCaseSensitive(root, "identifier");
    if (cJSON_IsString(identifier) && (identifier->valuestring != nullptr)) {
        book.identifier = identifier->valuestring;
    }

    cJSON* cover = cJSON_GetObjectItemCaseSensitive(root, "cover");
    if (cJSON_IsString(cover) && (cover->valuestring != nullptr)) {
        book.cover = cover->valuestring;
    }

    cJSON* index = cJSON_GetObjectItemCaseSensitive(root, "index");
    if (cJSON_IsNumber(index)) {
        book.index = cover->valueint;
    } else
    {
        book.index = 1;
    }

    cJSON_Delete(root);
}

inline char* bookinfo_to_json(const bookinfo_t &book) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return nullptr;

    cJSON_AddStringToObject(root, "title", book.title.c_str());
    cJSON_AddStringToObject(root, "creator", book.creator.c_str());
    cJSON_AddStringToObject(root, "identifier", book.identifier.c_str());
    cJSON_AddStringToObject(root, "cover", book.cover.c_str());

    // Print to string (heap-allocated, must free later with free())
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_str;
}