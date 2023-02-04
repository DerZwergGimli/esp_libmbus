//
// Header for creating a JSON representation of a frame
//

#ifndef _MBUS_JSON_H
#define _MBUS_JSON_H

#include "mbus-protocol.h"

char *mbus_frame_json(mbus_frame *frame);

char *mbus_data_variable_header_json(mbus_data_variable_header *header);

char *mbus_data_variable_record_json(mbus_data_record *record, int record_cnt, int frame_cnt,
                                     mbus_data_variable_header *header);


char *mbus_data_error_json(int error);

char *mbus_data_fixed_json(mbus_data_fixed *data);

char *mbus_data_variable_json(mbus_data_variable *data);

int mbus_str_json_encode(unsigned char *dst, const unsigned char *src, size_t max_len);

char *mbus_frame_data_json(mbus_frame_data *data);

#endif