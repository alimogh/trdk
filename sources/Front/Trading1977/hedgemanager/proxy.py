
import trdk
from trdkfront.wsgi import application


# noinspection PyCallByClass,PyTypeChecker
class Proxy(trdk.Strategy):

    def __init__(self, param, uid, symbol1, symbol2):
        trdk.Strategy.__init__(self, param)
        self.uid = uid
        self.symbol1 = symbol1
        self.symbol2 = symbol2
        application.fullTradeStrategyList.append(self)

    def getRequiredSuppliers(self):
        return 'Broker Positions[{0}, {1}]'.format(self.symbol1, self.symbol2)

    def onBrokerPositionUpdate(self, security, qty, isInitial):

        if isInitial is False:
            return

        if qty < 0:
            position = trdk.ShortPosition(self, security, qty, 0)
        elif qty > 0:
            position = trdk.LongPosition(self, security, qty, 0)
        else:
            return

        self.log.debug(
            'Restoring {0} {1} position: {2}...'
            .format(
                position.type,
                position.security.symbol,
                position.activeQty))
        position.restoreOpenState()


class Proxy1(Proxy):

    def __init__(self, param):
        Proxy.__init__(
            self,
            param,
            '2166230c-b4ff-4f4c-8cf1-24d7af1a3b91',
            'AUD.USD',
            'GBP.USD')


class Proxy2(Proxy):

    def __init__(self, param):
        Proxy.__init__(
            self,
            param,
            'e6bb0f76-19a4-4fec-a3be-15930d68331e',
            'EUR.USD',
            'NZD.USD')
