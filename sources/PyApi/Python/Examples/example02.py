
import trader
import time

class Example02(trader.Strategy):

    _bars = None

    def getName(self):
        return self.__class__.__name__

    def notifyServiceStart(self, service):
        assert(self._bars is None)
        if service.tag == "5 minute bar":
            self._bars = service
        else:
            super(self.__class__, self).notifyServiceStart(service)

    def onServiceDataUpdate(self, service):

        # Events only for registered services
        assert(self._bars is not None)
        assert(service == self._bars)
        assert(self._bars.isEmpty == False)

        lastBar = self._bars.getBarByReversedIndex(0)
        openPrice = self._bars.getOpenPriceStat(10)
        trader.logInfo(
            "{0}: new data ({1} / {2})..."
                .format(
                    time.ctime(lastBar.time),
                    self.security.descalePrice(lastBar.openPrice),
                    self.security.descalePrice(openPrice.max)))

        # Check condition (price scaled):
        if openPrice.max <= 137449:
            # Skip opening as max open price less then required...
            return False    # Method returns False - no another events will be
                            # called for this event

        return True # True initiates positions methods call

    def tryToOpenPositions(self):

        maxOpenPrice = self._bars.getOpenPriceStat(10).max

        trader.logInfo(
            "Opening as max open price {0}."
                .format(self.security.descalePrice(maxOpenPrice)))

        self._testCount = 0
        position = trader.LongPosition(
            # security object
            self.security,
            # number of shares
            10000,
            # start price
            maxOpenPrice,
            # strategy tag for statistic
            self.tag)

        # Sending IOC order (if order will be filled:
        # tryToClosePositions-method will be called, if not:
        # tryToOpenPositions will be called again):
        position.openOrCancel(position.openStartPrice)

        return position

    def tryToClosePositions(self, position):
        if position.hasActiveOrders:
            return
        self._testCount = self._testCount + 1
        if self._testCount < 10:
            trader.logInfo('Skip closing...')
        else:
            trader.logInfo('Close position...')
            position.cancelAtMarketPrice()
            self._stop = True
