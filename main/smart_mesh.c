#include "smart_mesh.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"

#define COMPANY_ID 0x05F1
#define SENSOR_OPCODE ESP_BLE_MESH_MODEL_OP_3(0xC1, COMPANY_ID)
static const char *TAG = "SMART_MESH";
/* Stable namespace plus factory MAC: no random UUID, no flash-dependent identity. */
static uint8_t uuid[16] = {0xF1, 0x05, 'S', 'M', 'A', 'R', 'T', 'P', 0, 1};
ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_pub, 3 + 8, ROLE_NODE);
static esp_ble_mesh_cfg_srv_t cfg = {
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};
/* Telemetry only; no incoming pump/control opcodes. */
static esp_ble_mesh_model_op_t vendor_ops[] = {ESP_BLE_MESH_MODEL_OP_END};
static esp_ble_mesh_model_t root_models[] = {ESP_BLE_MESH_MODEL_CFG_SRV(&cfg)};
static esp_ble_mesh_model_t vendor_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(COMPANY_ID, 0x0001, vendor_ops, &sensor_pub, NULL),
};
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vendor_models),
};
static esp_ble_mesh_comp_t composition = {
    .cid = COMPANY_ID, .element_count = 1, .elements = elements,
};
static esp_ble_mesh_prov_t provisioning = {.uuid = uuid, .output_size = 0, .output_actions = 0};

static void prov_cb(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *p)
{
    if (event == ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT) {
        ESP_LOGI(TAG, "Provisioned: net_idx=0x%04x address=0x%04x",
                 p->node_prov_complete.net_idx, p->node_prov_complete.addr);
    } else if (event == ESP_BLE_MESH_NODE_PROV_RESET_EVT) {
        ESP_ERROR_CHECK(esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV));
    }
}
static void config_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                      esp_ble_mesh_cfg_server_cb_param_t *p)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        ESP_LOGI(TAG, "Configuration changed: opcode=0x%08lx", (unsigned long)p->ctx.recv_op);
    }
}
static void model_cb(esp_ble_mesh_model_cb_event_t event, esp_ble_mesh_model_cb_param_t *p)
{
    if (event == ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT && p->model_publish_comp.err_code) {
        ESP_LOGW(TAG, "Publication failed: %d", p->model_publish_comp.err_code);
    }
}
esp_err_t smart_mesh_init(void)
{
    /* Do not erase NVS automatically: that would silently lose network credentials. */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_read_mac(uuid + 10, ESP_MAC_BT));
    ESP_LOGI(TAG, "Device UUID:");
    ESP_LOG_BUFFER_HEX(TAG, uuid, sizeof(uuid));
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_ble_mesh_register_prov_callback(prov_cb));
    ESP_ERROR_CHECK(esp_ble_mesh_register_config_server_callback(config_cb));
    ESP_ERROR_CHECK(esp_ble_mesh_register_custom_model_callback(model_cb));
    ESP_ERROR_CHECK(esp_ble_mesh_init(&provisioning, &composition));
    if (!esp_ble_mesh_node_is_provisioned()) {
        return esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV);
    }
    ESP_LOGI(TAG, "Restored mesh from NVS");
    return ESP_OK;
}
esp_err_t smart_mesh_publish(const uint8_t payload[8])
{
    if (!payload) return ESP_ERR_INVALID_ARG;
    /* Inspect restored live state, not a RAM flag set only by configuration callbacks. */
    if (!esp_ble_mesh_node_is_provisioned() || sensor_pub.publish_addr != 0x0001 ||
        sensor_pub.app_idx != 0 || sensor_pub.period != 0 ||
        !esp_ble_mesh_node_get_local_net_key(0) || !esp_ble_mesh_node_get_local_app_key(0)) {
        return ESP_ERR_INVALID_STATE;
    }
    bool bound = false;
    for (size_t i = 0; i < sizeof(vendor_models[0].keys) / sizeof(vendor_models[0].keys[0]); ++i) {
        if (vendor_models[0].keys[i] == 0) bound = true;
    }
    if (!bound) return ESP_ERR_INVALID_STATE;
    return esp_ble_mesh_model_publish(&vendor_models[0], SENSOR_OPCODE, 8,
                                       (uint8_t *)payload, ROLE_NODE);
}
