cmake_minimum_required(VERSION 3.16)
idf_component_register(SRCS
        "mbus.c"
        "mbus-protocol.c"
        "mbus-protocol-aux.c"
        "mbus-tcp.c"
        "mbus-serial.c"
        "mbus-json.c"
        INCLUDE_DIRS "include"
        PRIV_REQUIRES driver freertos)


set(ESP_MBUS_FLAGS
        -Wno-pointer-sign
        -Wno-maybe-uninitialized)


set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-pointer-sign)
#target_compile_definitions(${COMPONENT_TARGET} PUBLIC)
#target_compile_options(${COMPONENT_LIB} PRIVATE --warn-pointer-sign --warn-char-subscripts)