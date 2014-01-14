
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/11 17:37:13"

from django.conf import settings as frontSettings
from wsgi import application
import trdk


def makeConnection():

    if application.tradeEngine is not None:
        return

    trdk.log.enableEventsToStdOut()

    engineSettings = \
        "[Common]\n" \
        "   trade_session_period_edt = 00:00:00 - 23:59:59\n" \
        "   wait_market_data = no\n" \
        "[Defaults]\n" \
        "   primary_exchange = FOREX\n" \
        "   exchange = IDEALPRO\n" \
        "   currency = USD\n" \
        "[TradeSystem]\n" \
        "   module = " + frontSettings.BASE_DIR + "trdk.pyd\n" \
        "   dbg_auto_name = no\n" \
        "   account = \n" \
        "[Strategy.Proxy]\n" \
        "   module = " + frontSettings.BASE_DIR + "trdk.pyd\n" \
        "   dbg_auto_name = no\n" \
        "   script_file_path = " + frontSettings.BASE_DIR + "hedgemanager\proxy.py\n" \
        "   instances = " + frontSettings.BASE_DIR + "symbols.ini\n"

    engine = trdk.Engine(engineSettings)
    try:
        engine.start()
    except RuntimeError:
        return
    assert len(application.fullTradeStrategyList) > 0

    tradeStrategies = {}
    for strategy in application.fullTradeStrategyList:
        for security in strategy.securities:
            break
        key = security.symbol + security.currency
        tradeStrategies[key] = []
        tradeStrategies[key].append(strategy)
        tradeStrategies[key].append(security)

    application.fullTradeStrategyList = []
    application.tradeEngine = engine
    application.tradeStrategies = tradeStrategies

