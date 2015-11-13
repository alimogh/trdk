
add_definitions(-DTRDK_ENGINE_SERVER)

file(
    GLOB
    source_list
    Service.cpp
    Server.cpp
    Client.cpp
    DropCopyService.cpp
    Settings.cpp
    Main.cpp
    DropCopyClient.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_executable(RobotEngine ${source_list})
set_target_properties(RobotEngine PROPERTIES DEBUG_POSTFIX "_dbg")
set_target_properties(RobotEngine PROPERTIES TEST_POSTFIX "_test")

include_directories("${TRDK_EXTERNALS_DIR}autobahn/")
include_directories("${TRDK_EXTERNALS_DIR}msgpack-c/include/")

target_link_libraries(
    RobotEngine
    LINK_PRIVATE
    pthread dl ${Boost_LIBRARIES} Lib Core Engine)
