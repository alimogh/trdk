
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/09 19:16:24"

from django.http import HttpResponse, HttpResponseServerError
from trdkfront.wsgi import application
import json
from trdkfront import connection as connection
import trdk
from django.conf import settings as frontSettings
import uuid


cache = None


def getState(request):

    global cache

    connection.makeConnection()

    result = {}

    if 'fullUpdate' in request.POST or cache is None:
        cache = dict()
        result['fullUpdate'] = True

    isConnected = application.tradeEngine is not None
    if 'connection' not in cache or cache['connection'] != isConnected:
        result['connection'] = cache['connection'] = isConnected

    balance = application.tradeEngine.tradeSystem.cashBalance
    if 'balance' not in cache or cache['balance'] != balance:
        result['balance'] = cache['balance'] = balance

    if 'strategies' not in cache:
        if result['fullUpdate']:
            cache['newStrategies'] = []
        cache['strategies'] = []
        for strategy in application.tradeStrategies:
            strategyInfo = {
                'uid': strategy.tag,
                'account': '---',
                'baseCurrency': 'USD',
                'tradeSizeInBase': 35000,
                'tradeSize': 35000,
                'symbol1': strategy.symbol1,
                'symbol2': strategy.symbol2}
            if result['fullUpdate']:
                cache['newStrategies'].append(strategyInfo)
            cache['strategies'].append({
                'uid': strategy.tag,
                's1': {
                    'ask': 0,
                    'bid': 0,
                    'spread': 0,
                    'fPrice': 0,
                    'opened': 1},
                's2': {
                    'ask': 0,
                    'bid': 0,
                    'spread': 0,
                    'fPrice': 0,
                    'opened': 1}})
        if result['fullUpdate']:
            result['newStrategies'] = cache['newStrategies']

    result['strategies'] = []
    for strategy in application.tradeStrategies:
        xxx = {'uid': strategy.tag}
        for security in strategy.securities:
            ask = security.descalePrice(security.askPrice)
            bid = security.descalePrice(security.bidPrice)
            zzz = {
                'ask': ask,
                'bid': bid,
                'spread': round(ask - bid, 6),
                'fPrice': 0}
            for position in strategy.positions:
                if position.security.symbol == security.symbol:
                    zzz['fPrice'] = security.descalePrice(position.openPrice)
                    break
            if 's1' not in xxx:
                xxx['s1'] = zzz
            else:
                xxx['s2'] = zzz
        assert 's1' in xxx
        if 's2' not in xxx:
            xxx['s2'] = xxx['s1']

        result['strategies'].append(xxx)

    return HttpResponse(json.dumps(result), mimetype='application/json')


def addStrategy(request):

    try:
        symbol1 = str(request.POST['symbol1'])
        symbol2 = str(request.POST['symbol2'])
    except KeyError:
        return HttpResponseServerError('Key error!')
    if symbol1 == "" or symbol2 == "":
        return HttpResponseServerError('Key value error!')

    if symbol1 != symbol2:
        symbolList = symbol1 + ", " + symbol2
    else:
        symbolList = symbol1

    newStrategy = \
        "[Strategy." + str(uuid.uuid1()) + "]\n" \
        "   module = " + frontSettings.BASE_DIR + "trdk.pyd\n" \
        "   class = Proxy\n" \
        "   dbg_auto_name = no\n" \
        "   requires = Level 1 Updates[" + symbolList + "]\n" \
        "   script_file_path = " + frontSettings.BASE_DIR + "hedgemanager\proxy.py\n"
    application.tradeEngine.addStrategies(newStrategy)

    for strategy in application.fullTradeStrategyList:
        if strategy.symbol1 is not None:
            continue
        assert strategy.symbol2 is None
        strategy.symbol1 = symbol1
        strategy.symbol2 = symbol2
        application.tradeStrategies.append(strategy)
    application.fullTradeStrategyList = []

    global cache
    cache = None

    return HttpResponse("")


def deleteStrategy(request):
    try:
        strategyUid = str(request.POST['strategyUid'])
    except KeyError:
        return HttpResponseServerError('Key error!')
    if strategyUid == "":
        return HttpResponseServerError('Key value error!')
    strategies = []
    for strategy in application.tradeStrategies:
        if strategy.tag != strategyUid:
            strategies.append(strategy)
    application.tradeStrategies = strategies
    global cache
    cache = None
    return HttpResponse("")


def openPosition(request, isLong):

    try:
        strategyUid = str(request.POST['strategyUid'])
        symbol = int(request.POST['symbol'])
    except KeyError:
        return HttpResponseServerError('Key error!')

    if strategyUid == "" or symbol < 1 or symbol > 2:
        return HttpResponseServerError('Key value error!')

    for i in application.tradeStrategies:
        if i.uid == strategyUid:
            strategy = i
            for j in strategy.securities:
                symbol -= 1
                if symbol == 0:
                    security = j
                    break
            break
    if strategy is None or security is None:
        return HttpResponseServerError(
            'Strategy "{0}" is unknown!'.format(strategyUid))

    if isLong is True:
        position = trdk.LongPosition(strategy, security, 35000, 0)
    else:
        position = trdk.ShortPosition(strategy, security, 35000, 0)
    position.openAtMarketPrice()

    return HttpResponse("")


def openLongPosition(request):
    return openPosition(request, True)


def openShortPosition(request):
    return openPosition(request, False)


def closePosition(request):

    try:
        strategyUid = str(request.POST['strategyUid'])
        symbol = int(request.POST['symbol'])
    except KeyError:
        return HttpResponseServerError('Key error!')
    if strategyUid == "" or symbol < 1 or symbol > 2:
        return HttpResponseServerError('Key value error!')

    for i in application.tradeStrategies:
        if i.uid == strategyUid:
            strategy = i
            for j in strategy.securities:
                symbol -= 1
                if symbol == 0:
                    security = j
                    break
            break
    if strategy is None or security is None:
        return HttpResponseServerError(
            'Strategy "{0}" is unknown!'.format(strategyUid))

    for position in strategy.positions:
        if position.security.symbol == security.symbol:
            position.cancelAtMarketPrice()

    return HttpResponse("")


def closeAllPositions(request):

    try:
        strategyUid = str(request.POST['strategyUid'])
    except KeyError:
        return HttpResponseServerError('Key error!')

    if strategyUid == "":
        return HttpResponseServerError('Key value error!')

    for strategy in application.tradeStrategies:
        if strategy.uid == strategyUid:
            for position in strategy.positions:
                position.cancelAtMarketPrice()
            return HttpResponse("")

    return HttpResponseServerError(
        'Strategy "{0}" is unknown!'.format(strategyUid))
