/***********************************************************************************************************************
 * File Name    : ble_app.h
 * Description  : BLE Application Header - Rack Thermal Monitoring via Wireless
 **********************************************************************************************************************/

#ifndef BLE_APP_H_
#define BLE_APP_H_

#include <stdint.h>
#include <stdbool.h>

/* ========================================
   Bluetooth Remote Monitoring Interface
   ======================================== */

/* BLE Function Declarations */
void ble_app_init(void);
void ble_app_run(void);
void ble_app_close(void);
void ble_send_notification(uint8_t *p_data, uint16_t len);
bool ble_is_connected(void);

/* BLE Callback Functions */
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data);
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data);
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data);
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data);

/* Global BLE Variables */
extern uint16_t g_conn_hdl;

/* Rack Status Notification Structure */
typedef struct {
    int16_t temperature;        /* Temperature in °C × 100 */
    uint8_t cooling_level;      /* 0=OFF, 1=LOW, 2=MED, 3=HIGH, 4=EMERGENCY */
    uint8_t pwm_duty_cycle;     /* PWM 0-100% */
    uint8_t system_alert;       /* Alert flag */
    uint16_t sample_count;      /* Samples taken */
} ble_rack_status_t;

#endif /* BLE_APP_H_ */
