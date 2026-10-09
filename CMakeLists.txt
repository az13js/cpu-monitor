cmake_minimum_required(VERSION 3.10)
project(cpu-monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# — 浮窗程序 —
add_executable(cpu-monitor WIN32
    src/main.cpp
    src/cpu_monitor.cpp
    src/disk_monitor.cpp
    src/counter_math.cpp
    src/disk_math.cpp
    src/disk_text.cpp
    src/usage_color.cpp
    src/overlay_layout.cpp
    src/overlay_window.cpp
    src/config_parser.cpp
    src/hotkey_config.cpp
)

# — 单元测试（assert 风格，无第三方框架）—
add_executable(cpu-monitor-tests
    tests/test_main.cpp
    tests/test_counter_math.cpp
    tests/test_disk_math.cpp
    tests/test_usage_color.cpp
    tests/test_overlay_layout.cpp
    tests/test_config_parser.cpp
    tests/test_disk_text.cpp
    tests/test_cpu_monitor.cpp
    tests/test_disk_monitor.cpp
    src/cpu_monitor.cpp
    src/disk_monitor.cpp
    src/counter_math.cpp
    src/disk_math.cpp
    src/disk_text.cpp
    src/usage_color.cpp
    src/overlay_layout.cpp
    src/config_parser.cpp
    src/hotkey_config.cpp
)

# — 离屏渲染预览工具（人工检查布局，不参与发布）—
add_executable(cpu-monitor-preview
    tools/preview_main.cpp
    src/cpu_monitor.cpp
    src/disk_monitor.cpp
    src/counter_math.cpp
    src/disk_math.cpp
    src/disk_text.cpp
    src/usage_color.cpp
    src/overlay_layout.cpp
    src/overlay_window.cpp
    src/config_parser.cpp
    src/hotkey_config.cpp
)

foreach(target cpu-monitor cpu-monitor-tests cpu-monitor-preview)
    target_include_directories(${target} PRIVATE src)
    # PDH：读取 \PhysicalDisk(_Total)\% Idle Time 等英文计数器
    target_link_libraries(${target} PRIVATE pdh user32 gdi32)
endforeach()

if(MSVC)
    foreach(target cpu-monitor cpu-monitor-tests cpu-monitor-preview)
        target_compile_options(${target} PRIVATE /utf-8 /W4)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    endforeach()
endif()

enable_testing()
add_test(NAME unit-tests COMMAND cpu-monitor-tests)
