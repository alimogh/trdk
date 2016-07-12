
add_definitions(-DTRDK_INTERACTION_FAKE)

file(
    GLOB
    source_list
    Api.cpp
    FakeMarketDataSource.cpp
    FakeTradingSystem.cpp
    Version.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Fake MODULE ${source_list})

target_link_libraries(Fake LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
