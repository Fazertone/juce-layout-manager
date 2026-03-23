cmake_minimum_required(VERSION 3.22)

project(LayoutSandbox VERSION 0.0.1)

# When consumed via FetchContent/add_subdirectory, skip the sandbox app —
# consumers only need the layout_manager JUCE module in the layout_manager/ dir.
if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

add_definitions(-w)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# 1. SETUP JUCE
set(JUCE_DIR "" CACHE PATH "Path to the JUCE source directory")

if(NOT JUCE_DIR OR NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    # Try to grab from environment variable if cache is empty
    if (DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR $ENV{JUCE_DIR})
    else()
        message(FATAL_ERROR "JUCE directory not found. Please set JUCE_DIR in .vscode/settings.json or your environment.")
    endif()
endif()

message(STATUS "Found JUCE at: ${JUCE_DIR}")
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# 2. ADD YOUR MODULE
# This looks for the folder "layout_manager" in the same directory as this script.
# It parses "layout_manager/layout_manager.h" to find the ID.
juce_add_module("layout_manager")
juce_add_module("modules/melatonin_blur")

# 3. CREATE THE SANDBOX APP
juce_add_gui_app(LayoutSandbox
    PRODUCT_NAME "LayoutSandbox"
    BUNDLE_ID "com.fazertone.LayoutSandbox"
    VERSION "0.0.1"
)

# --- Binary Data (Assets, Models, Fonts) ---
juce_add_binary_data(LayoutSandboxAssets
    SOURCES
        Tests/data/image_buttons/save_preset_img_btn.png
)

juce_generate_juce_header(LayoutSandbox)

# 4. LINK SOURCES
# Assumes your directory structure is:
# Root/Tests/SandboxApp.cpp
target_sources(LayoutSandbox
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/SandboxApp.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestComponents.cpp"
)

# 5. LINK LIBRARIES
target_link_libraries(LayoutSandbox
    PRIVATE
        LayoutSandboxAssets
        juce::juce_gui_basics
        juce::juce_audio_processors
        layout_manager
        melatonin_blur
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Applying Clang specific optimization flags.")

    # --- Common Compile Options ---
    # Apply optimization levels for Debug and Release builds on all platforms.
    target_compile_options(LayoutSandbox PUBLIC
        $<$<CONFIG:Debug>:-O0 -g -fstandalone-debug>
        $<$<CONFIG:Release>:-O3>
    )
else()
    message(STATUS "Not applying Clang specific optimization flags. Compiler ID: ${CMAKE_CXX_COMPILER_ID} and System Name: ${CMAKE_SYSTEM_NAME}")
endif()
