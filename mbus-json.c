//
// Implementation for creating a JSON representation of a frame
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mbus-json.h"

char *mbus_frame_json(mbus_frame *frame) {
    mbus_frame_data frame_data;
    mbus_frame *iter;
    mbus_data_record *record;
    char *buff = NULL, *new_buff;

    size_t len = 0, buff_size = 8192;
    int record_cnt = 0, frame_cnt;

    if (frame) {
        memset((void *) &frame_data, 0, sizeof(mbus_frame_data));

        if (mbus_frame_data_parse(frame, &frame_data) == -1) {
            mbus_error_str_set("M-bus data parse error.");
            return NULL;
        }

        if (frame_data.type == MBUS_DATA_TYPE_ERROR) {
            //
            // generate XML for error
            //
            return mbus_data_error_json(frame_data.error);
        }

        if (frame_data.type == MBUS_DATA_TYPE_FIXED) {
            //
            // generate XML for fixed data
            //
            return mbus_data_fixed_json(&(frame_data.data_fix));
        }

        if (frame_data.type == MBUS_DATA_TYPE_VARIABLE) {
            //
            // generate XML for a sequence of variable data frames
            //

            buff = (char *) malloc(buff_size);

            if (buff == NULL) {
                mbus_data_record_free(frame_data.data_var.record);
                return NULL;
            }

            // include frame counter in XML output if more than one frame
            // is available (frame_cnt = -1 => not included in output)
            frame_cnt = (frame->next == NULL) ? -1 : 0;


            len += snprintf(&buff[len], buff_size - len, "{\n");

            // only print the header info for the first frame (should be
            // the same for each frame in a sequence of a multi-telegram
            // transfer.
            len += snprintf(&buff[len], buff_size - len, "%s",
                            mbus_data_variable_header_json(&(frame_data.data_var.header)));

            // loop through all records in the current frame, using a global
            // record count as record ID in the XML output
            len += snprintf(&buff[len], buff_size - len, ",\"slave_data\": [\n");
            for (record = frame_data.data_var.record; record; record = record->next, record_cnt++) {

                if ((buff_size - len) < 1024) {
                    buff_size *= 2;
                    new_buff = (char *) realloc(buff, buff_size);

                    if (new_buff == NULL) {
                        free(buff);
                        mbus_data_record_free(frame_data.data_var.record);
                        return NULL;
                    }

                    buff = new_buff;
                }

                len += snprintf(&buff[len], buff_size - len, "%s",
                                mbus_data_variable_record_json(record, record_cnt, frame_cnt,
                                                               &(frame_data.data_var.header)));

                len += snprintf(&buff[len], buff_size - len, ",");

            }

            // free all records in the list
            if (frame_data.data_var.record) {
                mbus_data_record_free(frame_data.data_var.record);
            }

            frame_cnt++;

            for (iter = frame->next; iter; iter = iter->next, frame_cnt++) {
                if (mbus_frame_data_parse(iter, &frame_data) == -1) {
                    mbus_error_str_set("M-bus variable data parse error.");
                    return NULL;
                }

                // loop through all records in the current frame, using a global
                // record count as record ID in the XML output
                for (record = frame_data.data_var.record; record; record = record->next, record_cnt++) {
                    if ((buff_size - len) < 1024) {
                        buff_size *= 2;
                        new_buff = (char *) realloc(buff, buff_size);

                        if (new_buff == NULL) {
                            free(buff);
                            mbus_data_record_free(frame_data.data_var.record);
                            return NULL;
                        }

                        buff = new_buff;
                    }

                    len += snprintf(&buff[len], buff_size - len, "%s",
                                    mbus_data_variable_record_json(record, record_cnt, frame_cnt,
                                                                   &(frame_data.data_var.header)));

                    len += snprintf(&buff[len], buff_size - len, ",");

                }

                // free all records in the list
                if (frame_data.data_var.record) {
                    mbus_data_record_free(frame_data.data_var.record);
                }

            }
            //Remove last , from string
            buff[len - 1] = ' ';
            len += snprintf(&buff[len], buff_size - len, "]}");

            return buff;
        }
    }

    return NULL;
}

char *mbus_data_variable_header_json(mbus_data_variable_header *header) {
    static char buff[8192];
    char str_encoded[768];
    size_t len = 0;

    if (header) {
        len += snprintf(&buff[len], sizeof(buff) - len, "\"salve_info\": {\n");

        len += snprintf(&buff[len], sizeof(buff) - len, "\"id:\": %llX,\n",
                        mbus_data_bcd_decode_hex(header->id_bcd, 4));
        len += snprintf(&buff[len], sizeof(buff) - len, "\"manufacturer:\": \"%s\",\n",
                        mbus_decode_manufacturer(header->manufacturer[0], header->manufacturer[1]));
        len += snprintf(&buff[len], sizeof(buff) - len, "\"version:\": %d,\n", header->version);

        mbus_str_json_encode(str_encoded, mbus_data_product_name(header), sizeof(str_encoded));

        len += snprintf(&buff[len], sizeof(buff) - len, "\"product_name:\": \"%s\",\n", str_encoded);

        mbus_str_json_encode(str_encoded, mbus_data_variable_medium_lookup(header->medium), sizeof(str_encoded));

        len += snprintf(&buff[len], sizeof(buff) - len, "\"medium:\": \"%s\",\n", str_encoded);
        len += snprintf(&buff[len], sizeof(buff) - len, "\"access_number:\": %d,\n", header->access_no);
        len += snprintf(&buff[len], sizeof(buff) - len, "\"status:\": \"%.2X\",\n", header->status);
        len += snprintf(&buff[len], sizeof(buff) - len, "\"signature:\": \"%.2X%.2X\"\n",
                        header->signature[1], header->signature[0]);

        len += snprintf(&buff[len], sizeof(buff) - len, "}\n\n");

        return buff;
    }

    return "";
}

char *mbus_data_variable_record_json(mbus_data_record *record, int record_cnt, int frame_cnt,
                                     mbus_data_variable_header *header) {
    static char buff[8192];
    char str_encoded[768];
    size_t len = 0;
    struct tm *timeinfo;
    char timestamp[22];
    long tariff;

    if (record) {
        if (frame_cnt >= 0) {
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "    { \"id\":%d,\n\"frame\":%d,\n",
                            record_cnt, frame_cnt);
        } else {
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "    { \"id\": %d,\n", record_cnt);
        }

        if (record->drh.dib.dif == MBUS_DIB_DIF_MANUFACTURER_SPECIFIC) // MBUS_DIB_DIF_VENDOR_SPECIFIC
        {
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "        \"function\": \"Manufacturer specific\",\n");
        } else if (record->drh.dib.dif == MBUS_DIB_DIF_MORE_RECORDS_FOLLOW) {
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "        \"function\": \"More records follow\",\n");
        } else {
            mbus_str_json_encode(str_encoded, mbus_data_record_function(record), sizeof(str_encoded));
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "        \"function\": \"%s\",\n", str_encoded);

            len += snprintf(&buff[len], sizeof(buff) - len,
                            "        \"storage_number\": %ld,\n",
                            mbus_data_record_storage_number(record));

            if ((tariff = mbus_data_record_tariff(record)) >= 0) {
                len += snprintf(&buff[len], sizeof(buff) - len, "        \"tariff\": %ld,\n",
                                tariff);
                len += snprintf(&buff[len], sizeof(buff) - len, "        \"device\": %d,\n",
                                mbus_data_record_device(record));
            }

            mbus_str_json_encode(str_encoded, mbus_data_record_unit(record), sizeof(str_encoded));
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "        \"unit\": \"%s\",\n", str_encoded);
        }

        mbus_str_json_encode(str_encoded, mbus_data_record_value(record), sizeof(str_encoded));
        len += snprintf(&buff[len], sizeof(buff) - len, "        \"value\": \"%s\",\n", str_encoded);

        if (record->timestamp > 0) {
            timeinfo = gmtime(&(record->timestamp));
            strftime(timestamp, 21, "%Y-%m-%dT%H:%M:%SZ", timeinfo);
            len += snprintf(&buff[len], sizeof(buff) - len,
                            "        \"timestamp\": \"%s\"\n", timestamp);
        }

        len += snprintf(&buff[len], sizeof(buff) - len, "    }\n\n");

        return buff;
    }

    return "";
}

char *
mbus_data_error_json(int error) {
    char *buff = NULL;
    char str_encoded[256];
    size_t len = 0, buff_size = 8192;

    buff = (char *) malloc(buff_size);

    if (buff == NULL)
        return NULL;

    len += snprintf(&buff[len], buff_size - len, MBUS_XML_PROCESSING_INSTRUCTION);
    len += snprintf(&buff[len], buff_size - len, "{\n\n");

    len += snprintf(&buff[len], buff_size - len, "    \"slave_information\":{\n");

    mbus_str_json_encode(str_encoded, mbus_data_error_lookup(error), sizeof(str_encoded));
    len += snprintf(&buff[len], buff_size - len, "        \"error\": \"%s\"\n", str_encoded);

    len += snprintf(&buff[len], buff_size - len, "    }\n\n");

    len += snprintf(&buff[len], buff_size - len, "}\n");

    return buff;
}

char *
mbus_data_fixed_json(mbus_data_fixed *data) {
    char *buff = NULL;
    char str_encoded[256];
    size_t len = 0, buff_size = 8192;
    int val;

    if (data) {
        buff = (char *) malloc(buff_size);

        if (buff == NULL)
            return NULL;

        len += snprintf(&buff[len], buff_size - len, MBUS_XML_PROCESSING_INSTRUCTION);

        len += snprintf(&buff[len], buff_size - len, "{\n\n");

        len += snprintf(&buff[len], buff_size - len, "    \"slave_information\":{\n");
        len += snprintf(&buff[len], buff_size - len, "        \"id\": %llX,\n",
                        mbus_data_bcd_decode_hex(data->id_bcd, 4));

        mbus_str_json_encode(str_encoded, mbus_data_fixed_medium(data), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        \"medium\": \"%s\",\n", str_encoded);

        len += snprintf(&buff[len], buff_size - len, "        \"access_number\": %d,\n", data->tx_cnt);
        len += snprintf(&buff[len], buff_size - len, "        \"status\": \"%.2X\",\n", data->status);
        len += snprintf(&buff[len], buff_size - len, "    },\n\n");

        len += snprintf(&buff[len], buff_size - len, "    \"salve_data\": [{ \"id\": 0,\n");

        mbus_str_json_encode(str_encoded, mbus_data_fixed_function(data->status), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        \"function\": \"%s\",\n", str_encoded);

        mbus_str_json_encode(str_encoded, mbus_data_fixed_unit(data->cnt1_type), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        \"unit\": \"%s\",\n", str_encoded);
        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD) {
            len += snprintf(&buff[len], buff_size - len, "        \"value\": \"%llX\",\n",
                            mbus_data_bcd_decode_hex(data->cnt1_val, 4));
        } else {
            mbus_data_int_decode(data->cnt1_val, 4, &val);
            len += snprintf(&buff[len], buff_size - len, "        \"value\": %d,\n", val);
        }

        len += snprintf(&buff[len], buff_size - len, "    ,}\n\n");

        len += snprintf(&buff[len], buff_size - len, "    { \"id\": 1,\n");

        mbus_str_json_encode(str_encoded, mbus_data_fixed_function(data->status), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        \"function\": \"%s\",\n", str_encoded);

        mbus_str_json_encode(str_encoded, mbus_data_fixed_unit(data->cnt2_type), sizeof(str_encoded));
        len += snprintf(&buff[len], buff_size - len, "        \"unit\": \"%s\",\n", str_encoded);
        if ((data->status & MBUS_DATA_FIXED_STATUS_FORMAT_MASK) == MBUS_DATA_FIXED_STATUS_FORMAT_BCD) {
            len += snprintf(&buff[len], buff_size - len, "        \"value\": \"%llX\",\n",
                            mbus_data_bcd_decode_hex(data->cnt2_val, 4));
        } else {
            mbus_data_int_decode(data->cnt2_val, 4, &val);
            len += snprintf(&buff[len], buff_size - len, "        \"value\": %d,\n", val);
        }

        len += snprintf(&buff[len], buff_size - len, "    }\n\n");

        len += snprintf(&buff[len], buff_size - len, "]}\n");

        return buff;
    }

    return NULL;
}

int
mbus_str_json_encode(unsigned char *dst, const unsigned char *src, size_t max_len) {
    size_t i, len;

    i = 0;
    len = 0;

    if (dst == NULL) {
        return -1;
    }

    if (src == NULL) {
        dst[len] = '\0';
        return -2;
    }

    while ((len + 6) < max_len) {
        if (src[i] == '\0') {
            break;
        }

        if (iscntrl(src[i])) {
            // convert all control chars into spaces
            dst[len++] = ' ';
        } else {
            switch (src[i]) {
                case '&':
                    len += snprintf(&dst[len], max_len - len, "&amp;");
                    break;
                case '<':
                    len += snprintf(&dst[len], max_len - len, "&lt;");
                    break;
                case '>':
                    len += snprintf(&dst[len], max_len - len, "&gt;");
                    break;
                case '"':
                    len += snprintf(&dst[len], max_len - len, "&quot;");
                    break;
                default:
                    dst[len++] = src[i];
                    break;
            }
        }

        i++;
    }

    dst[len] = '\0';
    return 0;
}