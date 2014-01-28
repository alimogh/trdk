
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/09 19:16:24"

from django.http import HttpResponse, HttpResponseServerError
from trdkfront.wsgi import application
import json
from trdkfront import connection as connection
import trdk


cache = None


def getState(request):

    global cache

    connection.makeConnection()

    result = {}

    result['fullUpdate'] \
        = 'fullUpdate' in request.POST and request.POST['fullUpdate']
    if result['fullUpdate'] is False and cache is None:
        cache = dict()
        cache['strategies'] = dict()

    if 'new' not in cache['strategies']:
        cache['strategies']['new'] = {}
        cache['strategies']['new'] = [
            {
                'id': '2166230c-b4ff-4f4c-8cf1-24d7af1a3b91',
                'account': 'DU15079',
                'baseCurrency': 'USD',
                'tradeSize': 1000,
                'symbol1': 'AUD.USD',
                'symbol2': 'GBP.USD'
            },
            {
                'id': 'e6bb0f76-19a4-4fec-a3be-15930d68331e',
                'account': 'DU15079',
                'baseCurrency': 'USD',
                'tradeSize': 1000,
                'symbol1': 'EUR.CHF',
                'symbol2': 'JPY.NZD'
            }]
        if 'strategies' not in result:
            result['strategies'] = {}
        result['strategies']['new'] = cache['strategies']['new']

    isConnected = application.tradeEngine is not None
    if 'connection' not in cache or cache['connection'] != isConnected:
        result['connection'] = cache['connection'] = isConnected

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
