
###############################################################################

__all__ = [
    "logInfo",
    "logTrading",
    "Security",
    "LongPosition",
    "ShortPosition",
    "SecurityAlgo",
    "Strategy"]

###############################################################################

def logInfo(event):
    """ Adds new log record into events.log
    :type event: str
    """
    pass

def logTrading(tag, event):
    """Adds new log record into trading.log
    :type tag: str
    :type event: str
    """
    pass

###############################################################################

class Security(object):

    symbol = str
    fullSymbol = str

    currency = str

    priceScale = int

    lastPrice = int
    lastSize = int

    askPrice = int
    askSize = int

    bidPrice = int
    bidSize = int

    def scalePrice(self, price):
        """
        :type price: float
        :rtype: int
        """
        pass

    def descalePrice(self, price):
        """
        :type price: int
        :rtype: float
        """
        pass

    def cancelOrder(self, orderId):
        """ Cancels order by ID.
        :type orderId: int
        """

    def cancelAllOrders(self):
        """ Cancels all active orders for this security, which were open by
        engine.
        """

###############################################################################

class Position(object):

    type = str

    hasActiveOrders = bool  # True if position has active orders (not cancelled
    # and not fully filled).

    planedQty = int

    openStartPrice = int
    openOrderId = int
    openedQty = int
    openPrice = int

    notOpenedQty = int
    activeQty = int

    closeOrderId = int
    closeStartPrice = int
    closePrice = int
    closedQty = int

    commission = int

    def __init__(self, security, qty, startPrice, tag):
        """
        :type security: Trade.Security
        :type startPrice: int
        :type qty: int
        :type tag: str
        """

    def openAtMarketPrice(self):
        """ Asynchronous. Returns order ID.
        :rtype: int
        """
        pass

    def open(self, price):
        """ Asynchronous. Returns order ID.
        :type price: int
        :rtype: int
        """

    def openAtMarketPriceWithStopPrice(self, stopPrice):
        """ Asynchronous. Returns order ID.
        :type stopPrice: int
        :rtype: int
        """
        pass

    def openOrCancel(self, price):
        """ Asynchronous. Returns order ID.
        :type price: int
        :rtype: int
        """
        pass

    def  closeAtMarketPrice(self):
        """ Asynchronous. Returns order ID.
        :rtype: int
        """
        pass

    def close(self, price):
        """ Asynchronous. Returns order ID.
        :type price: int
        :rtype: int
        """
        pass

    def closeAtMarketPriceWithStopPrice(self, stopPrice):
        """ Asynchronous. Returns order ID.
        :type stopPrice: int
        :rtype: int
        """
        pass

    def closeOrCancel(self, price):
        """
        Closes position with "Immediate or Cancel" order. Asynchronous.
        Returns order ID.
        :type price: int
        :rtype: int
        """
        pass

    def cancelAtMarketPrice(self):
        """
        Cancels all active orders for this position and close at market price.
        Asynchronous. Returns True if position opened and order will be sent.
        :rtype: int
        """
        pass

    def cancelAllOrders(self):
        """
        Cancels all active orders for this position. Asynchronous.
        Returns True if one or more orders will be canceled.
        :rtype: bool
        """
        pass

class LongPosition(Position):

    def __init__(self, security, qty, startPrice, tag):
        """
        :type security: Trade.Security
        :type startPrice: int
        :type qty: int
        :type tag: str
        """
        pass

class ShortPosition(Position):

    def __init__(self, security, qty, startPrice, tag):
        """
        :type security: Trade.Security
        :type startPrice: int
        :type qty: int
        :type tag: str
        """
        pass

###############################################################################

class SecurityAlgo(object):

    tag = str

    security = Security

    def __init__(self, param):
        """
        :type param: int
        """
        pass

    def getName(self):
        """ Pure virtual method. Returns algorithm method.
        :rtype: str
        """
        pass

    def notifyServiceStart(self, service):
        """ Method notifies about new service start. Virtual.
        :type service: trader.Service
        """
        pass

    def onServiceDataUpdate(self, service):
        """
        Method notifies about service data update. Method returns True if
        object state has changed, False otherwise. Virtual.
        :type service: trader.Service
        :rtype: bool
        """
        pass

    def onNewTrade(self, time, price, qty, side):
        """
        Method notifies about new trade (price tick). Method returns True
        if object state has changed, False otherwise. Virtual.
        :type time: int
        :type price: int
        :type qty: int
        :type side: char 'S' (sell) or 'B' (buy)
        :rtype: bool
        """
        pass

class Strategy(SecurityAlgo):
    """ Every strategy algorithm class must be inherited from this class.
    """

    def __init__(self, param):
        """
        :type param: int
        """
        pass

    def tryToOpenPositions(self):
        """

        Pure virtual method

        Returns Trader.LongPosition or Trader.ShortPosition if decides to
        open position or returns nothing otherwise.

        Will be called by engine each time when binded service has changed
        its state (ex.: received new market data for strategy symbol).

        :rtype: trader.Position
        """
        pass

    def tryToClosePositions(self, position):
        """

        Pure virtual method.

        Takes Trader.LongPosition or Trader.ShortPosition and tries to
        close it.

        Will be called by engine every time when:
        - binded service has changed its state (ex.: received new market
          data for strategy symbol);
        - strategy has opened position;
        - all previous orders has been canceled or filled.

        :type position: trader.Position
        """
        pass

class Service(SecurityAlgo):

    def __init__(self, param):
        """
        :type param: int
        """
        pass

###############################################################################
