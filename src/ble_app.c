/***********************************************************************************************************************
 * File Name    : ble_app.c
 * Description  : BLE Application for Temperature Sensor Monitoring
 * No FreeRTOS - Polling-based implementation
 **********************************************************************************************************************/

#include <string.h>
#include "r_ble_api.h"
#include "rm_ble_abs.h"
#include "rm_ble_abs_api.h"
#include "gatt_db.h"
#include "profile_cmn/r_ble_servs_if.h"
#include "profile_cmn/r_ble_servc_if.h"
#include "hal_data.h"
#include "common_utils.h"
#include "main_application.h"
#include "log_disabled.h"

/* BLE Configuration Constants */
#define BLE_LOG_TAG "ble_app"
#define BLE_GATTS_QUEUE_ELEMENTS_SIZE   (14)
#define BLE_GATTS_QUEUE_BUFFER_LEN      (245)
#define BLE_GATTS_QUEUE_NUM             (1)
#define BLE_OPTIMAL_MTU                 101
#define MAX_ADV_DATA_LENGTH             (20)
#define PRE_ADV_DATA_LEN                (6)  /* "US000-" */

/* Global variables */
uint16_t g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
static bool g_ble_connected = false;

/* Advertisement data */
static const char pre_adv_data[] = "US000-";
static uint8_t gs_advertising_data[] = {
    /* Flags */
    0x02, 0x01, 0x06,
    /* Complete Local Name */
    0x13, 0x09, 'T', 'E', 'M', 'P', '_', 'S', 'E', 'N', 'S', 'O', 'R', 0x00, 0x00, 0x00, 0x00, 0x00
};

/* BLE Advertising Parameters */
ble_abs_legacy_advertising_parameter_t g_ble_advertising_parameter = {
    .p_peer_address             = NULL,
    .slow_advertising_interval  = 0x000000A0,  /* 100ms */
    .slow_advertising_period    = 0x0000,
    .p_advertising_data         = gs_advertising_data,
    .advertising_data_length    = sizeof(gs_advertising_data),
    .advertising_filter_policy  = BLE_ABS_ADVERTISING_FILTER_ALLOW_ANY,
    .advertising_channel_map    = (BLE_GAP_ADV_CH_37 | BLE_GAP_ADV_CH_38 | BLE_GAP_ADV_CH_39),
    .own_bluetooth_address_type = BLE_GAP_ADDR_RAND,
    .own_bluetooth_address      = { 0 }
};

/* GATT Server Queue */
static st_ble_gatt_queue_elm_t  gs_queue_elms[BLE_GATTS_QUEUE_ELEMENTS_SIZE];
static uint8_t gs_buffer[BLE_GATTS_QUEUE_BUFFER_LEN];
static st_ble_gatt_pre_queue_t gs_queue[BLE_GATTS_QUEUE_NUM] = {
    {
        .p_buf_start = gs_buffer,
        .buffer_len  = BLE_GATTS_QUEUE_BUFFER_LEN,
        .p_queue     = gs_queue_elms,
        .queue_size  = BLE_GATTS_QUEUE_ELEMENTS_SIZE,
    }
};

/* GATT Server Callback Parameters */
ble_abs_gatt_server_callback_set_t gs_abs_gatts_cb_param[] = {
    {
        .gatt_server_callback_function = gatts_cb,
        .gatt_server_callback_priority = 1,
    },
    {
        .gatt_server_callback_function = NULL,
    }
};

/* GATT Client Callback Parameters */
ble_abs_gatt_client_callback_set_t gs_abs_gattc_cb_param[] = {
    {
        .gatt_client_callback_function = gattc_cb,
        .gatt_client_callback_priority = 1,
    },
    {
        .gatt_client_callback_function = NULL,
    }
};

/*******************************************************************************
 * Callback Functions
 *******************************************************************************/

/**
 * @brief GAP Callback
 */
void gap_cb(uint16_t type, ble_status_t result, st_ble_evt_data_t *p_data)
{
    switch(type)
    {
        case BLE_GAP_EVENT_STACK_ON:
        {
            log_info("BLE Stack initialized\r\n");
            R_BLE_VS_GetBdAddr(BLE_VS_ADDR_AREA_REG, BLE_GAP_ADDR_RAND);
        }
        break;

        case BLE_GAP_EVENT_CONN_IND:
        {
            if (BLE_SUCCESS == result)
            {
                st_ble_gap_conn_evt_t *p_gap_conn_evt_param = (st_ble_gap_conn_evt_t *)p_data->p_param;
                g_conn_hdl = p_gap_conn_evt_param->conn_hdl;
                g_ble_connected = true;
                log_info("BLE Connected, handle: 0x%04x\r\n", g_conn_hdl);
            }
            else
            {
                log_error("BLE Connection failed\r\n");
                RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
            }
        }
        break;

        case BLE_GAP_EVENT_DISCONN_IND:
        {
            g_conn_hdl = BLE_GAP_INVALID_CONN_HDL;
            g_ble_connected = false;
            log_info("BLE Disconnected\r\n");
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
        }
        break;

        case BLE_GAP_EVENT_CONN_PARAM_UPD_REQ:
        {
            st_ble_gap_conn_upd_req_evt_t *p_conn_upd_req_evt_param = 
                (st_ble_gap_conn_upd_req_evt_t *)p_data->p_param;

            st_ble_gap_conn_param_t conn_updt_param = {
                .conn_intv_min = p_conn_upd_req_evt_param->conn_intv_min,
                .conn_intv_max = p_conn_upd_req_evt_param->conn_intv_max,
                .conn_latency  = p_conn_upd_req_evt_param->conn_latency,
                .sup_to        = p_conn_upd_req_evt_param->sup_to,
            };

            R_BLE_GAP_UpdConn(p_conn_upd_req_evt_param->conn_hdl,
                            BLE_GAP_CONN_UPD_MODE_RSP,
                            BLE_GAP_CONN_UPD_ACCEPT,
                            &conn_updt_param);
        }
        break;

        default:
            break;
    }
}

/**
 * @brief GATT Server Callback
 */
void gatts_cb(uint16_t type, ble_status_t result, st_ble_gatts_evt_data_t *p_data)
{
    switch(type)
    {
        case BLE_GATTS_EVENT_DB_ACCESS_IND:
        {
            log_debug("GATT DB Access\r\n");
        }
        break;

        case BLE_GATTS_EVENT_HDL_VAL_CNF:
        {
            log_debug("GATT Notification confirmed\r\n");
        }
        break;

        default:
            break;
    }
}

/**
 * @brief GATT Client Callback
 */
void gattc_cb(uint16_t type, ble_status_t result, st_ble_gattc_evt_data_t *p_data)
{
    R_BLE_SERVC_GattcCb(type, result, p_data);
}

/**
 * @brief Vendor Specific Callback
 */
void vs_cb(uint16_t type, ble_status_t result, st_ble_vs_evt_data_t *p_data)
{
    R_BLE_SERVS_VsCb(type, result, p_data);
    
    switch(type)
    {
        case BLE_VS_EVENT_GET_ADDR_COMP:
        {
            st_ble_vs_get_bd_addr_comp_evt_t *get_address = 
                (st_ble_vs_get_bd_addr_comp_evt_t *)p_data->p_param;
            memcpy(g_ble_advertising_parameter.own_bluetooth_address, 
                   get_address->addr.addr, BLE_BD_ADDR_LEN);
            log_info("Starting BLE Advertisement\r\n");
            RM_BLE_ABS_StartLegacyAdvertising(&g_ble_abs0_ctrl, &g_ble_advertising_parameter);
        }
        break;

        default:
            break;
    }
}

/**
 * @brief Quick Connect Service Callback
 */
static void qc_svcs_cb(uint16_t type, ble_status_t result, st_ble_servs_evt_data_t *p_data)
{
    FSP_PARAMETER_NOT_USED(result);
    FSP_PARAMETER_NOT_USED(p_data);
}

/*******************************************************************************
 * BLE Initialization
 *******************************************************************************/

/**
 * @brief Initialize BLE Stack
 */
ble_status_t ble_init(void)
{
    ble_status_t status;
    fsp_err_t err;

    log_info("Initializing BLE\r\n");

    /* Initialize BLE */
    err = RM_BLE_ABS_Open(&g_ble_abs0_ctrl, &g_ble_abs0_cfg);
    if (FSP_SUCCESS != err)
    {
        log_error("BLE Open failed: 0x%04x\r\n", err);
        return err;
    }

    /* Initialize GATT Database */
    status = R_BLE_GATTS_SetDbInst(&g_gatt_db_table);
    if (BLE_SUCCESS != status)
    {
        log_error("GATT DB initialization failed\r\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT Server */
    status = R_BLE_SERVS_Init();
    if (BLE_SUCCESS != status)
    {
        log_error("GATT Server initialization failed\r\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Initialize GATT Client */
    status = R_BLE_SERVC_Init();
    if (BLE_SUCCESS != status)
    {
        log_error("GATT Client initialization failed\r\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    /* Set Prepare Write Queue */
    R_BLE_GATTS_SetPrepareQueue(gs_queue, BLE_GATTS_QUEUE_NUM);

    /* Initialize Quick Connect Service */
    status = R_BLE_QC_SVCS_Init(qc_svcs_cb);
    if (BLE_SUCCESS != status)
    {
        log_error("QC Service initialization failed\r\n");
        return BLE_ERR_INVALID_OPERATION;
    }

    log_info("BLE initialization complete\r\n");
    return status;
}

/**
 * @brief Initialize BLE Application
 */
void ble_app_init(void)
{
    log_info("Starting BLE Application\r\n");

    if (BLE_SUCCESS != ble_init())
    {
        log_error("BLE initialization failed\r\n");
    }
}

/**
 * @brief BLE Application Main Loop (No-RTOS Polling)
 */
void ble_app_run(void)
{
    /* Process BLE events */
    R_BLE_Execute();
}

/**
 * @brief Cleanup BLE Application
 */
void ble_app_close(void)
{
    RM_BLE_ABS_Close(&g_ble_abs0_ctrl);
}

/**
 * @brief Send Temperature Data via BLE Notification
 */
void ble_send_notification(uint8_t *p_data, uint16_t len)
{
    if (!g_ble_connected || g_conn_hdl == BLE_GAP_INVALID_CONN_HDL)
    {
        log_debug("BLE not connected, notification not sent\r\n");
        return;
    }

    st_ble_gatt_hdl_value_pair_t hdl_value_pair;
    hdl_value_pair.attr_hdl        = 0;  /* Update with actual characteristic handle */
    hdl_value_pair.value.p_value   = p_data;
    hdl_value_pair.value.value_len = len;

    ble_status_t status = R_BLE_GATTS_Notification(g_conn_hdl, &hdl_value_pair);
    if (BLE_SUCCESS != status)
    {
        log_debug("BLE Notification failed: 0x%04x\r\n", status);
    }
}

/**
 * @brief Get BLE Connection Status
 */
bool ble_is_connected(void)
{
    return g_ble_connected;
}
