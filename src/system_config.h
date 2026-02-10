/***********************************************************************************************************************
 * File Name    : system_config.h
 * Description  : Server Rack Thermal Management System Configuration
 **********************************************************************************************************************/

#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_

/* ========================================
   System Operation Mode Configuration
   ======================================== */
#define ENABLE_TEMPERATURE_SENSOR   1
#define ENABLE_PWM_CONTROL          1          /* Fan control */
#define ENABLE_BLUETOOTH            1          /* Remote monitoring */
#define USE_RTOS                    0          /* Polling-based (no FreeRTOS) */
#define USE_POLLING_MODE            1

/* ========================================
   RACK THERMAL MONITORING
   Multi-Level Cooling Control
   ======================================== */

#define TEMP_SENSOR_SAMPLE_RATE_MS  1000       /* Monitor every 1 second */
#define TEMP_SENSOR_FILTER_SAMPLES  10         /* 10-sample moving average */

/* Temperature Control Levels */
#define TEMP_OFF_THRESHOLD          30.0f      /* Fans OFF below 30°C */
#define TEMP_LOW_THRESHOLD          40.0f      /* LOW cooling 30-40°C */
#define TEMP_MEDIUM_THRESHOLD       50.0f      /* MEDIUM cooling 40-50°C */
#define TEMP_HIGH_THRESHOLD         55.0f      /* HIGH cooling 50-55°C */
#define TEMP_EMERGENCY_THRESHOLD    60.0f      /* EMERGENCY above 55°C */

#define TEMP_CRITICAL_THRESHOLD     58.0f      /* Critical alert */
#define TEMP_SHUTDOWN_THRESHOLD     65.0f      /* Thermal shutdown */

#define TEMP_HYSTERESIS             1.0f       /* ±1°C hysteresis */

/* ========================================
   COOLING FAN PWM CONTROL
   ======================================== */

#define PWM_FREQUENCY_HZ            1000       /* 1 kHz fan control */
#define PWM_PERIOD_MS               1          /* 1ms period */

#define PWM_OFF_DUTY_CYCLE          0          /* Fans OFF */
#define PWM_LOW_DUTY_CYCLE          25         /* Low speed - quiet operation */
#define PWM_MEDIUM_DUTY_CYCLE       50         /* Medium speed */
#define PWM_HIGH_DUTY_CYCLE         75         /* High speed - active cooling */
#define PWM_EMERGENCY_DUTY_CYCLE    100        /* Maximum cooling + alarm */

/* ========================================
   BLUETOOTH REMOTE MONITORING
   ======================================== */

#define BLE_TX_INTERVAL_MS          500        /* Update every 500ms */
#define BLE_ADV_INTERVAL_MS         100        /* Advertise every 100ms */
#define BLE_ADV_TIMEOUT_SEC         0          /* Continuous advertising */
#define BLE_MTU_SIZE                101        /* Maximum Transmission Unit */
#define BLE_DEVICE_NAME             "RackCooler"

/* ========================================
   BLE DATA PACKET STRUCTURE
   ======================================== */

#define BLE_TEMP_DATA_SIZE          7
typedef struct {
    int16_t temperature;                /* Temperature * 100 (Celsius) */
    uint8_t cooling_level;              /* 0=OFF, 1=LOW, 2=MED, 3=HIGH, 4=EMERGENCY */
    uint8_t pwm_duty_cycle;             /* PWM duty cycle 0-100% */
    uint8_t system_alert;               /* Alert flag */
    uint16_t sample_count;              /* Packet sequence number */
} ble_rack_status_t;

/* ========================================
   SYSTEM SAFETY LIMITS
   ======================================== */

#define MAX_TEMP_CELSIUS            65.0f      /* Maximum safe temperature */
#define MIN_TEMP_CELSIUS            0.0f       /* Minimum safe temperature */
#define TEMP_SENSOR_TIMEOUT_MS      5000       /* Timeout for sensor read */

/* ========================================
   DEBUG & DIAGNOSTICS
   ======================================== */

#define ENABLE_DEBUG_LOG            1
#define DEBUG_SERIAL_PORT           0          /* UART0 for debug output */
#define ENABLE_SENSOR_DIAGNOSTICS   1
#define ENABLE_THERMAL_LOGGING      1          /* Log all thermal events */

/* ========================================
   RACK OPERATIONAL PARAMETERS
   ======================================== */

#define RACK_SAFE_TEMPERATURE       30.0f      /* Target safe temperature */
#define RACK_WARNING_TEMPERATURE    45.0f      /* Warning threshold */
#define RACK_CRITICAL_TEMPERATURE   58.0f      /* Critical alert threshold */

#define INTAKE_FAN_ENABLE           1          /* Incoming cool air */
#define EXHAUST_FAN_ENABLE          1          /* Outgoing hot air */

#endif /* SYSTEM_CONFIG_H_ */
