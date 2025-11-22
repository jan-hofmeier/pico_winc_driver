# This is a copy of the pico_sdk_import.cmake file from the Raspberry Pi Pico SDK.
# It is used to locate the Pico SDK, which is a required dependency.
# You can override the location of the Pico SDK by setting the PICO_SDK_PATH environment variable.

if (NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
    if (NOT PICO_SDK_PATH)
        message(FATAL_ERROR "PICO_SDK_PATH not set")
    endif()
endif()

if (NOT EXISTS ${PICO_SDK_PATH}/pico_sdk_init.cmake)
    message(FATAL_ERROR "PICO_SDK_PATH does not point to a valid Pico SDK")
endif()

set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)

include(${PICO_SDK_INIT_CMAKE_FILE})
