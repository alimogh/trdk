"""

 Trading Robot Development Kit module

    Author: Eugene V. Palchukovsky
    E-mail: eugene@palchukovsky.com
   Project: Trading Robot Development Kit
       URL: http://robotdk.com
 Copyright: Eugene V. Palchukovsky

"""


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
    exchange = str
    primaryExchange = str

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

        :param price: float
        :rtype: int
        """
        pass

    def descalePrice(self, price):
        """

        :param price: int
        :rtype: float
        """
        pass


class Security(SecurityInfo):

    def cancelOrder(self, orderId):
        """
        Cancels order by ID. Asynchronous.

        :param orderId: int
        """

    def cancelAllOrders(self):
        """
        Cancels all active orders for this security, which were open by engine.
        Asynchronous.
        """

###############################################################################


class Module(object):

    class Log(object):

        def debug(self, eventMessage):
            """
            Adds new debug record into events.log

            :param eventMessage: str
            """
            pass

        def info(self, eventMessage):
            """
            Adds new information record into events.log

            :param eventMessage: str
            """
            pass

        def warn(self, eventMessage):
            """
            Adds new warning record into events.log

            :param eventMessage: str
            """
            pass

        def error(self, eventMessage):
            """
            Adds new error record into events.log

            :param eventMessage: str
            """
            pass

    name = str
    tag = str

    log = Log

    def __init__(self, param):
        """

        :param param: int
        """
        pass

    def getRequiredSuppliers(self):
        """
        Virtual. Returns required service list. Calls once at engine start.
        Optional.

        :rtype: str
        """
        pass

    def onServiceStart(self, service):
        """
        Notifies about new service start. Virtual.

        :param service: trdk.ServiceInfo
        """
        pass


class Strategy(Module):
    """
    Every strategy algorithm class must be inherited from this class.
    """

    class PositionList(object):
        """
        Iterable position list.
        """

        def __iter__(self):
            pass

        def count(self):
            """

            :rtype: int
            """
            pass

    class SecurityList(object):
        """
        Iterable security list.
        """

        def __iter__(self):
            pass

        def count(self):
            """

            :rtype: int
            """
            pass

        def find(self, symbol, exchange=None, primaryExchange=None):
            """
            Finds security or return None if security doesn't exist in list.

            :rtype: trdk.Security
            """
            pass

    positions = PositionList  # active positions

    def __init__(self, param):
        """

        :param param: int
        """
        super(self.__class__, self).__init__(param)

    def findSecurity(self, symbol, exchange=None, primaryExchange=None):
        """
        Finds security or return None if security doesn't exist.

        :rtype: trdk.Security
        """
        pass

    def onSecurityStart(self, security):
        """
        Notifies about new security start. Virtual.

        :param security: trdk.Security
        """
        pass

    def onLevel1Update(self, security):
        """
        Virtual. Notifies about Level 1 data update (best bid, best ask or
        last trade). Required if strategy subscribed to Level 1 updates.

        :param security: trdk.Security
        """
        pass

    def onNewTrade(self, security, time, price, qty, side):
        """
        Virtual. Notifies about new trade (price tick). Required if strategy
        subscribed to new trades.

        :param security: trdk.Security
        :param time: int
        :param price: int
        :param qty: int
        :param side: char 'S' (sell) or 'B' (buy)
        """
        pass

    def onServiceDataUpdate(self, service):
        """
        Virtual. Notifies about service data update. Required if strategy
        subscribed to at least one service.

        :param service: trdk.ServiceInfo
        """
        pass

    def onPositionUpdate(self, position):
        """
        Virtual. Notifies about position state update. Optional.

        :param position: trdk.Position
        """
        pass

    def onBrokerPositionUpdate(self, security, qty, isInitial):
        """
        Virtual. Notifies about broker position update. Position size "qty"
        may differ from current "trdk::Security::GetBrokerPosition". Optional.

        :param security: trdk.Security
        :param qty: int
        :param isInitial: bool
        """
        pass


class ServiceInfo(Module):

    class SecurityList(object):
        """ Iterable security list.
        """

        def __iter__(self):
            pass

        def count(self):
            """

            :rtype: int
            """
            pass

        def find(self, symbol, exchange=None, primaryExchange=None):
            """
            Finds security or return None if security doesn't exist in list.

            :rtype: trdk.SecurityInfo
            """
            pass

    def __init__(self, param):
        """

        :param param: int
        """
        super(self.__class__, self).__init__(param)


class Service(ServiceInfo):

    def __init__(self, param):
        """

        :param param: int
        """
        pass

    def onSecurityStart(self, security):
        """
        Notifies about new security start. Virtual.

        :param security: trdk.SecurityInfo
        """
        pass

    def onLevel1Update(self, security):
        """
        Virtual. Notifies about Level 1 data update. Required if service
        subscribed to Level 1 updates. Method returns True if service state has
        changed, False otherwise.

        :param security: trdk.SecurityInfo
        :rtype: bool
        """
        pass

    def onServiceDataUpdate(self, service):
        """
        Virtual. Notifies about service data update. Required if service
        subscribed to at least one service. Method returns True if service
        state has changed, False otherwise.

        :param service: trdk.ServiceInfo
        :rtype: bool
        """
        pass

    def onNewTrade(self, security, time, price, qty, side):
        """
        Virtual. Notifies about new trade (price tick). Required if service
        subscribed to new trades. Method returns True if service state has
        changed, False otherwise.

        :param security: trdk.SecurityInfo
        :param time: int
        :param price: int
        :param qty: int
        :param side: char 'S' (sell) or 'B' (buy)
        :rtype: bool
        """
        pass

###############################################################################


class Position(object):

    type = str

    isCompleted = bool  # Started and now hasn't any orders and active qty.

    isOpened = bool  # Has opened qty and hasn't active open-orders.
    isClosed = bool     # First was opened, then closed, hasn't active quantity
                        # and active orders.

    isError = bool  # Open operation started, but error occurred at opening or
                    # closing.
    isCanceled = bool   # All orders canceled, position will be closed or
                        # already closed.

    hasActiveOrders = bool  # True if position has active orders (not cancelled
                            # and not fully filled).
    hasActiveOpenOrders = bool  # True if position has active open-orders
                                # (not cancelled and not fully filled).
    hasActiveCloseOrders = bool     # True if position has active close-orders
                                    # (not cancelled and not fully filled).

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

    security = Security

    def __init__(self, strategy, security, qty, startPrice):
        """

        :param strategy: trdk.Strategy
        :param security: trdk.Security
        :param startPrice: int
        :param qty: int
        """
        pass

    def restoreOpenState(self, openOrderId=0):
        """
        Restores position in open-state. Just creates position in open state at
        current strategy, doesn't make any trading actions. Order ID can be any
        user-defined ID, it doesn't affect the engine logic.

        :param openOrderId: int
        """

    def openAtMarketPrice(self, displaySize=None):
        """
        Sends market order. Asynchronous. Returns order ID.

        :param displaySize: int
        :rtype: int
        """
        pass

    def open(self, price, displaySize=None):
        """
        Sends limit order. Asynchronous. Returns order ID.

        :param price: int
        :param displaySize: int
        :rtype: int
        """

    def openAtMarketPriceWithStopPrice(self, stopPrice, displaySize=None):
        """
        Sends market order. Asynchronous. Returns order ID.

        :param stopPrice: int
        :param displaySize: int
        :rtype: int
        """
        pass

    def openOrCancel(self, price):
        """
        Sends "Immediate or Cancel" order. Asynchronous. Returns order ID.

        :param price: int
        :rtype: int
        """
        pass

    def closeAtMarketPrice(self, displaySize=None):
        """
        Sends market order. Asynchronous. Returns order ID.

        :param displaySize: int
        :rtype: int
        """
        pass

    def close(self, price, displaySize=None):
        """
        Sends limit order. Asynchronous. Returns order ID.

        :param price: int
        :param displaySize: int
        :rtype: int
        """
        pass

    def closeAtMarketPriceWithStopPrice(self, stopPrice, displaySize=None):
        """
        Sends market order. Asynchronous. Returns order ID.

        :param stopPrice: int
        :param displaySize: int
        :rtype: int
        """
        pass

    def closeOrCancel(self, price):
        """
        Sends "Immediate or Cancel" order. Asynchronous. Returns order ID.

        :param price: int
        :rtype: int
        """
        pass

    def cancelAtMarketPrice(self, displaySize=None):
        """
        Cancels all active orders for this position and close at market price.
        Asynchronous. Returns True if position opened and order will be sent.

        :param displaySize: int
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

    def __init__(self, strategy, security, qty, startPrice):
        """

        :param strategy: trdk.Strategy
        :param security: trdk.Security
        :param startPrice: int
        :param qty: int
        """
        super(self.__class__, self)\
            .__init__(strategy, security, qty, startPrice)


class ShortPosition(Position):

    def __init__(self, strategy, security, qty, startPrice):
        """

        :param strategy: trdk.Strategy
        :param security: trdk.Security
        :param startPrice: int
        :param qty: int
        """
        super(self.__class__, self)\
            .__init__(strategy, security, qty, startPrice)

###############################################################################
