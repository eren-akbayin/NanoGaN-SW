cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME NanoGaN-SW)

# Software version
set(SW_VERSION "0.1")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

include(FetchContent)

set(SCRUTINY_ENABLE_DATALOGGING 1)
set(SCRUTINY_SUPPORT_64BITS 0)

FetchContent_Declare(
  scrutiny-embedded
  GIT_REPOSITORY https://github.com/scrutinydebugger/scrutiny-embedded.git
  GIT_TAG        v0.5.0
)
FetchContent_MakeAvailable(scrutiny-embedded)

# -----------------------------------------------------------------------------
# TinyUSB
# -----------------------------------------------------------------------------
set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/tinyusb)

# Include TinyUSB's own CMake logic and attach sources/includes to the target
include(${TINYUSB_DIR}/CMakeLists.txt)
tinyusb_target_add(${CMAKE_PROJECT_NAME})
# -----------------------------------------------------------------------------

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here

    ${CMAKE_SOURCE_DIR}/Core/Src/tusb_descriptors.c
    ${CMAKE_SOURCE_DIR}/Core/Src/core_tasks.c
    ${CMAKE_SOURCE_DIR}/Core/Src/scrutiny_integration.c

    # TinyUSB STM32H7 DWC2 hardware driver (device mode)
    # Switch dcd_dwc2.c -> hcd_dwc2.c if you need host mode
    ${TINYUSB_DIR}/portable/synopsys/dwc2/dcd_dwc2.c
    ${TINYUSB_DIR}/portable/synopsys/dwc2/dwc2_common.c

    # Add your USB descriptors file once created, e.g.:
    # Core/Src/usb_descriptors.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${scrutiny-embedded_SOURCE_DIR}/cwrapper
    # Directory containing your tusb_config.h (e.g. Core/Inc/tusb_config.h)
    Core/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    SW_VERSION="${SW_VERSION}"

    # TinyUSB MCU and OS selection
    CFG_TUSB_MCU=OPT_MCU_STM32H7
    CFG_TUSB_OS=OPT_OS_FREERTOS         # Change to OPT_OS_FREERTOS if using FreeRTOS
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    scrutiny-cwrapper
)

add_compile_options(-Os -Wall -Wextra -Werror -gdwarf-4)

# ─────────────────────────────────────────────
# Scrutiny post-build: generate and install SFD
# ─────────────────────────────────────────────

find_program(SCRUTINY_CLI scrutiny REQUIRED)

set(SFD_WORKDIR "${CMAKE_BINARY_DIR}/_scrutiny_${PROJECT_NAME}_sfd_workfolder")
set(SFD_OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.sfd")
set(TAGGED_ELF "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_tagged.elf")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${SFD_WORKDIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SFD_WORKDIR}"
    COMMAND "${SCRUTINY_CLI}" get-firmware-id "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf" --output "${SFD_WORKDIR}"
    COMMAND "${SCRUTINY_CLI}" tag-firmware-id "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf" "${TAGGED_ELF}"
    COMMAND "${SCRUTINY_CLI}" elf2varmap "${TAGGED_ELF}" --output "${SFD_WORKDIR}" --loglevel error --cppfilt "c++filt"
    COMMAND "${SCRUTINY_CLI}" make-metadata --output "${SFD_WORKDIR}" --project-name "${PROJECT_NAME}" --author "Eren Akbayin" --version "${SW_VERSION}"
    COMMAND "${SCRUTINY_CLI}" noop "${SFD_WORKDIR}" --file
    COMMAND "${SCRUTINY_CLI}" make-sfd "${SFD_WORKDIR}" "${SFD_OUTPUT}" --install
    COMMENT "Generating ${PROJECT_NAME}.sfd"
)