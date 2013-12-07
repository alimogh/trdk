# TRDK-21:
#
# I would like to add an technical indicator filter: Moving Averages (Choice
# of simple, exponential, smoothed, hull).
#
# Application will buy only when price is above Moving Average
# (I can input MA period, and time frame 15min, 1 hr, 4 hr, daily, weekly).
#
# Sell only when price is below MA.
#
# Only trade one position per symbol.

import trdk
import time
from Util import *

openOrderParams = trdk.OrderParams()
closeOrderParams = trdk.OrderParams()


# noinspection PyCallByClass,PyTypeChecker
class MovingAverage(trdk.Strategy):

    def __init__(self, param):
        super(self.__class__, self).__init__(param)
        self.movingAverage = None
        self.prevMovingAverageValue = 0
        self.lastLogPingTime = None

    def getRequiredSuppliers(self):
        return 'Level 1 Updates, MovingAverage'

    def onServiceStart(self, service):
        self.movingAverage = service

    def onServiceDataUpdate(self, service):
        # required to mark service as "used"
        pass

    def onLevel1Update(self, security):
        updateContext(self)
        self._pingLog(security)
        if self.movingAverage.isEmpty:
            return
        if self.positions.count() == 0:
            self.checkEntry(security)
        else:
            # Only trade one position per symbol:
            assert self.positions.count() == 1
            map(
                lambda position: self.checkPosition(position),
                self.positions)

    def checkEntry(self, security):

        lastPrice = security.lastPrice
        movingAverage = int(self.movingAverage.lastPoint.value)
        movingAveragePrev = self.prevMovingAverageValue
        self.prevMovingAverageValue = movingAverage

        if lastPrice <= movingAverage:
            # Application will buy only when price is above Moving Average
            return
        elif movingAverage - movingAveragePrev <= 0 or movingAveragePrev == 0:
            # 1. "Also, for the MA strategy, would you be able to add another
            # option to buy only if MA is sloping up and sell if MA is sloping
            # down." "Yes something simple is fine."
            return

        self._updatePingTime()

        lastPriceDescaled = security.descalePrice(lastPrice)
        accountVolumeForPosition \
            = float(self.context.params.account_volume_for_position)
        volumeSource = getFullBalance(self) * accountVolumeForPosition
        qtySource = int(volumeSource / lastPriceDescaled)
        qty = int(round(float(qtySource) / 100) * 100)
        volume = qty * lastPriceDescaled

        self.log.debug(
            'Opening {0} position:'
            ' "last price {1}" > "moving average {2} -> {3}"'
            ' (using volume: {4} -> {5}, qty: {6} -> {7}'
            ', cash: {8}, el: {9})...'
            .format(
                security.symbol,
                lastPriceDescaled,
                security.descalePrice(movingAveragePrev),
                security.descalePrice(movingAverage),
                volumeSource, volume,
                qtySource, qty,
                self.context.tradeSystem.cashBalance,
                self.context.tradeSystem.excessLiquidity))
        if qty <= 0:
            self.log.debug(
                "Can't open position: too small account volume for this price")
            return

        if checkAccount(self, volume) is True:
            pos = trdk.LongPosition(self, security, qty, lastPrice)
            pos.openAtMarketPriceWithStopPrice(movingAverage, openOrderParams)

    def checkPosition(self, position):

        if self.context.params.ma_do_not_close_position == 'yes' \
                or isActualForClosingPositionUpdate(position) is False:
            return

        lastPrice = position.security.lastPrice
        movingAverage = int(self.movingAverage.lastPoint.value)
        movingAveragePrev = self.prevMovingAverageValue
        self.prevMovingAverageValue = movingAverage

        if lastPrice >= movingAverage:
            # Sell only when price is below MA.
            return
        elif movingAverage - movingAveragePrev >= 0:
            # 1. "Also, for the MA strategy, would you be able to add another
            # option to buy only if MA is sloping up and sell if MA is sloping
            # down." "Yes something simple is fine."
            return

        self._updatePingTime()

        self.log.debug(
            'Closing {0} position:'
            ' "last price {1}" < "moving average {2} -> {3}"...'
            .format(
                position.security.symbol,
                position.security.descalePrice(lastPrice),
                position.security.descalePrice(movingAveragePrev),
                position.security.descalePrice(movingAverage)))
        position.closeAtMarketPrice(closeOrderParams)

    def _pingLog(self, security):
        now = time.time()
        if self.lastLogPingTime is not None:
            if now - self.lastLogPingTime < 60 * 10:
                return
        if self.movingAverage.isEmpty:
            maStr = 'None'
        else:
            maStr = self.movingAverage.lastPoint.value
            maStr = security.descalePrice(maStr)
        self.log.debug(
            'Ping {0}: price = {1}, ma = {2} -> {3}; cash = {4}; el = {5};'
            .format(
                security.symbol,
                security.descalePrice(security.lastPrice),
                security.descalePrice(self.prevMovingAverageValue),
                maStr,
                self.context.tradeSystem.cashBalance,
                self.context.tradeSystem.excessLiquidity))
        self._updatePingTime()

    def _updatePingTime(self):
        self.lastLogPingTime = time.time()