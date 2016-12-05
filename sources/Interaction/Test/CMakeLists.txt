
add_definitions(-DTRDK_INTERACTION_TEST)

file(
    GLOB
    source_list
    Api.cpp
    RandomMarketDataSource.cpp
    TradingSystem.cpp
    Version.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Test MODULE ${source_list})

target_link_libraries(Test LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
