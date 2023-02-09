//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <string.h>
#include <hal/uart_types.h>
#include <esp_err.h>
#include <driver/uart.h>
#include <esp_log.h>

#include "mbus-serial.h"
#include "mbus-protocol-aux.h"
#include "mbus-protocol.h"
#include "mqtt_manager.h"


//static const int PACKET_BUFF_SIZE = 1024 * 2;
static const int RX_BUF_SIZE = 1024;
#define TXD_PIN 4
#define RXD_PIN 5

int mbus_serial_wakeup(mbus_handle *handle) {
    ESP_LOGI("SERIAL_WAKEUP", "Sending serial wakeup!");

    uart_config_t uart_config = {
            .baud_rate = 2400,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));


    for (int i = 0; i < 132; i++) {

        const char wakeup[] = {0x55};
        const size_t wakeup_len = sizeof(wakeup);
        uart_write_bytes(UART_NUM_1, wakeup, wakeup_len);
    };
    vTaskDelay(pdMS_TO_TICKS(1300));
    if (handle == NULL)
        return -1;

    ESP_ERROR_CHECK(uart_driver_delete(UART_NUM_1));

    return 0;
}

//------------------------------------------------------------------------------
/// Set up a serial connection handle.
//------------------------------------------------------------------------------
int mbus_serial_connect(mbus_handle *handle) {
    uart_config_t uart_config = {
            .baud_rate = 2400,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_EVEN,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    return 0;
}

//------------------------------------------------------------------------------
// Set baud rate for serial connection
//------------------------------------------------------------------------------
int mbus_serial_set_baudrate(mbus_handle *handle, long baudrate) {
    if (handle == NULL)
        return -1;
    if (baudrate <= 0)
        return -1;
    ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_1, baudrate));

    return 0;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int mbus_serial_disconnect(mbus_handle *handle) {
    if (handle == NULL) {
        return -1;
    }


    uart_driver_delete(UART_NUM_1);

    return 0;
}

void mbus_serial_data_free(mbus_handle *handle) {
    mbus_serial_data *serial_data;

    if (handle) {
        serial_data = (mbus_serial_data *) handle->auxdata;

        if (serial_data == NULL) {
            return;
        }

        free(serial_data->device);
        free(serial_data);
        handle->auxdata = NULL;
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int mbus_serial_send_frame(mbus_handle *handle, mbus_frame *frame) {
    unsigned char buff[RX_BUF_SIZE];
    int len, ret;

    if (handle == NULL || frame == NULL) {
        fprintf(stderr, "handle or frame is null!\n");
        return -1;
    }


    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1) {
        fprintf(stderr, "%s: mbus_frame_pack failed\n", __PRETTY_FUNCTION__);
        return -1;
    }


    // if debug, dump in HEX form to stdout what we write to the serial port
    printf("%s: Dumping M-Bus frame [%d bytes]: ", __PRETTY_FUNCTION__, len);
    int i;
    for (i = 0; i < len; i++) {
        printf("%.2X ", buff[i]);
    }
    printf("\n");

    if ((ret = uart_write_bytes(UART_NUM_1, buff, len)) == len) {
        //
        // call the send event function, if the callback function is registered
        //
        if (handle->send_event)
            handle->send_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);
    } else {
        fprintf(stderr, "%s: Failed to write frame to socket (ret = %d: %s)\n", __PRETTY_FUNCTION__, ret,
                strerror(errno));
        return -1;
    }


    //
    // wait until complete frame has been transmitted
    //
    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int mbus_serial_recv_frame(mbus_handle *handle, mbus_frame *frame) {
    char buff[RX_BUF_SIZE];
    int remaining, timeouts;
    ssize_t len, nread;

    if (handle == NULL || frame == NULL) {
        fprintf(stderr, "%s: Invalid parameter.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_ERROR;
    }


    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));


    memset((void *) buff, 0, sizeof(buff));

    //
    // read data until a packet is received
    //
    remaining = 1; // start by reading 1 byte
    len = 0;
    timeouts = 0;

    do {
        if (len + remaining > RX_BUF_SIZE) {
            // avoid out of bounds access
            return MBUS_RECV_RESULT_ERROR;
        }

        printf("%s: Attempt to read %d bytes [len = %d]\n", __PRETTY_FUNCTION__, remaining, len);

        //if ((nread = read(handle->fd, &buff[len], remaining)) == -1) {
        if ((nread = uart_read_bytes(UART_NUM_1, &buff[len], (RX_BUF_SIZE - 1), pdMS_TO_TICKS(1000))) == -1) {

            fprintf(stderr, "%s: aborting recv frame (remaining = %d, len = %d, nread = %d)\n",
                    __PRETTY_FUNCTION__, remaining, len, nread);
            return MBUS_RECV_RESULT_ERROR;
        }

        printf("%s: Got %d byte [remaining %d, len %d]\n", __PRETTY_FUNCTION__, nread, remaining, len);

        if (nread == 0) {
            timeouts++;

            if (timeouts >= 3) {
                // abort to avoid endless loop
                fprintf(stderr, "%s: Timeout\n", __PRETTY_FUNCTION__);
                break;
            }
        }

        if (len > (SIZE_MAX - nread)) {
            // avoid overflow
            return MBUS_RECV_RESULT_ERROR;
        }

        len += nread;

    } while ((remaining = mbus_parse(frame, buff, len)) > 0);

    if (len == 0) {
        // No data received
        return MBUS_RECV_RESULT_TIMEOUT;
    }

    //
    // call the receive event function, if the callback function is registered
    //
    if (handle->recv_event)
        handle->recv_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);

    if (remaining != 0) {
        // Would be OK when e.g. scanning the bus, otherwise it is a failure.
        // printf("%s: M-Bus layer failed to receive complete data.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_INVALID;
    }

    if (len == -1) {
        fprintf(stderr, "%s: M-Bus layer failed to parse data.\n", __PRETTY_FUNCTION__);
        return MBUS_RECV_RESULT_ERROR;
    }

    return MBUS_RECV_RESULT_OK;
}
