#ifndef __MAIN_APPLICATION_H
#define __MAIN_APPLICATION_H

#include "common_utils.h"

/* ========================================
   SERVER RACK THERMAL MANAGEMENT
   Embedded Cooling Control System
   ======================================== */

/* Temperature Sensor Configuration */
#define TEMP_SENSOR_ENABLE          1
#define TEMP_SAMPLE_INTERVAL_MS     1000       /* Sample every 1 second */
#define TEMP_ADC_CHANNEL            0

/* ========================================
   THERMAL THRESHOLDS (°C)
   Multi-Level Cooling Control
   ======================================== */

#define TEMP_LEVEL_OFF              30.0f      /* Below: Fans OFF */
#define TEMP_LEVEL_LOW              40.0f      /* 30-40°C: LOW speed (25%) */
#define TEMP_LEVEL_MEDIUM           50.0f      /* 40-50°C: MEDIUM speed (50%) */
#define TEMP_LEVEL_HIGH             55.0f      /* 50-55°C: HIGH speed (75%) */
#define TEMP_LEVEL_EMERGENCY        60.0f      /* Above: EMERGENCY (100%) + ALERT */

/* ========================================
   PWM DUTY CYCLE CONTROL (%)
   ======================================== */

#define PWM_ENABLE                  1
#define PWM_DEFAULT_PERIOD_MS       1000       /* 1 second period */

#define PWM_DUTY_CYCLE_OFF          0          /* Fans OFF */
#define PWM_DUTY_CYCLE_LOW          25         /* Low cooling */
#define PWM_DUTY_CYCLE_MEDIUM       50         /* Medium cooling */
#define PWM_DUTY_CYCLE_HIGH         75         /* High cooling */
#define PWM_DUTY_CYCLE_EMERGENCY    100        /* Maximum cooling + alarm */

/* ========================================
   HYSTERESIS (Prevent Rapid Switching)
   ======================================== */

#define TEMP_HYSTERESIS             1.0f       /* ±1°C hysteresis */

/* ========================================
   BLUETOOTH CONFIGURATION
   ======================================== */

#define BLE_TX_INTERVAL_MS          500        /* Send status every 500ms */
#define MAX_SENSOR_DATA_LEN         20
#define BLE_DEVICE_NAME             "RackCooler"

/* ========================================
   SYSTEM MONITORING
   ======================================== */

#define SYSTEM_CRITICAL_TEMP        58.0f      /* Critical temperature threshold */
#define SYSTEM_SHUTDOWN_TEMP        65.0f      /* Emergency shutdown temperature */

/* Function declarations */
void main_application(void);
void temp_sensor_init(void);
fsp_err_t temp_sensor_read(float *p_temperature);
void pwm_control_update(float temperature);
void ble_send_temperature_data(float temperature);
uint8_t get_cooling_level(float temperature);

/* Temperature data structure */
typedef struct {
    float current_temp;
    float previous_temp;
    uint32_t sample_count;
    uint8_t pwm_duty_cycle;        /* Current PWM duty cycle (0-100) */
    uint8_t cooling_level;         /* 0=OFF, 1=LOW, 2=MEDIUM, 3=HIGH, 4=EMERGENCY */
    uint8_t system_alert_active;   /* Alert flag for critical conditions */
} temperature_sensor_data_t;

#endif /* __MAIN_APPLICATION_H */
