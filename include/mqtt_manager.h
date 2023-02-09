//
// .
//

#ifndef LOGME_INTERFACE_MQTT_MANAGER_H
#define LOGME_INTERFACE_MQTT_MANAGER_H
typedef struct mqtt_message_t {
    char topic[50];
    char message[800];
} mqtt_message_t;

void mbus_frame_data_mqtt(mbus_frame_data *data);

void send_message_async(mqtt_message_t mqtt_message);

#endif //LOGME_INTERFACE_MQTT_MANAGER_H
