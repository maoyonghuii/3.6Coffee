# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(usb_camera_lcd_display)

target_add_binary_data(usb_camera_lcd_display.elf "components/my_mqtt/mqtt_eclipseprojects_io.pem" TEXT)
