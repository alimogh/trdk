
add_definitions(-DTRDK_ENGINE_SERVICE)

file(
    GLOB
    source_list
    trdk.EngineService.Control.pb.cc
    trdk.EngineService.DropCopy.pb.cc
    trdk.EngineService.MarketData.pb.cc
    trdk.EngineService.MarketDataSource.pb.cc
    trdk.EngineService.pb.cc
    Utils.cpp)

add_library(EngineService STATIC ${source_list})

target_link_libraries(EngineService LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
