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
from Util import *

openOrderParams = trdk.OrderParams()
closeOrderParams = trdk.OrderParams()


# noinspection PyCallByClass,PyTypeChecker
class RangeWithBollingerBands(trdk.Strategy):

    class Point:
        def __init__(self, point):
            self.source = point.source
            self.high = point.high
            self.low = point.low

    def __init__(self, param, tradeOnNewBar):
        trdk.Strategy.__init__(self, param)
        self.tradeOnNewBar = tradeOnNewBar
        self.security = None
        self.prevPoint = None
        self.currentPoint = None
        self.lastLogPingTime = None

    def getRequiredSuppliers(self):
        result = "BollingerBands"
        if self.tradeOnNewBar is False:
            result += ", Level 1 Updates"
        return result

    def onSecurityStart(self, security):
        # Only trade one position per symbol.
        assert self.security is None
        self.security = security

    def onServiceDataUpdate(self, service):
        updateContext(self)
        if self.currentPoint is None:
            if self.prevPoint is None:
                self.prevPoint \
                    = RangeWithBollingerBands.Point(service.lastPoint)
                return
        else:
            self.prevPoint = self.currentPoint
        self.currentPoint = RangeWithBollingerBands.Point(service.lastPoint)
        if self.tradeOnNewBar:
            self.checkSignal()

    def onLevel1Update(self, security):
        if self.tradeOnNewBar is True or self.currentPoint is None:
            return
        self.prevPoint = self.currentPoint
        self.currentPoint.source = security.lastPrice
        self.checkSignal()

    def checkSignal(self):
        self._pingLog()
        if self.prevPoint.source > self.prevPoint.high:
            if self.currentPoint.source < self.prevPoint.high:
                self.checkIfPriceReturnsFromTop(self.currentPoint)
        elif self.prevPoint.source < self.prevPoint.low:
            if self.currentPoint.source > self.prevPoint.low:
                self.checkIfPriceReturnsFromBottom(self.currentPoint)

    def checkIfPriceReturnsFromTop(self, currentPoint):
        # When price goes above higher band and back down the band, enter a
        # sell.
        self._updatePingTime()
        conditionStr = \
            '"prev. price {0} > prev. upper bound point {1}"' \
            ' and "curr. price {2} < curr. upper bound point {3}"' \
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
                'Opening {0} short position {1}: (qty = {2}, cash = {3})...'
                .format(
                    self.security.symbol,
                    conditionStr,
                    qty,
                    self.context.tradeSystem.cashBalance))
            if qty <= 0:
                self.log.debug(
                    "Can't open position:"
                    " too small account volume for this price")
                return
            trdk.ShortPosition(self, self.security, qty, currentPoint.source)\
                .openAtMarketPrice(openOrderParams)

    def checkIfPriceReturnsFromBottom(self, currentPoint):
        # When price goes below lower band and price comes back above lower
        # band, enter a buy
        self._updatePingTime()
        conditionStr = \
            '"prev. price {0} < prev. lower bound point {1}"' \
            ' and "curr. price {2} > curr. lower bound point {3}"' \
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
                'Opening {0} long position {1}: (qty = {2}'
                ', cash = {3}, el = {4})...'
                .format(
                    self.security.symbol,
                    conditionStr,
                    qty,
                    self.context.tradeSystem.cashBalance,
                    self.context.tradeSystem.excessLiquidity))
            if qty <= 0:
                self.log.debug(
                    "Can't open position:"
                    " too small account volume for this price")
                return
            if checkAccount(self, qty * currentPoint.source) is False:
                return
            trdk.LongPosition(self, self.security, qty, currentPoint.source)\
                .openAtMarketPrice(openOrderParams)

    def closePositions(self, condition):
        # Only trade one position per symbol:
        assert self.positions.count() == 1
        map(
            lambda position: self.checkPosition(position, condition),
            self.positions)

    def checkPosition(self, position, condition):
        if isActualForClosingPositionUpdate(position) is False:
            return
        self.log.debug(
            'Closing {0} {1} position: {2}...'
            .format(self.security.symbol, position.type, condition))
        position.closeAtMarketPrice(closeOrderParams)

    def _calcQty(self, point):
        lastPriceDescaled = self.security.descalePrice(point.source)
        accountVolumeForPosition\
            = float(self.context.params.account_volume_for_position)
        volumeSource = getFullBalance(self) * accountVolumeForPosition
        qtySource = int(volumeSource / lastPriceDescaled)
        qty = int(round(float(qtySource) / 100) * 100)
        return qty

    def _pingLog(self):
        now = time.time()
        if self.lastLogPingTime is not None:
            if now - self.lastLogPingTime < 60 * 10:
                return
        self.log.debug(
            'Ping {7}: prev = {1} / {0} / {2};'
            ' current = {4} / {3} / {5};'
            ' cash = {6}; el = {8};'
            .format(
                self.security.descalePrice(self.prevPoint.source),
                self.security.descalePrice(self.prevPoint.high),
                self.security.descalePrice(self.prevPoint.low),
                self.security.descalePrice(self.currentPoint.source),
                self.security.descalePrice(self.currentPoint.high),
                self.security.descalePrice(self.currentPoint.low),
                self.context.tradeSystem.cashBalance,
                self.security.symbol,
                self.context.tradeSystem.excessLiquidity))
        self._updatePingTime()

    def _updatePingTime(self):
        self.lastLogPingTime = time.time()


class RangeWithBollingerBandsByBar(RangeWithBollingerBands):
    def __init__(self, param):
        RangeWithBollingerBands.__init__(self, param, True)


class RangeWithBollingerBandsByTick(RangeWithBollingerBands):
    def __init__(self, param):
        RangeWithBollingerBands.__init__(self, param, False)
