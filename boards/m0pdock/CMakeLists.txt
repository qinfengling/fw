cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_add_include_directories(.)
sdk_add_compile_definitions(-DCONFIG_BOARD_FLASH_INIT_SKIP)

target_sources(app PRIVATE sdk-usb.c flash-storage.c)
target_link_libraries(app libsunela.a -L$ENV{TOP}/obj-sdk/)
sdk_set_main_file(sdk-main.c)
sdk_set_linker_script(sunela.ld)

project(sunela)
