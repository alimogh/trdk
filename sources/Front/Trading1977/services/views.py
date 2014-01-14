
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/09 19:16:24"

from django.http import HttpResponse, HttpResponseServerError
from trdkfront.wsgi import application
import json
from trdkfront import connection as connection
import trdk


def getState(request):

    connection.makeConnection()

    result = dict()

    for strategy in application.tradeStrategies.itervalues():
        position = None
        for position in strategy[0].positions:
            break
        if position is not None:
            result['pos'] = {}
            result['pos']['symbol']\
                = position.security.symbol + position.security.currency
            result['pos']['notOpenedQty'] = position.notOpenedQty
            result['pos']['activeQty'] = position.activeQty
            result['pos']['closedQty'] = position.closedQty
            break

    if application.tradeEngine is not None:
        result['cashBalance'] = application.tradeEngine.tradeSystem.cashBalance
    else:
        result['cashBalance'] = 'unknown'

    result['isConnected'] = application.tradeEngine is not None

    return HttpResponse(json.dumps(result), mimetype='application/json')


def openPosition(request):

    try:
        symbol = str(request.POST['symbol'])
        qty = int(request.POST['qty'])
    except KeyError:
        return HttpResponseServerError('Key error!')

    if symbol == "" or qty < 1:
        return HttpResponseServerError('Key value error!')

    if symbol not in application.tradeStrategies.keys():
        return HttpResponseServerError(
            'Symbol "{0}" is unknown!'.format(symbol))

    position = trdk.LongPosition(
        application.tradeStrategies[symbol][0],
        application.tradeStrategies[symbol][1],
        qty,
        0)
    position.openAtMarketPrice()

    return HttpResponse("")


def closePosition(request):

    try:
        symbol = str(request.POST['symbol'])
    except KeyError:
        return HttpResponseServerError('Key error!')

    if symbol == "":
        return HttpResponseServerError('Key value error!')

    if symbol not in application.tradeStrategies.keys():
        return HttpResponseServerError('Symbol "{0}" is unknown!'.format(symbol))

    for strategy in application.tradeStrategies.itervalues():
        map(
            lambda position: position.cancelAtMarketPrice(),
            strategy[0].positions)

    return HttpResponse("")
