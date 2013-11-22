# TRDK-33
#
# When price goes below lower band and price comes back above lower band, enter
# a buy. when price goes above higher band and back down the band, enter a
# sell.
#
# I want to input time frame to use, i.e. 15 min, 1 hour, 4 hour, daily, or
# weekly.
#
# Only trade one position per symbol.



import trdk
import time


accountVolumeForPosition = .05  # Allocate how much % of account to each trade.
                                # For example, 5% will allocate $5,000 to
                                # a $100,000 account. # It will then calculate
                                # the number of shares to buy or sell.
account = 100000

openOrderParams = trdk.OrderParams()
closeOrderParams = trdk.OrderParams()


# noinspection PyCallByClass,PyTypeChecker
class RangeWithBollingerBands(trdk.Strategy):

    def getRequiredSuppliers(self):
        return "BollingerBands"

    def onSecurityStart(self, security):
        # Only trade one position per symbol.
        assert hasattr(self, "security") is False
        self.security = security

    def onServiceDataUpdate(self, service):
        currentPoint = service.lastPoint
        self._pingLog(currentPoint)
        if hasattr(self, "prevPoint") is False:
            assert self.positions.count() == 0
            self.prevPoint = currentPoint
        elif self.prevPoint.source > self.prevPoint.high:
            if currentPoint.source <= self.prevPoint.high:
                self.checkIfPriceReturnsFromTop(currentPoint)
        elif self.prevPoint.source < self.prevPoint.low:
            if currentPoint.source >= self.prevPoint.high:
                self.checkIfPriceReturnsFromBottom(currentPoint)

    def checkIfPriceReturnsFromTop(self, currentPoint):
        # When price goes above higher band and back down the band, enter a
        # sell.
        assert hasattr(self, "prevPoint")
        self._updatePingTime()
        conditionStr = \
            '"prev. price {0} > prev. upper bound point {1}"' \
            ' and "curr. price {2} <= curr. upper bound point {3}"' \
            .format(
                self.security.descalePrice(self.prevPoint.source),
                self.security.descalePrice(self.prevPoint.high),
                self.security.descalePrice(currentPoint.source),
                self.security.descalePrice(currentPoint.high))
        if self.positions.count() > 0:
            self.closePositions(conditionStr)
        else:
            qty = self._calcQty(currentPoint)
            self.log.debug(
                'Opening short position {0}: (qty = {1})...'
                .format(conditionStr, qty))
            trdk.ShortPosition(self, self.security, qty, currentPoint.source)\
                .openAtMarketPrice(openOrderParams)

    def checkIfPriceReturnsFromBottom(self, currentPoint):
        # When price goes below lower band and price comes back above lower
        # band, enter a buy
        assert hasattr(self, "prevPoint")
        self._updatePingTime()
        conditionStr = \
            '"prev. price {0} < prev. lower bound point {1}"' \
            ' and "curr. price {2} >= curr. lower bound point {3}"' \
            .format(
                self.security.descalePrice(self.prevPoint.source),
                self.security.descalePrice(self.prevPoint.low),
                self.security.descalePrice(currentPoint.source),
                self.security.descalePrice(currentPoint.low))
        if self.positions.count() > 0:
            self.closePositions(conditionStr)
        else:
            qty = self._calcQty(currentPoint)
            self.log.debug(
                'Opening long position {0}: (qty = {1})...'
                .format(conditionStr, qty))
            trdk.LongPosition(self, self.security, qty, currentPoint.source)\
                .openAtMarketPrice(openOrderParams)

    def closePositions(self, condition):
        # Only trade one position per symbol:
        assert self.positions.count() == 1
        map(
            lambda position: self.checkPosition(position, condition),
            self.positions)

    def checkPosition(self, position, condition):
        if position.isOpened is False or position.hasActiveCloseOrders is True:
            return
        self.log.debug(
            'Closing {0} position: {1}...'
            .format(position.type, condition))
        position.closeAtMarketPrice(closeOrderParams)

    def _calcQty(self, point):
        lastPriceDescaled = self.security.descalePrice(point.source)
        volumeSource = account * accountVolumeForPosition
        qtySource = int(volumeSource / lastPriceDescaled)
        return int(qtySource / 100) * 100

    def _pingLog(self, currentPoint):
        now = time.time()
        hasLastLogPingTime = hasattr(self, 'lastLogPingTime')
        if hasLastLogPingTime is not False and now - self.lastLogPingTime < 60:
            return
        if hasattr(self, "prevPoint"):
            prevPointLastPrice = self.security.descalePrice(
                self.prevPoint.source)
            prevPointHighBb = self.security.descalePrice(
                self.prevPoint.high)
            prevPointLowBb = self.security.descalePrice(
                self.prevPoint.low)
        else:
            prevPointLastPrice \
                = prevPointHighBb \
                = prevPointLowBb \
                = 'None'
        self.log.debug(
            'Ping: prev = {1} / {0} / {2}; current = {4} / {3} / {5};'
            .format(
                prevPointLastPrice,
                prevPointHighBb,
                prevPointLowBb,
                self.security.descalePrice(currentPoint.source),
                self.security.descalePrice(currentPoint.high),
                self.security.descalePrice(currentPoint.low)))
        self._updatePingTime()

    def _updatePingTime(self):
        self.lastLogPingTime = time.time()
