#include "esp_zigbee_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#ifndef ZIGBEE_MODE_ZCZR
#error "Zigbee coordinator mode is not selected in Tools->Zigbee mode"
#endif

/* Switch configuration */
#define GPIO_INPUT_IO_TOGGLE_SWITCH  GPIO_NUM_9
#define PAIR_SIZE(TYPE_STR_PAIR) (sizeof(TYPE_STR_PAIR) / sizeof(TYPE_STR_PAIR[0]))

typedef enum {
    SWITCH_ON_CONTROL,
    SWITCH_OFF_CONTROL,
    SWITCH_ONOFF_TOGGLE_CONTROL,
    SWITCH_LEVEL_UP_CONTROL,
    SWITCH_LEVEL_DOWN_CONTROL,
    SWITCH_LEVEL_CYCLE_CONTROL,
    SWITCH_COLOR_CONTROL,
} switch_func_t;

typedef struct {
    uint8_t state;
    switch_func_t func;
} switch_func_pair_t;

typedef enum {
    SWITCH_IDLE,
    SWITCH_PRESS_ARMED,
    SWITCH_PRESS_DETECTED,
    SWITCH_PRESSED,
    SWITCH_RELEASE_DETECTED,
} switch_state_t;

static switch_func_pair_t button_func_pair[] = {
    {1, SWITCH_ONOFF_TOGGLE_CONTROL}
};

/* Default Coordinator config */
#define ESP_ZB_ZC_CONFIG()                                      \
    {                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,          \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,       \
        .nwk_cfg = {                                            \
            .zczr_cfg = {                                       \
                .max_children = MAX_CHILDREN,                   \
            },                                                  \
        }                                                       \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
    {                                                           \
        .radio_mode = RADIO_MODE_NATIVE,                        \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
    {                                                           \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,      \
    }

typedef struct light_bulb_device_params_s {
    esp_zb_ieee_addr_t ieee_addr;
    uint8_t  endpoint;
    uint16_t short_addr;
} light_bulb_device_params_t;

/* Zigbee configuration */
#define MAX_CHILDREN                    10          /* the max amount of connected devices */
#define INSTALLCODE_POLICY_ENABLE       false    /* enable the install code policy for security */
#define HA_ONOFF_SWITCH_ENDPOINT        1          /* esp light switch device endpoint */
#define ESP_ZB_PRIMARY_CHANNEL_MASK     ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK  /* Zigbee primary channel mask use in the example */

/********************* Define functions **************************/
static void esp_zb_buttons_handler(switch_func_pair_t *button_func_pair)
{
    if (button_func_pair->func == SWITCH_ONOFF_TOGGLE_CONTROL) {
        if(button_func_pair->state == 1 || button_func_pair->state == 0){
          esp_zb_zcl_on_off_cmd_t cmd_req;
          cmd_req.zcl_basic_cmd.src_endpoint = HA_ONOFF_SWITCH_ENDPOINT;
          cmd_req.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
          if(button_func_pair->state == 1){
            cmd_req.on_off_cmd_id = ESP_ZB_ZCL_CMD_ON_OFF_ON_ID;
            Serial.println("Send 'on' command");
          }
          else if(button_func_pair->state == 0){
            cmd_req.on_off_cmd_id = ESP_ZB_ZCL_CMD_ON_OFF_OFF_ID;
            Serial.println("Send 'off' command");
          }
          esp_zb_zcl_on_off_cmd_req(&cmd_req);
        }
    }
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

static void bind_cb(esp_zb_zdp_status_t zdo_status, void *user_ctx)
{
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
        Serial.println("Bound successfully!");
        if (user_ctx) {
            light_bulb_device_params_t *light = (light_bulb_device_params_t *)user_ctx;
            Serial.print("The light originating from address 0x");
            Serial.print(light->short_addr,HEX);
            Serial.print(" on endpoint ");
            Serial.println(light->endpoint);
            free(light);
        }
    }
}

static void user_find_cb(esp_zb_zdp_status_t zdo_status, uint16_t addr, uint8_t endpoint, void *user_ctx)
{
    if (zdo_status == ESP_ZB_ZDP_STATUS_SUCCESS) {
        Serial.println("Found light");
        esp_zb_zdo_bind_req_param_t bind_req;
        light_bulb_device_params_t *light = (light_bulb_device_params_t *)malloc(sizeof(light_bulb_device_params_t));
        light->endpoint = endpoint;
        light->short_addr = addr;
        esp_zb_ieee_address_by_short(light->short_addr, light->ieee_addr);
        esp_zb_get_long_address(bind_req.src_address);
        bind_req.src_endp = HA_ONOFF_SWITCH_ENDPOINT;
        bind_req.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
        bind_req.dst_addr_mode = ESP_ZB_ZDO_BIND_DST_ADDR_MODE_64_BIT_EXTENDED;
        memcpy(bind_req.dst_address_u.addr_long, light->ieee_addr, sizeof(esp_zb_ieee_addr_t));
        bind_req.dst_endp = endpoint;
        bind_req.req_dst_addr = esp_zb_get_short_address();
        Serial.println("Try to bind On/Off");
        esp_zb_zdo_device_bind_req(&bind_req, bind_cb, (void *)light);
    }
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)*p_sg_p;
    esp_zb_zdo_signal_device_annce_params_t *dev_annce_params = NULL;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        Serial.println("Zigbee stack initialized");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            Serial.println("Start network formation");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        } else {
            Serial.print("Failed to initialize Zigbee stack, status:");
            Serial.println(esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            Serial.print("Formed network successfully (Extended PAN ID:");
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
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else {
            Serial.print("Restart network formation status:");
            Serial.println(esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            Serial.println("Network steering started");
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        dev_annce_params = (esp_zb_zdo_signal_device_annce_params_t *)esp_zb_app_signal_get_params(p_sg_p);
        Serial.print("New device commissioned or rejoined (short: 0x");
        Serial.print(dev_annce_params->device_short_addr,HEX);
        Serial.println(")");
        esp_zb_zdo_match_desc_req_param_t  cmd_req;
        cmd_req.dst_nwk_addr = dev_annce_params->device_short_addr;
        cmd_req.addr_of_interest = dev_annce_params->device_short_addr;
        esp_zb_zdo_find_on_off_light(&cmd_req, user_find_cb, NULL);
        break;
    default:
//        Serial.println("ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
//                 esp_err_to_name(err_status));
        break;
    }
}
