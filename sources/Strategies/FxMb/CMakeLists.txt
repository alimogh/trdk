
add_definitions(-DTRDK_STRATEGIES_FXMB)

file(
    GLOB
    source_list
    TriangulationWithDirection.cpp
    TriangulationWithDirectionReport.cpp
    TriangulationWithDirectionStatService.cpp
    TriangulationWithDirectionTriangle.cpp
    TriangulationWithDirectionPosition.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(FxMb MODULE ${source_list})

target_link_libraries(FxMb LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
