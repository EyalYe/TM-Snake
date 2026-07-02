# app_snake — a self-registering snake game app.
# WHOLE_ARCHIVE keeps the registration constructor from being dropped by the linker.
idf_component_register(
    SRCS "app_snake.c"
    REQUIRES taskmaster_core esp_timer esp_hw_support
    WHOLE_ARCHIVE
)
