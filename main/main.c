#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
//#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "inttypes.h"
#include "driver/gpio.h"

#define SPP_SERVER_NAME "ESP32_SPP_SERVER" //server name
#define SPP_SERVER_UUID 0x1101
#define LED_PIN GPIO_NUM_2 // GPIO number for the LED pin

static const char *TAG = "BLUETOOTH_SPP"; // teg for logs
static uint32_t spp_handle = 0; // handle for spp

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT: //event for initialization
        ESP_LOGI(TAG, "ESP_SPP_INIT_EVT");
        esp_bt_gap_set_device_name(SPP_SERVER_NAME); // set server name
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE); // set scan mode
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, SPP_SERVER_NAME); // start servera
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT: 
        ESP_LOGI(TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_SPP_OPEN_EVT"); // event for connection
        spp_handle = param->open.handle;
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_SPP_CLOSE_EVT"); // event for disconnection
        spp_handle = 0;
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(TAG, "ESP_SPP_START_EVT"); // event for start
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(TAG, "ESP_SPP_CL_INIT_EVT"); // event for client initialization
        break;

        // event for receiving data from the client, in this case it will be received in hexadecimal format and toggle the LED 
    case ESP_SPP_DATA_IND_EVT: 
         ESP_LOGI(TAG, "ESP_SPP_DATA_IND_EVT len=%"PRIu32" handle=%"PRIu32, (uint32_t)param->data_ind.len, (uint32_t)param->data_ind.handle);
            // enter the data in hexadecimal format
        esp_log_buffer_hex("", param->data_ind.data, param->data_ind.len);
        
        char received_data[5] = {0};  // buffer for received data
    strncpy(received_data, (char*)param->data_ind.data, sizeof(received_data) - 1); // copy received data to buffer

        if (strncmp(received_data, "ON", 2) == 0) // check if received data is "ON"
        {
            gpio_set_level(LED_PIN, 1); // turn on the LED
            ESP_LOGI(TAG, "Setting LED to ON");
        }
        else if (strncmp(received_data, "OFF", 3) == 0) // check if received data is "OFF"
        {
            gpio_set_level(LED_PIN, 0); // turn off the LED
            ESP_LOGI(TAG, "Setting LED to OFF");
        }

        esp_spp_write(spp_handle, param->data_ind.len, param->data_ind.data); // send data back to the client
        break;
    case ESP_SPP_CONG_EVT: // event for congestion
        ESP_LOGI(TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT: // event for sending data
        ESP_LOGI(TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_SPP_SRV_OPEN_EVT"); // event for spp server open
        break;
    default:
        break;
    }
}

esp_spp_cfg_t spp_cfg = { // set config for spp
    .mode = ESP_SPP_MODE_CB,
    
};

void app_main(void)
{
    esp_err_t ret;

    // init NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { // erase and init NVS
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret); 

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE)); // release memory

    // set GPIO for LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT); // set LED as output

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT(); // set config for bluetooth controller
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) { 
        ESP_LOGE(TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret)); // initialize bluetooth controller
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) { 
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret)); // enable bluetooth controller
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret)); // initialize bluedroid
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret)); // chack if bluedroid is enabled
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret)); 
        return;
    }

    if ((ret = esp_spp_enhanced_init(&spp_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret)); // check if spp is initialized
        return;
    }

    ESP_LOGI(TAG, "Bluetooth SPP server initialized"); // log that spp is initialized
    

}
