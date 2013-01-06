
# Trader engine module
import trader

# Strategy algorithm class must be inherited from trader.Strategy
class Example01(trader.Strategy):

    # Constructor is not required by API, here just for example.
    def __init__(self, param):
        # Pass parameter to base class:
        super(self.__class__, self).__init__(param)
        # Place here initial procedure for this instance:
        self._foo = 10

    # Pure virtual method, must be implemented in strategy implementation.
    # Returns algorithm name (any string unique name).
    def getName(self):
        return self.__class__.__name__

    # Virtual method. Not pure. Not required, even if algorithm uses services.
    def onServiceStart(self, service):
        if service.getName() == 'Nonexistent service':
            # Here can be some logic at service start...
            pass

    # Pure virtual method, must be implemented in strategy implementation.
    # Will be called each time when engine has received Level 1 data update
    # (such as best bid, best ask or last trade).
    def onLevel1Update(self):
        if self.positions.count() == 0:
            # No positions currently opened, so trying to open one...
            self.tryToOpenPosition()
        else:
            # Some positions are opened, checking each for closing...
            map(self.tryToClosePosition, self.positions)

    # Example method: how position can be opened. See onLevel1Update for call.
    def tryToOpenPosition(self):

        # Checking entry condition:
        spread = self.security.bidPrice - self.security.askPrice
        if spread > -0.01:
            # Not the case for order:
            return

        # Creating position object (long position in this case):
        position = trader.LongPosition(
            # strategy object
            self,
            # number of shares
            int(10000 / self.security.askPrice),
            # start price
            self.security.bidPrice)

        # Sending IOC order (method onPositionUpdate will be called at each
        # position state update):
        position.openOrCancel(position.openStartPrice)

    # Example method: how position can be closed. See onLevel1Update for call.
    def tryToClosePosition(self, position):

        if isinstance(position, trader.LongPosition):
            # Here can be special logic for long position.
            pass
        else:
            assert(isinstance(position, trader.ShortPosition))
            # Here can be special logic for short position.
            pass

        takeProfitPrice = position.openPrice + 0.03
        stopLossPrice = position.openPrice - 0.03

        if self.security.askPrice >= takeProfitPrice:
            if position.hasActiveOrders:
                # Order already sent and still active:
                return
            # Take profit: trying to close with IOC (method onPositionUpdate
            # will be called at each position state update):
            position.closeOrCancel(takeProfitPrice)
        elif self.security.askPrice <= stopLossPrice:
            # Stop loss: cancel all active orders and close position at market
            # price (method onPositionUpdate will be called at each position
            # state update):
            position.cancelAtMarketPrice()
