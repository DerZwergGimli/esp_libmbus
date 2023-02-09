//
//
//
#include <stdio.h>
#include <string.h>
#include "mbus-protocol.h"
#include "mbus-json.h"
#include "mqtt_manager.h"

static long long int mbus_data_variable_header_mqtt(mbus_data_variable_header *header) {
    char str_encoded[768];
    size_t len = 0;
    long long int device_id = 0;
    if (header) {

        device_id = mbus_data_bcd_decode_hex(header->id_bcd, 4);

        mqtt_message_t message;
        char base_topic[20];
        sprintf(base_topic, "%llX/", mbus_data_bcd_decode_hex(header->id_bcd, 4));
        char buffer[800];

        //ID
        strcpy(message.topic, base_topic);
        strcat(message.topic, "id");
        sprintf(buffer, "%llX", mbus_data_bcd_decode_hex(header->id_bcd, 4));
        strcpy(message.message, buffer);
        send_message_async(message);

        //manufacturer
        strcpy(message.topic, base_topic);
        strcat(message.topic, "manufacturer");
        sprintf(buffer, "%s", mbus_decode_manufacturer(header->manufacturer[0], header->manufacturer[1]));
        strcpy(message.message, buffer);
        send_message_async(message);

        //version
        strcpy(message.topic, base_topic);
        strcat(message.topic, "version");
        sprintf(buffer, "%d", header->version);
        strcpy(message.message, buffer);
        send_message_async(message);

        //product_name
        strcpy(message.topic, base_topic);
        mbus_str_json_encode(str_encoded, mbus_data_product_name(header), sizeof(str_encoded));
        strcat(message.topic, "product_name");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);


        //medium
        strcpy(message.topic, base_topic);
        mbus_str_json_encode(str_encoded, mbus_data_variable_medium_lookup(header->medium), sizeof(str_encoded));
        strcat(message.topic, "medium");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);

        //access_number
        strcpy(message.topic, base_topic);
        strcat(message.topic, "access_number");
        sprintf(buffer, "%d", header->access_no);
        strcpy(message.message, buffer);
        send_message_async(message);

        //status
        strcpy(message.topic, base_topic);
        strcat(message.topic, "status");
        sprintf(buffer, "%d", header->status);
        strcpy(message.message, buffer);
        send_message_async(message);

        //signature
        strcpy(message.topic, base_topic);
        strcat(message.topic, "signature");
        sprintf(buffer, "%.2X%.2X", header->signature[1], header->signature[0]);
        strcpy(message.message, buffer);
        send_message_async(message);
        return device_id;
    }
    return 0;
}


static void mbus_data_variable_record_mqtt(mbus_data_record *record, int record_cnt, int frame_cnt,
                                           mbus_data_variable_header *header, long long int device_id) {
    static char buff[8192];
    char str_encoded[768];

    struct tm *timeinfo;
    char timestamp[22];
    long tariff;

    if (record) {
        mqtt_message_t message;
        char base_topic[20];
        sprintf(base_topic, "%llX/slave_data/%d/", mbus_data_bcd_decode_hex(header->id_bcd, 4), record_cnt);
        char buffer[800];


        if (frame_cnt >= 0) {
            //id
            strcpy(message.topic, base_topic);
            strcat(message.topic, "id");
            sprintf(buffer, "%d", record_cnt);
            strcpy(message.message, buffer);
            send_message_async(message);
            //frame
            strcpy(message.topic, base_topic);
            strcat(message.topic, "frame");
            sprintf(buffer, "%d", frame_cnt);
            strcpy(message.message, buffer);
            send_message_async(message);

        } else {
            //id
            strcpy(message.topic, base_topic);
            strcat(message.topic, "id");
            sprintf(buffer, "%d", record_cnt);
            strcpy(message.message, buffer);
            send_message_async(message);

        }

        if (record->drh.dib.dif == MBUS_DIB_DIF_MANUFACTURER_SPECIFIC) // MBUS_DIB_DIF_VENDOR_SPECIFIC
        {
            //function
            strcpy(message.topic, base_topic);
            strcat(message.topic, "function");
            sprintf(buffer, "%s", "Manufacturer specific");
            strcpy(message.message, buffer);
            send_message_async(message);

        } else if (record->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW) {
            //function
            strcpy(message.topic, base_topic);
            strcat(message.topic, "function");
            sprintf(buffer, "%s", "More records follow");
            strcpy(message.message, buffer);
            send_message_async(message);

        } else {
            //function
            mbus_str_json_encode(str_encoded, mbus_data_record_function(record), sizeof(str_encoded));
            strcpy(message.topic, base_topic);
            strcat(message.topic, "function");
            sprintf(buffer, "%s", str_encoded);
            strcpy(message.message, buffer);
            send_message_async(message);

            //storage_number
            strcpy(message.topic, base_topic);
            strcat(message.topic, "storage_number");
            sprintf(buffer, "%ld", mbus_data_record_storage_number(record));
            strcpy(message.message, buffer);
            send_message_async(message);


            if ((tariff = mbus_data_record_tariff(record)) >= 0) {
                //tariff
                strcpy(message.topic, base_topic);
                strcat(message.topic, "tariff");
                sprintf(buffer, "%ld", tariff);
                strcpy(message.message, buffer);
                send_message_async(message);
                //device
                strcpy(message.topic, base_topic);
                strcat(message.topic, "device");
                sprintf(buffer, "%d", mbus_data_record_device(record));
                strcpy(message.message, buffer);
                send_message_async(message);

            }

            mbus_str_json_encode(str_encoded, mbus_data_record_unit(record), sizeof(str_encoded));
            //unit
            strcpy(message.topic, base_topic);
            strcat(message.topic, "unit");
            sprintf(buffer, "%s", str_encoded);
            strcpy(message.message, buffer);
            send_message_async(message);

        }

        mbus_str_json_encode(str_encoded, mbus_data_record_value(record), sizeof(str_encoded));
        //value
        strcpy(message.topic, base_topic);
        strcat(message.topic, "value");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);


        if (record->timestamp > 0) {
            timeinfo = gmtime(&(record->timestamp));
            strftime(timestamp, 21, "%Y-%m-%dT%H:%M:%SZ", timeinfo);
            //timestamp
            strcpy(message.topic, base_topic);
            strcat(message.topic, "timestamp");
            sprintf(buffer, "%s", timestamp);
            strcpy(message.message, buffer);
            send_message_async(message);

        }


    }


}

static void
mbus_data_fixed_mqtt(mbus_data_fixed *data) {
    char *buff = NULL;
    char str_encoded[256];
    size_t len = 0, buff_size = 8192;
    int val;

    if (data) {
        mqtt_message_t message;
        char base_topic[20];
        sprintf(base_topic, "%llX/", mbus_data_bcd_decode_hex(data->id_bcd, 4));
        char buffer[800];

        //id
        strcpy(message.topic, base_topic);
        strcat(message.topic, "id");
        sprintf(buffer, "%lld", mbus_data_bcd_decode_hex(data->id_bcd, 4));
        strcpy(message.message, buffer);
        send_message_async(message);

        strcpy(message.topic, base_topic);
        mbus_str_json_encode(str_encoded, mbus_data_fixed_medium(data), sizeof(str_encoded));
        strcat(message.topic, "medium");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);

        //access_number
        strcpy(message.topic, base_topic);
        strcat(message.topic, "access_number");
        sprintf(buffer, "%d", data->tx_cnt);
        strcpy(message.message, buffer);
        send_message_async(message);

        //status
        strcpy(message.topic, base_topic);
        strcat(message.topic, "status");
        sprintf(buffer, "%d", data->status);
        strcpy(message.message, buffer);
        send_message_async(message);

        sprintf(base_topic, "%llX/salve_data/0", mbus_data_bcd_decode_hex(data->id_bcd, 4));

        //function
        mbus_str_json_encode(str_encoded, mbus_data_fixed_function(data->status), sizeof(str_encoded));
        strcpy(message.topic, base_topic);
        strcat(message.topic, "function");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);

        //unit
        mbus_str_json_encode(str_encoded, mbus_data_fixed_unit(data->cnt1_type), sizeof(str_encoded));
        strcpy(message.topic, base_topic);
        strcat(message.topic, "unit");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);

        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD) {
            //value
            strcpy(message.topic, base_topic);
            strcat(message.topic, "value");
            sprintf(buffer, "%llX", mbus_data_bcd_decode_hex(data->cnt1_val, 4));
            strcpy(message.message, buffer);
            send_message_async(message);
        } else {
            //value
            mbus_data_int_decode(data->cnt1_val, 4, &val);
            strcpy(message.topic, base_topic);
            strcat(message.topic, "value");
            sprintf(buffer, "%d", val);
            strcpy(message.message, buffer);
            send_message_async(message);
        }


        sprintf(base_topic, "%llX/salve_data/1", mbus_data_bcd_decode_hex(data->id_bcd, 4));
        //function
        mbus_str_json_encode(str_encoded, mbus_data_fixed_function(data->status), sizeof(str_encoded));
        strcpy(message.topic, base_topic);
        strcat(message.topic, "function");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);

        //unit
        mbus_str_json_encode(str_encoded, mbus_data_fixed_unit(data->cnt2_type), sizeof(str_encoded));
        strcpy(message.topic, base_topic);
        strcat(message.topic, "unit");
        sprintf(buffer, "%s", str_encoded);
        strcpy(message.message, buffer);
        send_message_async(message);

        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD) {
            //value
            strcpy(message.topic, base_topic);
            strcat(message.topic, "value");
            sprintf(buffer, "%llX", mbus_data_bcd_decode_hex(data->cnt2_val, 4));
            strcpy(message.message, buffer);
            send_message_async(message);
        } else {
            //value
            mbus_data_int_decode(data->cnt2_val, 4, &val);
            strcpy(message.topic, base_topic);
            strcat(message.topic, "value");
            sprintf(buffer, "%d", val);
            strcpy(message.message, buffer);
            send_message_async(message);
        }
    }
}


static void
mbus_data_variable_mqtt(mbus_data_variable *data) {
    mbus_data_record *record;
    char *buff = NULL, *new_buff;
    size_t len = 0, buff_size = 8192;
    int i;

    if (data) {
        long long int device_id = 0;

        device_id = mbus_data_variable_header_mqtt(&(data->header));
        for (record = data->record, i = 0; record; record = record->next, i++) {
            mbus_data_variable_record_mqtt(record, i, -1, &(data->header), device_id);

        }


    }


}


void
mbus_frame_data_mqtt(mbus_frame_data *data) {

    if (data) {
        if (data->type == MBUS_DATA_TYPE_ERROR) {
            //mbus_data_error_mqtt(data->error);
        }

        if (data->type == MBUS_DATA_TYPE_FIXED) {
            mbus_data_fixed_mqtt(&(data->data_fix));
        }

        if (data->type == MBUS_DATA_TYPE_VARIABLE) {
            mbus_data_variable_mqtt(&(data->data_var));
        }
    }


}