/***********************************************************************************************************************
 * File Name    : temperature_sensor.h
 * Description  : Server Rack Temperature Monitoring ADC Driver
 **********************************************************************************************************************/

#ifndef TEMPERATURE_SENSOR_H_
#define TEMPERATURE_SENSOR_H_

#include "hal_data.h"

/* Temperature Sensor Configuration for Rack Monitoring */
#define TEMP_MIN_CELSIUS        0.0f
#define TEMP_MAX_CELSIUS        65.0f
#define TEMP_SAFE_CELSIUS       30.0f        /* Optimal operating range */
#define TEMP_WARNING_CELSIUS    45.0f        /* Warning level */
#define TEMP_CRITICAL_CELSIUS   58.0f        /* Critical alert */

/* ADC Configuration */
#define ADC_RESOLUTION          12            /* 12-bit ADC */
#define ADC_MAX_VALUE           4095          /* 2^12 - 1 */
#define ADC_REFERENCE_VOLTAGE   3.3f          /* 3.3V reference */
#define TEMP_SENSOR_CHANNEL     0             /* ADC channel for rack temperature */

/* Function Declarations */
fsp_err_t temp_sensor_adc_init(void);
fsp_err_t temp_sensor_read_adc(float *p_temperature);
void temp_sensor_adc_deinit(void);
uint32_t temp_sensor_get_sample_count(void);

/* Temperature Data Structure for Rack Monitoring */
typedef struct {
    float current_temp;
    float min_temp;
    float max_temp;
    float avg_temp;
    uint32_t sample_count;
    uint8_t temperature_status;        /* SAFE / WARNING / CRITICAL */
} temp_sensor_data_t;

/* Temperature Status Levels */
#define TEMP_STATUS_SAFE        0
#define TEMP_STATUS_WARNING     1
#define TEMP_STATUS_CRITICAL    2

#endif /* TEMPERATURE_SENSOR_H_ */
