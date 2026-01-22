# I2C Memory Library

add_library(i2c_memory
    drivers/at24cxx_driver.c
)

target_include_directories(i2c_memory PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(i2c_memory
    pico_stdlib
    hardware_i2c
)
