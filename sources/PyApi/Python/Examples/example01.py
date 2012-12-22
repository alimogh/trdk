
# Trader engine module
import trader

################################################################################

# Strategy algorithm class must be inherited from Trader.Strategy
class Example01(trader.Strategy):

    # Constructor doesn't required, here just for example.
    def __init__(self, param):
        # Pass parameter to base class:
        super(self.__class__, self).__init__(param)
        # Place here initial procedure for this instance:
        self._foo = 10

    # Pure virtual method, must be implemented in strategy implementation.
    # Returns algorithm name.
    def getName(self):
        return self.__class__.__name__

    # Virtual method. Not pure. Not required, if algorithm doesn't use services.
    def notifyServiceStart(self, service):
        if service.getName() == 'Nonexistent service':
            # Using service object if algorithm requires it...
            pass
        else:
            # Send service object to base if algorithm doesn't required it...
            super(self.__class__, self).notifyServiceStart(service)

    # Pure virtual method, must be implemented in strategy implementation.
    # Will be called by engine each time when binded service has changed its
    # state (ex.: received new market data for strategy symbol).
    def tryToOpenPositions(self):

        # Checking entry condition:
        spread = self.security.bidPrice - self.security.askPrice
        if spread > -0.01:
            # not the case for trade
            return

        # Creating position object (long position on this case):
        position = trader.LongPosition(
            # security object
            self.security,
            # number of shares
            int(10000 / self.security.askPrice),
            # start price
            self.security.bidPrice,
            # strategy tag for statistic
            self.tag)

        # Sending IOC order (if order will be filled:
        # tryToClosePositions-method will be called, if not:
        # tryToOpenPositions will be called again):
        position.openOrCancel(position.openStartPrice)

        return position

    # Pure virtual method, must be implemented in strategy implementation.
    # Will be called by engine every time when:
    #	- binded service has changed its state (ex.: received new market data
    #	  for strategy symbol);
    #	- strategy has opened position;
    #	- all previous orders has been canceled or filled.
    def tryToClosePositions(self, position):

        takeProfitPrice = position.openPrice + 0.03
        stopLossPrice = position.openPrice - 0.03

        if self.security.askPrice >= takeProfitPrice:
            if position.hasActiveOrders:
                # order already sent and still active
                return
            # take profit: trying to close with IOC
            position.closeOrCancel(takeProfitPrice)
        elif self.security.askPrice <= stopLossPrice:
            # stop loss: cancel all active orders and close
            # position at market price
            position.cancelAtMarketPrice()
