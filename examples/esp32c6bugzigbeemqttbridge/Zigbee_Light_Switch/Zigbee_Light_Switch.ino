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
 * @brief This example demonstrates simple Zigbee light switch.
 * 
 * The example demonstrates how to use ESP Zigbee stack to control a light bulb.
 * The light bulb is a Zigbee end device, which is controlled by a Zigbee coordinator.
 * Button switch and Zigbee runs in separate tasks.
 * 
 * Proper Zigbee mode must be selected in Tools->Zigbee mode 
 * and also the correct partition scheme must be selected in Tools->Partition Scheme.
 * 
 * Please check the README.md for instructions and more detailed description.
 */

#ifndef ZIGBEE_MODE_ZCZR
#error "Zigbee coordinator mode is not selected in Tools->Zigbee mode"
#endif
#include "Zigbee.h"

static void esp_zb_task(void *pvParameters)
{
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZC_CONFIG();
    esp_zb_init(&zb_nwk_cfg);
    esp_zb_on_off_switch_cfg_t switch_cfg = ESP_ZB_DEFAULT_ON_OFF_SWITCH_CONFIG();
    esp_zb_ep_list_t *esp_zb_on_off_switch_ep = esp_zb_on_off_switch_ep_create(HA_ONOFF_SWITCH_ENDPOINT, &switch_cfg);
    esp_zb_device_register(esp_zb_on_off_switch_ep);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_main_loop_iteration();
}

/********************* Arduino functions **************************/
void setup() {
    Serial.begin(115200);
    // Init Zigbee
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };

    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    // Init button switch 
    for (int i = 0; i < PAIR_SIZE(button_func_pair); i++) {
        pinMode(button_func_pair[i].pin, INPUT_PULLUP);
        /* create a queue to handle gpio event from isr */
        gpio_evt_queue = xQueueCreate(10, sizeof(switch_func_pair_t));
        if ( gpio_evt_queue == 0) {
            log_e("Queue was not created and must not be used");
            while(1);
        }
        attachInterruptArg(button_func_pair[i].pin, gpio_isr_handler, (void *) (button_func_pair + i), FALLING);
    }

    // Start Zigbee task
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}

void loop() {
    // Handle button switch in loop()
    uint8_t pin = 0;
    switch_func_pair_t button_func_pair;
    static switch_state_t switch_state = SWITCH_IDLE;
    bool evt_flag = false;

    /* check if there is any queue received, if yes read out the button_func_pair */
    if (xQueueReceive(gpio_evt_queue, &button_func_pair, portMAX_DELAY)) {
        pin =  button_func_pair.pin;
        switch_gpios_intr_enabled(false);
        evt_flag = true;
    }
    while (evt_flag) {
        bool value = digitalRead(pin);
        switch (switch_state) {
        case SWITCH_IDLE:
            switch_state = (value == LOW) ? SWITCH_PRESS_DETECTED : SWITCH_IDLE;
            break;
        case SWITCH_PRESS_DETECTED:
            switch_state = (value == LOW) ? SWITCH_PRESS_DETECTED : SWITCH_RELEASE_DETECTED;
            break;
        case SWITCH_RELEASE_DETECTED:
            switch_state = SWITCH_IDLE;
            /* callback to button_handler */
            (*esp_zb_buttons_handler)(&button_func_pair);
            break;
        default:
            break;
        }
        if (switch_state == SWITCH_IDLE) {
            switch_gpios_intr_enabled(true);
            evt_flag = false;
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
