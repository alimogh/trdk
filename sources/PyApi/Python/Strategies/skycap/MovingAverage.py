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


accountVolumeForPosition = .05  # Allocate how much % of account to each trade.
                                # For example, 5% will allocate $5,000 to
                                # a $100,000 account. # It will then calculate
                                # the number of shares to buy or sell.

openOrderParams = trdk.OrderParams()
closeOrderParams = trdk.OrderParams()


# noinspection PyCallByClass,PyTypeChecker
class MovingAverage(trdk.Strategy):

    def __init__(self, param):
        super(self.__class__, self).__init__(param)
        self.account = 100000

    def getRequiredSuppliers(self):
        return 'Level 1 Updates, MovingAverage'

    def onServiceStart(self, service):
        self.movingAverage = service

    def onServiceDataUpdate(self, service):
        # required to mark service as "used"
        pass

    def onLevel1Update(self, security):
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

        if lastPrice <= movingAverage:
            # Application will buy only when price is above Moving Average
            return

        lastPriceDescaled = security.descalePrice(lastPrice)
        volumeSource = self.account * accountVolumeForPosition
        qtySource = int(volumeSource / lastPriceDescaled)
        qty = int(qtySource / 100) * 100
        volume = qty * lastPriceDescaled

        self.log.debug(
            'Opening position: "last price {0}" > "moving average {1}"'
            ' (using volume: {2} -> {3}, qty: {4} -> {5})...'
            .format(
                lastPriceDescaled,
                security.descalePrice(movingAverage),
                volumeSource, volume,
                qtySource, qty))
        trdk.LongPosition(self, security, qty, lastPrice)\
            .openAtMarketPrice(openOrderParams)

        self._updatePingTime()

    def checkPosition(self, position):

        if position.isOpened is False or position.hasActiveCloseOrders is True:
            return

        lastPrice = position.security.lastPrice
        movingAverage = int(self.movingAverage.lastPoint.value)

        if lastPrice >= movingAverage:
            # Sell only when price is below MA.
            return

        self.log.debug(
            'Closing position: "last price {0}" < "moving average {1}"...'
            .format(
                position.security.descalePrice(lastPrice),
                position.security.descalePrice(movingAverage)))
        position.closeAtMarketPrice(closeOrderParams)

        self._updatePingTime()

    def _pingLog(self, security):
        now = time.time()
        hasLastLogPingTime = hasattr(self, 'lastLogPingTime')
        if hasLastLogPingTime is False or now - self.lastLogPingTime >= 60:
            if self.movingAverage.isEmpty:
                maStr = 'None'
            else:
                maStr = self.movingAverage.lastPoint.value
                maStr = security.descalePrice(maStr)
            self.log.debug(
                'Ping: last price = {0}, ma = {1};'
                .format(security.descalePrice(security.lastPrice), maStr))
            self._updatePingTime()

    def _updatePingTime(self):
        self.lastLogPingTime = time.time()