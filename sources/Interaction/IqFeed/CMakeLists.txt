
add_definitions(-DTRDK_INTERACTION_IQFEED)

file(
    GLOB
    source_list
    Version.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(IqFeed MODULE ${source_list})

target_link_libraries(IqFeed LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
