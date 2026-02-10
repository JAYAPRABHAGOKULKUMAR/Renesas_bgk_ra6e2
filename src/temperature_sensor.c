/***********************************************************************************************************************
 * File Name    : temperature_sensor.c
 * Description  : Server Rack Temperature Monitoring ADC Driver
 **********************************************************************************************************************/

#include "hal_data.h"
#include "temperature_sensor.h"
#include "log_disabled.h"

/* ADC Configuration */
#define ADC_RESOLUTION          12          /* 12-bit ADC */
#define ADC_MAX_VALUE           4095        /* 2^12 - 1 */
#define ADC_REFERENCE_VOLTAGE   3.3f        /* 3.3V reference */
#define TEMP_SENSOR_CHANNEL     0           /* ADC channel for rack temperature */

/* Temperature Sensor Calibration for Server Rack */
#define TEMP_SENSOR_V_25        0.75f       /* Voltage at 25°C */
#define TEMP_SENSOR_TC          -0.01f      /* Temperature coefficient (mV/°C) */

/* ADC Instance */
extern adc_ctrl_t g_adc0_ctrl;
extern const adc_cfg_t g_adc0_cfg;

/* Static variables */
static volatile uint16_t g_adc_result = 0;
static uint32_t g_sample_count = 0;

/**
 * @brief ADC Callback for Rack Temperature Reading
 */
static void adc_callback(adc_callback_args_t *p_args)
{
    if (ADC_EVENT_SCAN_COMPLETE == p_args->event)
    {
        g_adc_result = p_args->result;
        g_sample_count++;
    }
}

/**
 * @brief Initialize Temperature Sensor ADC for Rack Monitoring
 * @return FSP_SUCCESS if successful
 */
fsp_err_t temp_sensor_adc_init(void)
{
    fsp_err_t err = FSP_SUCCESS;
    
    log_info("Initializing Rack Temperature Sensor...\r\n");
    
    /* Open ADC */
    err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    if (FSP_SUCCESS != err)
    {
        log_error("Rack Temperature Sensor: INITIALIZATION FAILED\r\n");
        return err;
    }
    
    /* Configure scan */
    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_cfg.scan_cfg);
    if (FSP_SUCCESS != err)
    {
        log_error("Rack Temperature Sensor: CONFIG FAILED\r\n");
        R_ADC_Close(&g_adc0_ctrl);
        return err;
    }
    
    /* Enable ADC */
    err = R_ADC_ScanStart(&g_adc0_ctrl);
    if (FSP_SUCCESS != err)
    {
        log_error("Rack Temperature Sensor: START FAILED\r\n");
        R_ADC_Close(&g_adc0_ctrl);
        return err;
    }
    
    log_info("Rack Temperature Sensor: ONLINE\r\n");
    log_info("Monitoring Range: 0-65°C\r\n");
    return FSP_SUCCESS;
}

/**
 * @brief Read Rack Temperature from ADC
 * @param[out] p_temperature Pointer to store temperature in Celsius
 * @return FSP_SUCCESS if successful
 */
fsp_err_t temp_sensor_read_adc(float *p_temperature)
{
    fsp_err_t err = FSP_SUCCESS;
    uint16_t adc_result = 0;
    float voltage = 0.0f;
    float temperature = 0.0f;
    
    if (NULL == p_temperature)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }
    
    /* Read ADC value */
    err = R_ADC_Read(&g_adc0_ctrl, TEMP_SENSOR_CHANNEL, &adc_result);
    if (FSP_SUCCESS != err)
    {
        log_error("ADC Read FAILED\r\n");
        return err;
    }
    
    /* Convert ADC value to voltage */
    voltage = ((float)adc_result / (float)ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE;
    
    /* Convert voltage to temperature */
    /* Formula: Temp = 25 + (V_ref - V_adc) / TC */
    temperature = 25.0f + ((TEMP_SENSOR_V_25 - voltage) / TEMP_SENSOR_TC);
    
    *p_temperature = temperature;
    
    log_debug("Rack Temp: ADC=0x%04x, V=%.3fV, T=%.1f°C\r\n", 
             adc_result, voltage, temperature);
    
    return FSP_SUCCESS;
}

/**
 * @brief Deinitialize Rack Temperature Sensor
 */
void temp_sensor_adc_deinit(void)
{
    R_ADC_Close(&g_adc0_ctrl);
    log_info("Rack Temperature Sensor: OFFLINE\r\n");
}

/**
 * @brief Get Temperature Sample Count
 */
uint32_t temp_sensor_get_sample_count(void)
{
    return g_sample_count;
}
