
add_definitions(-DTRDK_INTERACTION_ITCH)

file(
    GLOB
    source_list
    Client.cpp
    HotspotStream.cpp
    Security.cpp
    Stream.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Itch MODULE ${source_list})

target_link_libraries(Itch LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
