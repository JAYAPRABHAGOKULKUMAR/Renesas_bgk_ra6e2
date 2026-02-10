 #include <stdio.h>
#include <string.h>
#include "common_utils.h"
#include "main_application.h"
#include "gpt_timer.h"

/* Debug logging configuration */
#include "log_disabled.h"
//#include "log_error.h"
//#include "log_warning.h"
//#include "log_info.h"
//#include "log_debug.h"

/* ========================================
   SERVER RACK THERMAL MANAGEMENT SYSTEM
   Real-time Cooling Control
   ======================================== */

/* Temperature sensor data structure */
static temperature_sensor_data_t g_temp_sensor_data = {
    .current_temp = 0.0f,
    .previous_temp = 0.0f,
    .sample_count = 0,
    .pwm_duty_cycle = 0,
    .cooling_level = 0,
    .system_alert_active = 0
};

/* Timer variables for periodic sampling */
static uint32_t g_temp_sample_tick = 0;
static uint32_t g_ble_tx_tick = 0;

/* External timer control structures (configured in HAL) */
extern timer_ctrl_t g_timer_pwm_led1_ctrl;
extern timer_cfg_t g_timer_pwm_led1_cfg;

/**
 * @brief Get cooling level based on temperature
 * @param[in] temperature Current rack temperature
 * @return Cooling level (0=OFF, 1=LOW, 2=MEDIUM, 3=HIGH, 4=EMERGENCY)
 */
uint8_t get_cooling_level(float temperature)
{
    if (temperature < TEMP_LEVEL_OFF)
    {
        return 0;  /* OFF */
    }
    else if (temperature < TEMP_LEVEL_LOW)
    {
        return 1;  /* LOW */
    }
    else if (temperature < TEMP_LEVEL_MEDIUM)
    {
        return 2;  /* MEDIUM */
    }
    else if (temperature < TEMP_LEVEL_HIGH)
    {
        return 3;  /* HIGH */
    }
    else
    {
        return 4;  /* EMERGENCY */
    }
}

/**
 * @brief Map cooling level to PWM duty cycle
 * @param[in] cooling_level Cooling level (0-4)
 * @return PWM duty cycle percentage (0-100)
 */
static uint8_t cooling_level_to_pwm(uint8_t cooling_level)
{
    switch (cooling_level)
    {
        case 0: return PWM_DUTY_CYCLE_OFF;       /* OFF */
        case 1: return PWM_DUTY_CYCLE_LOW;       /* LOW - 25% */
        case 2: return PWM_DUTY_CYCLE_MEDIUM;    /* MEDIUM - 50% */
        case 3: return PWM_DUTY_CYCLE_HIGH;      /* HIGH - 75% */
        case 4: return PWM_DUTY_CYCLE_EMERGENCY; /* EMERGENCY - 100% */
        default: return PWM_DUTY_CYCLE_OFF;
    }
}

/**
 * @brief Initialize temperature sensor ADC
 */
void temp_sensor_init(void)
{
    fsp_err_t err = FSP_SUCCESS;
    
    log_info("========================================\r\n");
    log_info("SERVER RACK THERMAL MANAGEMENT SYSTEM\r\n");
    log_info("Initializing sensors...\r\n");
    log_info("========================================\r\n");
    
    /* ADC initialization through HAL configuration */
    log_info("Temperature Sensor: READY\r\n");
    log_info("Monitoring Range: 0-60°C\r\n");
    log_info("Sample Interval: %dms\r\n", TEMP_SAMPLE_INTERVAL_MS);
}

/**
 * @brief Read temperature from ADC
 * @param[out] p_temperature Pointer to store temperature value
 * @return FSP_SUCCESS if read successful
 */
fsp_err_t temp_sensor_read(float *p_temperature)
{
    fsp_err_t err = FSP_SUCCESS;
    uint16_t adc_result = 0;
    
    if (NULL == p_temperature)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }
    
    /* Simplified conversion for demonstration */
    *p_temperature = 25.5f;  /* Placeholder - replace with actual ADC reading */
    
    return err;
}

/**
 * @brief Update PWM fan speed based on temperature
 * @param[in] temperature Current rack temperature
 */
void pwm_control_update(float temperature)
{
    fsp_err_t err = FSP_SUCCESS;
    static uint8_t pwm_initialized = 0;
    uint8_t new_cooling_level;
    uint8_t new_pwm_duty;
    
    /* Initialize PWM on first call */
    if (!pwm_initialized)
    {
        err = init_gpt_timer(&g_timer_pwm_led1_ctrl, &g_timer_pwm_led1_cfg);
        if (FSP_SUCCESS != err)
        {
            log_error("Fan control initialization FAILED\r\n");
            return;
        }
        
        err = start_gpt_timer(&g_timer_pwm_led1_ctrl);
        if (FSP_SUCCESS != err)
        {
            log_error("Fan control start FAILED\r\n");
            deinit_gpt_timer(&g_timer_pwm_led1_ctrl);
            return;
        }
        
        pwm_initialized = 1;
        log_info("Fan Control System: ONLINE\r\n");
    }
    
    /* Determine new cooling level based on temperature */
    new_cooling_level = get_cooling_level(temperature);
    new_pwm_duty = cooling_level_to_pwm(new_cooling_level);
    
    /* Update only if cooling level changed (reduce noise) */
    if (new_cooling_level != g_temp_sensor_data.cooling_level)
    {
        g_temp_sensor_data.cooling_level = new_cooling_level;
        g_temp_sensor_data.pwm_duty_cycle = new_pwm_duty;
        
        /* Update PWM */
        err = set_timer_duty_cycle(new_pwm_duty, &g_timer_pwm_led1_ctrl);
        
        if (FSP_SUCCESS == err)
        {
            /* Log cooling level changes */
            const char* level_names[] = {"OFF", "LOW", "MEDIUM", "HIGH", "EMERGENCY"};
            log_info("THERMAL CONTROL: Temp=%.1f°C, Level=%s (PWM=%d%%)\r\n", 
                     temperature, level_names[new_cooling_level], new_pwm_duty);
        }
    }
    
    /* Check for critical conditions */
    if (temperature >= SYSTEM_CRITICAL_TEMP)
    {
        g_temp_sensor_data.system_alert_active = 1;
        log_error("⚠️  CRITICAL TEMPERATURE ALERT: %.1f°C\r\n", temperature);
    }
    else if (temperature >= SYSTEM_SHUTDOWN_TEMP)
    {
        log_error("🚨 EMERGENCY: Temperature %.1f°C - THERMAL SHUTDOWN INITIATED\r\n", temperature);
        /* In production: trigger emergency shutdown */
    }
    else if (g_temp_sensor_data.system_alert_active && temperature < (SYSTEM_CRITICAL_TEMP - TEMP_HYSTERESIS))
    {
        g_temp_sensor_data.system_alert_active = 0;
        log_info("✅ Alert cleared - Temperature normalized\r\n");
    }
}

/**
 * @brief Send rack status via Bluetooth
 * @param[in] temperature Current rack temperature
 */
void ble_send_temperature_data(float temperature)
{
    uint8_t ble_data[MAX_SENSOR_DATA_LEN];
    uint16_t data_len = 0;
    
    /* Pack thermal data into BLE packet */
    int16_t temp_int = (int16_t)(temperature * 100);
    
    ble_data[0] = (uint8_t)(temp_int & 0xFF);
    ble_data[1] = (uint8_t)((temp_int >> 8) & 0xFF);
    
    /* Cooling level (0-4) */
    ble_data[2] = g_temp_sensor_data.cooling_level;
    
    /* PWM duty cycle (0-100) */
    ble_data[3] = g_temp_sensor_data.pwm_duty_cycle;
    
    /* System alert flag */
    ble_data[4] = g_temp_sensor_data.system_alert_active;
    
    /* Sample count */
    ble_data[5] = (uint8_t)(g_temp_sensor_data.sample_count & 0xFF);
    ble_data[6] = (uint8_t)((g_temp_sensor_data.sample_count >> 8) & 0xFF);
    
    data_len = 7;
    
    log_debug("BLE TX: Temp=%.1f°C, Level=%d, PWM=%d%%, Alert=%d\r\n", 
              temperature, g_temp_sensor_data.cooling_level, 
              g_temp_sensor_data.pwm_duty_cycle, g_temp_sensor_data.system_alert_active);
}

/**
 * @brief Main application loop - Server Rack Thermal Management
 */
void main_application(void)
{
    fsp_err_t err = FSP_SUCCESS;
    float current_temperature = 0.0f;
    uint32_t tick_ms = 0;
    
    log_info("\r\n╔════════════════════════════════════════╗\r\n");
    log_info("║ RACK THERMAL CONTROL SYSTEM - STARTING ║\r\n");
    log_info("╚════════════════════════════════════════╝\r\n\r\n");
    
    /* Initialize temperature sensor */
    temp_sensor_init();
    
    /* Main control loop */
    while (true)
    {
        tick_ms++;
        
        /* STEP 1: Environment Sensing (every 1 second) */
        if (tick_ms >= TEMP_SAMPLE_INTERVAL_MS)
        {
            tick_ms = 0;
            
            /* Read current rack temperature */
            err = temp_sensor_read(&current_temperature);
            if (FSP_SUCCESS == err)
            {
                g_temp_sensor_data.current_temp = current_temperature;
                g_temp_sensor_data.sample_count++;
                
                /* STEP 3: Decision & Control - Update cooling */
                pwm_control_update(current_temperature);
                
                log_debug("Rack Temperature: %.1f°C | Sample: %d\r\n", 
                         current_temperature, g_temp_sensor_data.sample_count);
            }
            else
            {
                log_error("Temperature sensor read FAILED\r\n");
            }
        }
        
        /* STEP 7: Monitoring Output (BLE update every 500ms) */
        if (tick_ms % BLE_TX_INTERVAL_MS == 0)
        {
            ble_send_temperature_data(current_temperature);
        }
        
        /* STEP 6: Feedback Loop - Continuous monitoring */
        /* Maintain 1ms loop cycle for responsive thermal control */
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
    }
}



