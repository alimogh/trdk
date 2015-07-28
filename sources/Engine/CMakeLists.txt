
add_definitions(-DTRDK_ENGINE)

file(
    GLOB
    source_list
    SubscriptionsManager.cpp
    Dispatcher.cpp
    ContextBootstrap.cpp
    Context.cpp
    SubscriberPtrWrapper.cpp
    Ini.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Engine SHARED ${source_list})

target_link_libraries(Engine LINK_PRIVATE ${Boost_LIBRARIES} Lib Core)
