
###############################################################################

__all__ = [
    "SecurityInfo",
    "Security",
    "LongPosition",
    "ShortPosition",
    "SecurityAlgo",
    "Strategy"]

###############################################################################

class SecurityInfo(object):

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

class Security(SecurityInfo):

    def cancelOrder(self, orderId):
        """ Cancels order by ID. Asynchronous.
        :type orderId: int
        """

    def cancelAllOrders(self):
        """ Cancels all active orders for this security, which were open by
        engine. Asynchronous.
        """

###############################################################################

class SecurityAlgo(object):

    class Log(object):

        def debug(self, eventMessage):
            """ Adds new debug record into events.log
            :type eventMessage: str
            """
            pass

        def info(self, eventMessage):
            """ Adds new information record into events.log
            :type eventMessage: str
            """
            pass

        def warn(self, eventMessage):
            """ Adds new warning record into events.log
            :type eventMessage: str
            """
            pass

        def error(self, eventMessage):
            """ Adds new error record into events.log
            :type eventMessage: str
            """
            pass

        def trading(self, eventMessage):
            """Adds new record into trading.log
            :type eventMessage: str
            """
            pass

    name = str
    tag = str

    log = Log

    def __init__(self, param):
        """
        :type param: int
        """
        pass

    def onServiceStart(self, service):
        """ Notifies about new service start. Virtual.
        :type service: trader.Service
        """
        pass

class Strategy(SecurityAlgo):
    """ Every strategy algorithm class must be inherited from this class.
    """

    class PositionList(object):
        """ Iterable position list.
        """
        def count(self):
            """
            :rtype: int
            """
            pass

    security = Security

    positions = PositionList # active positions

    def __init__(self, param):
        """
        :type param: int
        """
        pass

    def onLevel1Update(self):
        """
        Virtual. Notifies about Level 1 data update (best bid, best ask or
        last trade). Required if strategy subscribed to Level 1 updates.
        """
        pass

    def onNewTrade(self, time, price, qty, side):
        """
        Virtual. Notifies about new trade (price tick). Required if strategy
        subscribed to new trades.
        :type time: int
        :type price: int
        :type qty: int
        :type side: char 'S' (sell) or 'B' (buy)
        """
        pass

    def onServiceDataUpdate(self, service):
        """
        Virtual. Notifies about service data update. Required if strategy
        subscribed to at least one service.
        :type service: trader.Service
        """
        pass

    def onPositionUpdate(self, position):
        """
        Virtual. Notifies about position state update. Optional.
        :type position: trader.Position
        """
        pass

class ServiceInfo(SecurityAlgo):

    security = SecurityInfo

    def __init__(self, param):
        """
        :type param: int
        """
        pass

class Service(ServiceInfo):

    def __init__(self, param):
        """
        :type param: int
        """
        pass

    def onLevel1Update(self):
        """
        Virtual. Notifies about Level 1 data update. Required if service
        subscribed to Level 1 updates. Method returns True if service state has
        changed, False otherwise.
        :rtype: bool
        """
        pass

    def onServiceDataUpdate(self, service):
        """
        Virtual. Notifies about service data update. Required if service
        subscribed to at least one service. Method returns True if service
        state has changed, False otherwise.
        :type service: trader.Service
        :rtype: bool
        """
        pass

    def onNewTrade(self, time, price, qty, side):
        """
        Virtual. Notifies about new trade (price tick). Required if service
        subscribed to new trades. Method returns True if service state has
        changed, False otherwise.
        :type time: int
        :type price: int
        :type qty: int
        :type side: char 'S' (sell) or 'B' (buy)
        :rtype: bool
        """
        pass

###############################################################################

class Position(object):

    type = str

    isCompleted = bool  # Started and now hasn't any orders and active qty.

    isOpened = bool # Has opened qty and hasn't active open-orders.
    isClosed = bool # Has opened qty and the same closed qty. Hasn't active
                    # close-orders.

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

    def __init__(self, strategy, qty, startPrice, tag):
        """
        :type strategy: trader.Strategy
        :type startPrice: int
        :type qty: int
        :type tag: str
        """

    def openAtMarketPrice(self):
        """ Sends market order. Asynchronous. Returns order ID.
        :rtype: int
        """
        pass

    def open(self, price):
        """ Sends limit order. Asynchronous. Returns order ID.
        :type price: int
        :rtype: int
        """

    def openAtMarketPriceWithStopPrice(self, stopPrice):
        """ Sends market order. Asynchronous. Returns order ID.
        :type stopPrice: int
        :rtype: int
        """
        pass

    def openOrCancel(self, price):
        """ Sends "Immediate or Cancel" order. Asynchronous. Returns order ID.
        :type price: int
        :rtype: int
        """
        pass

    def  closeAtMarketPrice(self):
        """ Sends market order. Asynchronous. Returns order ID.
        :rtype: int
        """
        pass

    def close(self, price):
        """ Sends limit order. Asynchronous. Returns order ID.
        :type price: int
        :rtype: int
        """
        pass

    def closeAtMarketPriceWithStopPrice(self, stopPrice):
        """
        Sends market order. Asynchronous. Returns order ID.
        :type stopPrice: int
        :rtype: int
        """
        pass

    def closeOrCancel(self, price):
        """ Sends "Immediate or Cancel" order. Asynchronous. Returns order ID.
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

    def __init__(self, strategy, qty, startPrice, tag):
        """
        :type strategy: trader.Strategy
        :type startPrice: int
        :type qty: int
        :type tag: str
        """
        pass

class ShortPosition(Position):

    def __init__(self, strategy, qty, startPrice, tag):
        """
        :type strategy: trader.Strategy
        :type startPrice: int
        :type qty: int
        :type tag: str
        """
        pass

###############################################################################
