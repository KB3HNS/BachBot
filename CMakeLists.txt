cmake_minimum_required(VERSION 3.13.0)

project(BachBot VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_TOOLCHAIN_FILE "D:/devel/vcpkg/scripts/buildsystems/vcpkg.cmake")
# set(VCPKG_TARGET_TRIPLET "x64-windows")
set(CMAKE_CL_64 True)

find_package(fmt REQUIRED)
find_package(wxWidgets COMPONENTS core base xml REQUIRED)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_package(RtMidi REQUIRED)
endif()

add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    FMT_DEPRECATED_INCLUDE_XCHAR
)

include(${wxWidgets_USE_FILE})
include(CompilerWarnings.cmake)

set(SRCS
    BachBot/bitmap_painter.cpp
    BachBot/label_animator.cpp
    BachBot/main.cpp
    BachBot/main_window.cpp
    BachBot/midi_note_tracker.cpp
    BachBot/organ_midi_event.cpp
    BachBot/player_thread.cpp
    BachBot/player_window.cpp
    BachBot/playlist_entry_control.cpp
    BachBot/playlist_loader.cpp
    BachBot/play_list.cpp
    BachBot/syndyne_importer.cpp
    BachBot/thread_loader.cpp
)

set(INCLUDE_DIRS
    ${wxWidgets_INCLUDE_DIRS}
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_subdirectory(midifile)
    set(SRCS
        ${SRCS}
        BachBot/rt_timer_win.cpp
    )
    set(INCLUDE_DIRS
        ${INCLUDE_DIRS}
        midifile/include
    )
else()
    set(SRCS
        ${SRCS}
        BachBot/rt_timer_posix.cpp
    )
endif()

add_executable(${PROJECT_NAME} WIN32
    ${SRCS}
)

set_project_warnings(${PROJECT_NAME} False)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIRS}
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        fmt::fmt
        ${wxWidgets_LIBRARIES}
        RtMidi::rtmidi
        midifile
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        fmt::fmt
        ${wxWidgets_LIBRARIES}
        rtmidi
        midifile
    )
endif()
