idf_component_register(
    SRCS "main.cpp" "led_rpc_service.cpp" "button_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES embed embed_core embed_extra cogitor_iot esp_driver_gpio esp_timer
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
