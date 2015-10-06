
add_definitions(-DTRDK_STRATEGY_TEST)

file(
    GLOB
    source_list
    TestStrategy.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(TestStrategy MODULE ${source_list})

target_link_libraries(TestStrategy LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
