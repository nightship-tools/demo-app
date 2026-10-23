cmake_minimum_required(VERSION 3.20)
project(nightship-demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plain Win32 window app. Static CRT, so the exe runs from any install
# directory with nothing next to it but message.txt.
add_executable(nightship-demo WIN32 main.cpp)
set_property(TARGET nightship-demo PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(nightship-demo PRIVATE UNICODE _UNICODE NOMINMAX)
# Sources are UTF-8 - without this MSVC reads them in the ANSI code page.
target_compile_options(nightship-demo PRIVATE /utf-8)
target_link_libraries(nightship-demo PRIVATE msimg32 shell32)

# The text the app shows sits beside the exe.
add_custom_command(TARGET nightship-demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/message.txt
            $<TARGET_FILE_DIR:nightship-demo>/message.txt)
