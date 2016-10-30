
add_definitions(-DTRDK_INTERACTION_TRANSAQ)

file(
    GLOB
    source_list
    Api.cpp
    Version.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Transaq MODULE ${source_list})

target_link_libraries(Transaq LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
