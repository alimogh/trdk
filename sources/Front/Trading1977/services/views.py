
__author__ = "Eugene V. Palchukovsky"
__email__ = "eugene@palchukovsky.com"
__date__ = "2014/01/09 19:16:24"

from django.http import HttpResponse
from trdkfront.wsgi import application
import json

def getState(request):

    result = dict()

    if hasattr(application, 'symbol') is True and application.symbol is not None:
        result['pos'] = {}
        result['pos']['symbol'] = application.symbol
        result['pos']['qty'] = application.qty

    application.xxx += 1
    result['cashBalance'] = application.xxx

    result['isConnected'] = application.xxx % 2

    return HttpResponse(json.dumps(result), mimetype='application/json')

def openPosition(request):

    application.xxx += 100

    try:
        symbol = str(request.POST['symbol'])
        qty = int(request.POST['qty'])
    except KeyError:
        return HttpResponse('Key error!')

    if symbol == "" or qty < 1:
        return HttpResponse('Key value error!')

    if hasattr(application, 'symbol') is True and application.symbol is not None:
        return HttpResponse('Position already opened!')

    application.symbol = symbol
    application.qty = qty

    return HttpResponse("")


def closePosition(request):
    application.symbol = None
    application.qty = None
    return HttpResponse("")
