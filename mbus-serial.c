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




//    for (int i = 0; i < 132; ++i) {
//        uart_write_bytes(UART_NUM_1, data, len);
//
//    }



    //uart_write_bytes(UART_NUM_1, "TEST", 5);
    //uart_flush(UART_NUM_1);
    //    mbus_serial_data *serial_data;
//    const char *device;
//    struct termios *term;
//
//    if (handle == NULL)
//        return -1;
//
//    serial_data = (mbus_serial_data *) handle->auxdata;
//    if (serial_data == NULL || serial_data->device == NULL)
//        return -1;
//
//    device = serial_data->device;
//    term = &(serial_data->t);
//    //
//    // create the SERIAL connection
//    //
//
//    // Use blocking read and handle it by serial port VMIN/VTIME setting
//    if ((handle->fd = open(device, O_RDWR | O_NOCTTY)) < 0) {
//        fprintf(stderr, "%s: failed to open tty. ", __PRETTY_FUNCTION__);
//        return -1;
//    }
//
//    memset(term, 0, sizeof(*term));
//    term->c_cflag |= (CS8 | CREAD | CLOCAL);
//    term->c_cflag |= PARENB;
//
//    // No received data still OK
//    term->c_cc[VMIN] = (cc_t) 0;
//
//    // Wait at most 0.2 sec.Note that it starts after first received byte!!
//    // I.e. if CMIN>0 and there are no data we would still wait forever...
//    //
//    // The specification mentions link layer response timeout this way:
//    // The time structure of various link layer communication types is described in EN60870-5-1. The answer time
//    // between the end of a master send telegram and the beginning of the response telegram of the slave shall be
//    // between 11 bit times and (330 bit times + 50ms).
//    //
//    // Nowadays the usage of USB to serial adapter is very common, which could
//    // result in additional delay of 100 ms in worst case.
//    //
//    // For 2400Bd this means (330 + 11) / 2400 + 0.15 = 292 ms (added 11 bit periods to receive first byte).
//    // I.e. timeout of 0.3s seems appropriate for 2400Bd.
//
//    term->c_cc[VTIME] = (cc_t) 3; // Timeout in 1/10 sec
//
//    cfsetispeed(term, B2400);
//    cfsetospeed(term, B2400);
//
//#ifdef MBUS_SERIAL_DEBUG
//    printf("%s: t.c_cflag = %x\n", __PRETTY_FUNCTION__, term->c_cflag);
//    printf("%s: t.c_oflag = %x\n", __PRETTY_FUNCTION__, term->c_oflag);
//    printf("%s: t.c_iflag = %x\n", __PRETTY_FUNCTION__, term->c_iflag);
//    printf("%s: t.c_lflag = %x\n", __PRETTY_FUNCTION__, term->c_lflag);
//#endif
//
//    tcsetattr(handle->fd, TCSANOW, term);

    return 0;
}

//------------------------------------------------------------------------------
// Set baud rate for serial connection
//------------------------------------------------------------------------------
int mbus_serial_set_baudrate(mbus_handle *handle, long baudrate) {
    speed_t speed;
    mbus_serial_data *serial_data;

    if (handle == NULL)
        return -1;

    //serial_data = (mbus_serial_data *) handle->auxdata;

    ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_1, baudrate));

//    if (serial_data == NULL)
//        return -1;
//
//    switch (baudrate) {
//        case 300:
//            speed = B300;
//            serial_data->t.c_cc[VTIME] = (cc_t) 13; // Timeout in 1/10 sec
//            break;
//
//        case 600:
//            speed = B600;
//            serial_data->t.c_cc[VTIME] = (cc_t) 8;  // Timeout in 1/10 sec
//            break;
//
//        case 1200:
//            speed = B1200;
//            serial_data->t.c_cc[VTIME] = (cc_t) 5;  // Timeout in 1/10 sec
//            break;
//
//        case 2400:
//            speed = B2400;
//            serial_data->t.c_cc[VTIME] = (cc_t) 3;  // Timeout in 1/10 sec
//            break;
//
//        case 4800:
//            speed = B4800;
//            serial_data->t.c_cc[VTIME] = (cc_t) 3;  // Timeout in 1/10 sec
//            break;
//
//        case 9600:
//            speed = B9600;
//            serial_data->t.c_cc[VTIME] = (cc_t) 2;  // Timeout in 1/10 sec
//            break;
//
//        case 19200:
//            speed = B19200;
//            serial_data->t.c_cc[VTIME] = (cc_t) 2;  // Timeout in 1/10 sec
//            break;
//
//        case 38400:
//            speed = B38400;
//            serial_data->t.c_cc[VTIME] = (cc_t) 2;  // Timeout in 1/10 sec
//            break;
//
//        default:
//            return -1; // unsupported baudrate
//    }
//
//    // Set input baud rate
//    if (cfsetispeed(&(serial_data->t), speed) != 0) {
//        return -1;
//    }
//
//    // Set output baud rate
//    if (cfsetospeed(&(serial_data->t), speed) != 0) {
//        return -1;
//    }
//
//    // Change baud rate immediately
//    if (tcsetattr(handle->fd, TCSANOW, &(serial_data->t)) != 0) {
//        return -1;
//    }

    return 0;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int mbus_serial_disconnect(mbus_handle *handle) {
    if (handle == NULL) {
        return -1;
    }

//    if (handle->fd < 0) {
//        return -1;
//    }


    // close(handle->fd);
//    handle->fd = -1;

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

    // Make sure serial connection is open

    //if (uart_is_driver_installed(UART_NUM_1)) {
    //    fprintf(stderr, "uart is not installed!\n");
    //    return -1;
    //}

//
//    // Make sure serial connection is open
//    if (isatty(handle->fd) == 0) {
//        return -1;
//    }

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


//    if ((ret = write(handle->fd, buff, len)) == len) {
//        //
//        // call the send event function, if the callback function is registered
//        //
//        if (handle->send_event)
//            handle->send_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);
//    } else {
//        fprintf(stderr, "%s: Failed to write frame to socket (ret = %d: %s)\n", __PRETTY_FUNCTION__, ret,
//                strerror(errno));
//        return -1;
//    }

    //
    // wait until complete frame has been transmitted
    //
    uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(100));
    //tcdrain(handle->fd);

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

    // Make sure serial connection is open
    //if (isatty(handle->fd) == 0) {
//    if (uart_is_driver_installed(UART_NUM_1)) {
//        fprintf(stderr, "%s: Serial connection is not available.\n", __PRETTY_FUNCTION__);
//        return MBUS_RECV_RESULT_ERROR;
//    }

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
