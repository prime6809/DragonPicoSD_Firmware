cmake_minimum_required(VERSION 3.12)

# Set this to pico if compiling for RP2040, pico 2 if compiling for RP2350
set(PICO_BOARD pico2)

# Copy binaries to ram
set(PICO_COPY_TO_RAM 1)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(dragonpicosd_project)

pico_sdk_init()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter" )

include_directories(. mmc hw util include)

#
# Dragon Build
#

add_executable(dragonpicosd
  DragonPicoSD.c
  hw/eeprom.c
  hw/mcp23017.c
  hw/pcf2129rtc.c
  util/hexout.c
  util/i2c_scan.c
  mmc/diskio.c
  mmc/ff.c
  mmc/mmcio.c
  mmc/mmc2core.c
  mmc/mmc2wfn.c
  mmc/wildcard.c
  )

pico_generate_pio_header(dragonpicosd ${CMAKE_CURRENT_LIST_DIR}/writebus_6809.pio)

pico_generate_pio_header(dragonpicosd ${CMAKE_CURRENT_LIST_DIR}/readbus_6809.pio)

target_compile_definitions(dragonpicosd PUBLIC -DPLATFORM=PLATFORM_DRAGON -DINCLUDE_SDDOS)

target_link_libraries(dragonpicosd PRIVATE
  pico_multicore
  pico_stdlib
  hardware_pio
  hardware_i2c
  hardware_spi
  )

pico_add_extra_outputs(dragonpicosd)

pico_enable_stdio_uart(dragonpicosd 1)
