idf_component_register(
    SRCS
        "display.c"
        "display_canvas.c"
        "display_fonts.c"
        "display_renderer.c"
        "display_qr.c"
        "display_driver.c"
        "display_task.c"
        "display_controller.c"
        "display_layout_templates.c"
        "display_geometry_test.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos
    PRIV_REQUIRES i2c_bus qr_encoder setup_url
)
