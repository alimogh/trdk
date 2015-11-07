
add_definitions(-DTRDK_LIB)

file(
    GLOB
    source_list
    TimeMeasurement.cpp
    Symbol.cpp
    Ini.cpp
    Currency.cpp
    Util.cpp
    SysError.cpp
    Log.cpp
    FileSystemChangeNotificator.cpp
    Exception.cpp
    Defaults.cpp
    Assert.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Lib STATIC ${source_list})

target_link_libraries(Lib LINK_PRIVATE ${Boost_LIBRARIES})
target_include_directories(Lib PUBLIC ${TRDK_SOURCES_DIR})
