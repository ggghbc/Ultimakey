cmake_minimum_required(VERSION 3.20)
project(Ultimakey LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
    src/main.cpp
    src/engine.cpp
    src/hook.cpp
    src/buffer.cpp
    src/detector.cpp
    src/keymap.cpp
    src/layout_mgr.cpp
    src/antiresonance.cpp
    src/replacer.cpp
    src/secure_input.cpp
    src/selection.cpp
    src/snippets.cpp
    src/sound.cpp
    src/settings.cpp
    src/autostart.cpp
    src/logger.cpp
    src/tray.cpp
    src/ui_settings.cpp
    src/resources.rc
)

add_executable(Ultimakey WIN32 ${SOURCES})

target_include_directories(Ultimakey PRIVATE
    src
)

# Required Windows system libraries
target_link_libraries(Ultimakey PRIVATE
    user32
    shell32
    comctl32
    ole32
    oleaut32
    winmm
    advapi32
)

# Compiler optimization settings (Demoscene / Lean footprint)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Ultimakey PRIVATE
        -O3
        -flto
        -Wall
        -Wextra
        -Wno-unused-parameter
        -finput-charset=utf-8
        -fexec-charset=utf-8
        -ffunction-sections
        -fdata-sections
    )
    target_link_options(Ultimakey PRIVATE
        -mwindows
        -flto
        -static
        -static-libgcc
        -static-libstdc++
        -s
        -Wl,--gc-sections
    )
elseif(MSVC)
    target_compile_options(Ultimakey PRIVATE
        /O2
        /Oi
        /GL
        /Gy
        /W4
        /permissive-
    )
    target_link_options(Ultimakey PRIVATE
        /LTCG
        /OPT:REF
        /OPT:ICF
        /SUBSYSTEM:WINDOWS
    )
endif()
