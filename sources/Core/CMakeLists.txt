
add_definitions(-DTRDK_CORE)

file(
    GLOB
    source_list
    Position.cpp
    Types.cpp
    TradeSystem.cpp
    Terminal.cpp
    Strategy.cpp
    Settings.cpp
    RiskControl.cpp
    Security.cpp
    Observer.cpp
    Module.cpp
    MarketDataSource.cpp
    EventsLog.cpp
    DropCopy.cpp
    Context.cpp
    Consumer.cpp
    Interactor.cpp
    AsyncLog.cpp
    Service.cpp
    StrategyPositionState.cpp
    Instrument.cpp
    Log.cpp
    Version.cpp)
trdk_add_precompiled_header("Prec.h" "Prec.cpp" source_list)

add_library(Core SHARED ${source_list})

target_link_libraries(Core LINK_PRIVATE ${Boost_LIBRARIES} Lib)
