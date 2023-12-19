idf_component_register(
        SRC_DIRS screens
        SRC_DIRS components
        SRC_DIRS images
        SRC_DIRS fonts
        INCLUDE_DIRS "includes"
        REQUIRES lvgl WIFI sd_card
        )
