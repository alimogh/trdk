
add_definitions(-DTRDK_INTERACTION_ONIXS_FIX_CONNECTOR)

file(
    GLOB
    source_list
    IntegralStream.cpp
    IntegralTrading.cpp
    FxAllTrading.cpp
    HotspotTrading.cpp
    FxAllStream.cpp
    FixTrading.cpp
    FixStream.cpp
    FixSession.cpp
    CurrenexStream.cpp
    CurrenexTrading.cpp
    AlpariStream.cpp
    FastmatchTrading.cpp
    FastmatchStream.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(OnixsFixConnector MODULE ${source_list})

set(TRDK_EXTERNALS_ONIXS_FIX_ENGINE_DIR "${TRDK_EXTERNALS_DIR}OnixS.FixEngineCpp-RHEL70-gcc482-x64-3.19.0.0/")
set(TRDK_EXTERNALS_ONIXS_FIX_ENGINE_LIB_DIR "${TRDK_EXTERNALS_ONIXS_FIX_ENGINE_DIR}lib/")

include_directories("${TRDK_EXTERNALS_ONIXS_FIX_ENGINE_DIR}include")

target_link_libraries(OnixsFixConnector LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
if(WIN32) # Cywin is UNIX and Windows, so test for WIN32 first
    target_link_libraries(
        OnixsFixConnector
        LINK_PRIVATE
        debug
        "${TRDK_EXTERNALS_ONIXS_FIX_ENGINE_LIB_DIR}FIXForge.Cpp.FIX.Engine-vs12_x64_D.lib")
    target_link_libraries(
        OnixsFixConnector
        LINK_PRIVATE
        optimized
        "${TRDK_EXTERNALS_ONIXS_FIX_ENGINE_LIB_DIR}FIXForge.Cpp.FIX.Engine-vs12_x64.lib")
elseif(UNIX)
    target_link_libraries(
        OnixsFixConnector
        LINK_PRIVATE
        "${TRDK_EXTERNALS_ONIXS_FIX_ENGINE_LIB_DIR}libffCppFixEngine.so")
endif()
