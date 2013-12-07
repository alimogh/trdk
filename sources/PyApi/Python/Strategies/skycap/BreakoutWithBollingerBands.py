# TRDK-23
# There will be two watchlists, a buy watchlist and a sell watchlist. In the
# buy watchlist, I would like the program to buy when price is at the lower
# band or middle line. After that, if any candle closes below the lower band,
# then I want it to exit (I want there to be an option to turn this on/off).
# If not, then keep the position. Same goes for sell watchlist except it will
# use the upper band.
#
# if bar closes above upper band, buy (if already in a sell position, exit sell
# and then buy).
#
# if bar closes below lower band, sell.
#
# I want to input time frame to use, i.e. 15 min, 1 hour, 4 hour, daily, or
# weekly
#
# Only trade one position per symbol.

import trdk
import time
from Util import *

openOrderParams = trdk.OrderParams()
closeOrderParams = trdk.OrderParams()


# noinspection PyCallByClass,PyTypeChecker
class BreakoutWithBollingerBands(trdk.Strategy):

    def getRequiredSuppliers(self):
        return 'BollingerBands'

    def onSecurityStart(self, security):
        # Only trade one position per symbol.
        assert hasattr(self, 'security') is False
        self.security = security

    def onServiceDataUpdate(self, service):
        updateContext(self)
        self._pingLog(service)
        if self.positions.count() == 0:
            # Only trade one position per symbol:
            self.checkEntry(service.lastPoint)
        else:
            # Only trade one position per symbol:
            assert self.positions.count() == 1
            map(
                lambda pos: self.checkPosition(pos, service.lastPoint),
                self.positions)

    def checkEntry(self, point):

        decision = self._makeEntryDecision(point)
        if decision == 0:
            return

        lastPriceDescaled = self.security.descalePrice(point.source)
        accountVolumeForPosition\
            = float(self.context.params.account_volume_for_position)
        volumeSource = getFullBalance(self) * accountVolumeForPosition
        qtySource = int(volumeSource / lastPriceDescaled)
        qty = int(round(float(qtySource) / 100) * 100)
        volume = qty * lastPriceDescaled

        if decision > 0:
            decisionStr = 'long position: "last price {0}"'\
                ' > "upper band point {1}"...'
            pos = trdk.LongPosition(self, self.security, qty, point.source)
        else:
            assert decision < 0
            decisionStr = 'short position: "last price {0}"'\
                ' < "lower band point {1}"...'
            pos = trdk.ShortPosition(self, self.security, qty, point.source)

        decisionStr = decisionStr.format(
            lastPriceDescaled,
            self.security.descalePrice(point.high))
        self.log.debug(
            'Opening {0} {1} (volume {2} -> {3}, qty {4} -> {5},'
            ' cash = {6})...'
            .format(
                self.security.symbol,
                decisionStr,
                volumeSource, volume,
                qtySource, qty,
                self.context.tradeSystem.cashBalance))
        if qty <= 0:
            self.log.debug(
                "Can't open position: too small account volume for this price")
            self._updatePingTime()
            return

        if decision < 0 or checkAccount(self, volume) is True:
            pos.openAtMarketPrice(openOrderParams)

        self._updatePingTime()

    def checkPosition(self, position, point):
        if isActualForClosingPositionUpdate(position) is False:
            return
        if self._makeExitDecision(point) is True:
            position.closeAtMarketPrice(closeOrderParams)
            self._updatePingTime()

    def _pingLog(self, service):
        now = time.time()
        hasLastLogPingTime = hasattr(self, 'lastLogPingTime')
        if hasLastLogPingTime is True:
            if now - self.lastLogPingTime < 60 * 10:
                return
        self.log.debug(
            'Ping {4}: price = {1} / {0} / {2}; cash = {3};'
            .format(
                self.security.descalePrice(service.lastPoint.source),
                self.security.descalePrice(service.lastPoint.high),
                self.security.descalePrice(service.lastPoint.low),
                self.context.tradeSystem.cashBalance,
                self.security.symbol))
        self._updatePingTime()

    def _updatePingTime(self):
        self.lastLogPingTime = time.time()


# There will be two watchlists, a buy watchlist and a sell watchlist. In the
# buy watchlist,
# noinspection PyCallByClass,PyTypeChecker
class BreakoutWithBollingerBandsBuy(BreakoutWithBollingerBands):

    def _makeEntryDecision(self, point):
        if point.source > point.high:
            # if bar closes above upper band, buy
            return +1
        else:
            # this is only buy-watchlist
            return 0

    def _makeExitDecision(self, point):
        # if bar closes below lower band, sell.
        if point.source < point.low:
            self.log.debug(
                'Closing {0} long position: "last price {1}"'
                ' < "lower band point {2}"...'
                .format(
                    self.security.symbol,
                    self.security.descalePrice(point.source),
                    self.security.descalePrice(point.low)))
            return True
        else:
            return False


# There will be two watchlists, a buy watchlist and a sell watchlist. In the
# buy watchlist,
# noinspection PyCallByClass,PyTypeChecker
class BreakoutWithBollingerBandsSell(BreakoutWithBollingerBands):

    def _makeEntryDecision(self, point):
        if point.source < point.low:
            return -1
        else:
            # this is only sell watchlist
            return 0

    def _makeExitDecision(self, point):
        if point.source > point.high:
            self.log.debug(
                'Closing {0} short position: "last price {1}"'
                ' > "upper band point {2}"...'
                .format(
                    self.security.symbol,
                    self.security.descalePrice(point.source),
                    self.security.descalePrice(point.high)))
            return True
        else:
            return False
