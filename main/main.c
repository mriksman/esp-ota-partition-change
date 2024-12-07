#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_partition.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "esp_system.h"

static const char *TAG = "main";

extern const char partition_start[] asm("_binary_partition_table_bin_start");
extern const char partition_end[]   asm("_binary_partition_table_bin_end");

// Get the string name of type enum values used in this example
static const char* get_type_str(esp_partition_type_t type)
{
    switch(type) {
        case ESP_PARTITION_TYPE_APP:
            return "app";
        case ESP_PARTITION_TYPE_DATA:
            return "data";
        default:
            return "unknown"; // type not used in this example
    }
}
// Get the string name of subtype enum values used in this example
static const char* get_subtype_str(esp_partition_subtype_t subtype)
{
    switch(subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_NVS:
            return "ESP_PARTITION_SUBTYPE_DATA_NVS";
        case ESP_PARTITION_SUBTYPE_APP_OTA_0:
            return "ESP_PARTITION_SUBTYPE_APP_OTA_0";
        case ESP_PARTITION_SUBTYPE_APP_OTA_1:
            return "ESP_PARTITION_SUBTYPE_APP_OTA_1";
        case ESP_PARTITION_SUBTYPE_DATA_OTA:
            return "ESP_PARTITION_SUBTYPE_DATA_OTA";
        case ESP_PARTITION_SUBTYPE_DATA_HOMEKIT:
            return "ESP_PARTITION_SUBTYPE_DATA_HOMEKIT";
        default:
            return "UNKNOWN_PARTITION_SUBTYPE"; 
    }
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_DEBUG);         

    esp_partition_iterator_t it;

    ESP_LOGI(TAG, "Iterating through partitions...");
    it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);

    // Loop through all matching partitions, in this case, all with the type 'data' until partition with desired
    // label is found. Verify if its the same instance as the one found before.
    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "\tfound partition with type %s, subtype %s, label '%s' at offset 0x%" PRIx32 " with size %" PRIu32 "KB",
            get_type_str(part->type), get_subtype_str(part->subtype), part->label, part->address, part->size / 1024);
    }

    // Release the partition iterator to release memory allocated for it
    esp_partition_iterator_release(it);

    
    
    const esp_partition_t *homekit_partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "homekit");

    if (homekit_partition) {
        // check size
        if (homekit_partition->size != 0x1000) {  // 0x1000 = 4KB
            ESP_LOGW(TAG, "Partition size %" PRIu32 "KB does not equal 4KB, abort", homekit_partition->size / 1024);
            return;
        }
        if (homekit_partition->type == ESP_PARTITION_TYPE_DATA && 
                 homekit_partition->subtype == ESP_PARTITION_SUBTYPE_DATA_HOMEKIT) {
            ESP_LOGW(TAG, "Partition type and subtype already correct, abort");
            return;
        }
    
        vTaskDelay(pdMS_TO_TICKS(1000));  //enough delay so that you can flash and enter monitor mode without it having already completed

        uint32_t partition_table_size = partition_end - partition_start;
        const void *partition_table = partition_start;

        ESP_LOGI(TAG, "Erasing partition table region...");
        esp_err_t err = esp_flash_erase_region(NULL, CONFIG_PARTITION_TABLE_OFFSET, 0x1000);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase partition table region: %s", esp_err_to_name(err));
            return;
        }

        ESP_LOGI(TAG, "Writing partition table (%" PRIu32 " bytes) to 0x%x...", partition_table_size, CONFIG_PARTITION_TABLE_OFFSET);
        err = esp_flash_write(NULL, partition_table, CONFIG_PARTITION_TABLE_OFFSET, partition_table_size);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Partition table written successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to write partition table: %s", esp_err_to_name(err));
        }
    
    }
    else {
        ESP_LOGW(TAG, "No partition named 'homekit' found, abort");
    }



}
