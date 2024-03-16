// Copyright 2023 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief This example demonstrates simple Zigbee light bulb.
 *
 * The example demonstrates how to use ESP Zigbee stack to create a end device light bulb.
 * The light bulb is a Zigbee end device, which is controlled by a Zigbee coordinator.
 *
 * Proper Zigbee mode must be selected in Tools->Zigbee mode
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 *
 * Please check the README.md for instructions and more detailed description.
 */

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "esp_zigbee_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"

// #define LIGHT_IS_HIGH
#define LED_PIN 8
bool ledstate = true;

/* Default End Device config */
#define ESP_ZB_ZED_CONFIG()                                     \
    {                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                   \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,       \
        .nwk_cfg = {                                            \
            .zed_cfg = {                                        \
                .ed_timeout = ED_AGING_TIMEOUT,                 \
                .keep_alive = ED_KEEP_ALIVE,                    \
            },                                                  \
        },                                                      \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
    {                                                           \
        .radio_mode = RADIO_MODE_NATIVE,                        \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
    {                                                           \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,      \
    }

/* Zigbee configuration */
#define INSTALLCODE_POLICY_ENABLE       false    /* enable the install code policy for security */
#define ED_AGING_TIMEOUT                ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE                   3000    /* 3000 millisecond */
#define HA_ESP_LIGHT_ENDPOINT           10    /* esp light bulb device endpoint, used to process light controlling commands */
#define ESP_ZB_PRIMARY_CHANNEL_MASK     ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK  /* Zigbee primary channel mask use in the example */

/********************* Zigbee functions **************************/
static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)*p_sg_p;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        Serial.println("Zigbee stack initialized");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            Serial.println("Start network steering");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            /* commissioning failed */
            Serial.print("Failed to initialize Zigbee stack, status:");
            Serial.println(esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            Serial.print("Joined network successfully (Extended PAN ID:");
            Serial.print(extended_pan_id[7],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[6],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[5],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[4],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[3],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[2],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[1],HEX);
            Serial.print(":");
            Serial.print(extended_pan_id[0],HEX);
            Serial.print(", PAN ID");
            Serial.print(esp_zb_get_pan_id(),HEX);
            Serial.print(" Channel:");
            Serial.print(esp_zb_get_current_channel());
            Serial.print(" Short address:");
            Serial.println(esp_zb_get_short_address(),HEX);
        } else {
            Serial.print("Network steering was not successful status:");
            Serial.println(esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    default:
//        log_i("ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
//                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        log_w("Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

static void esp_zb_task(void *pvParameters)
{
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&zb_nwk_cfg);
    esp_zb_on_off_light_cfg_t light_cfg = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
    esp_zb_ep_list_t *esp_zb_on_off_light_ep = esp_zb_on_off_light_ep_create(HA_ESP_LIGHT_ENDPOINT, &light_cfg);
    esp_zb_device_register(esp_zb_on_off_light_ep);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

    //Erase NVRAM before creating connection to new Coordinator
    //esp_zb_nvram_erase_at_start(true); //Comment out this line to erase NVRAM data if you are conneting to new Coordinator

    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_main_loop_iteration();
}

/* Handle the light attribute */

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    bool light_state = 0;

    if(!message){
      Serial.println("Empty message");
    }
    if(message->info.status != ESP_ZB_ZCL_STATUS_SUCCESS){
      Serial.print("Received message: error status: ");
      Serial.println(message->info.status);
    }

    Serial.print("Received message: endpoint 0x");
    Serial.print(message->info.dst_endpoint);
    Serial.print(" cluster:0x");
    Serial.print(message->info.cluster,HEX);
    Serial.print(" attribute:0x");
    Serial.print(message->attribute.id,HEX);
    Serial.print(" data size: ");
    Serial.println(message->attribute.data.size);
    if (message->info.dst_endpoint == HA_ESP_LIGHT_ENDPOINT) {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
#ifndef LIGHT_IS_HIGH
                ledstate = !light_state;
                Serial.print("Light sets to ");
                if(ledstate)Serial.println("Off");
                if(!ledstate)Serial.println("On");
                digitalWrite(LED_PIN,ledstate);
#else
                ledstate = light_state;
                Serial.print("Light sets to ");
                if(ledstate)Serial.println("On");
                if(!ledstate)Serial.println("Off");
                digitalWrite(LED_PIN,ledstate);
#endif
            }
        }
    }
    return ret;
}

/********************* Arduino functions **************************/
void setup() {
    // Init Zigbee
    Serial.begin(115200);
    Serial.println("Esp32-C6-Bug Zigbee bulb begin!");
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    // Init RMT and leave light OFF
    pinMode(LED_PIN ,OUTPUT);
    digitalWrite(LED_PIN,ledstate);

    // Start Zigbee task
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}

void loop() {
    //empty, zigbee running in task
}
