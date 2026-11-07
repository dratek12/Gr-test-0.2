cmake_minimum_required(VERSION 3.20)
project(Cs2Esp LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

# uprav podle sve instalace CS2
set(GAME_DIR "C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Global Offensive/game/csgo/bin/win64")

set(BACKEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DrateCoreD/vendor/imgui/backends")
set(GETREKT_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(DrateCoreD)

get_property(KIT_TARGETS DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DrateCoreD" PROPERTY BUILDSYSTEM_TARGETS)
list(GET KIT_TARGETS 0 KIT_TARGET)

add_library(Cs2Esp SHARED
    DLLMain.cpp Global.cpp SDK.cpp ESP.cpp LegitBot.cpp Misc.cpp Config.cpp Hooks.cpp
    "${BACKEND_DIR}/imgui_impl_dx11.cpp"
    "${BACKEND_DIR}/imgui_impl_win32.cpp"
)

target_include_directories(Cs2Esp PRIVATE
    "${BACKEND_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/DrateCoreD/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(Cs2Esp PRIVATE ${KIT_TARGET} d3d11 dwmapi dxgi comctl32)
set_target_properties(Cs2Esp PROPERTIES OUTPUT_NAME "Cs2Esp")

add_executable(Injector injector.cpp)
target_link_libraries(Injector PRIVATE advapi32)

add_custom_command(TARGET Injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Cs2Esp>" "$<TARGET_FILE_DIR:Injector>"
)

add_custom_command(TARGET Injector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Cs2Esp>" "${GAME_DIR}/Cs2Esp.dll"
    COMMENT "Kopiruji DLL do slozky hry"
)