add_executable(g_w3dview WIN32)

target_link_libraries(g_w3dview PRIVATE
    core_config
    core_utility
    core_wwstub # avoid linking GameEngine
    corei_w3dview # this interface gets the source files for the tool
    d3d8
    d3d8lib
    d3dx8
    dbghelplib
    imm32
    milesstub
    Version
    vfw32
    winmm
    g_wwaudio
    g_wwvegas
)

if(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
    target_compile_definitions(g_w3dview PRIVATE _AFXDLL)
    set_target_properties(g_w3dview PROPERTIES OUTPUT_NAME W3DViewV)
else()
    set_target_properties(g_w3dview PROPERTIES OUTPUT_NAME w3dviewv)
endif()
