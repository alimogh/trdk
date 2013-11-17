import trdk


accountVolumeForPosition = .05  # Allocate how much % of account to each trade.
                                # For example, 5% will allocate $5,000 to
                                # a $100,000 account. # It will then calculate
                                # the number of shares to buy or sell.

openOrderParams = trdk.OrderParams()
closeOrderParams = trdk.OrderParams()


# noinspection PyCallByClass,PyTypeChecker
class MovingAverage(trdk.Strategy):

    movingAverage = None
    account = 100000
    skipsCount = 0

    def getRequiredSuppliers(self):
        return 'Level 1 Updates, MovingAverage'

    def onServiceStart(self, service):
        self.movingAverage = service

    def onServiceDataUpdate(self, service):
        pass

    def onLevel1Update(self, security):

        if self.movingAverage.isEmpty:
            return

        # Only trade one position per symbol:
        if self.positions.count() == 0:
            self.checkEntry(security)
        else:
            assert self.positions.count() == 1
            map(
                lambda position: self.checkPosition(position),
                self.positions)

    def checkEntry(self, security):

        lastPrice = security.lastPrice
        movingAverage = int(self.movingAverage.lastPoint.value)

        # Application will buy only when price is above Moving Average
        if lastPrice <= movingAverage:
            if ++self.skipsCount > 20:
                self.log.debug(
                    'Still no suitable prices for positions opening'
                    ' (last price: {0}, moving average: {1})...'
                    .format(
                        security.descalePrice(lastPrice),
                        security.descalePrice(movingAverage)))
                self.skipsCount = 0
            return
        self.skipsCount = 0

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
        trdk.LongPosition(self, security, qty, lastPrice).openAtMarketPrice(
            openOrderParams)

    def checkPosition(self, position):

        if position.isOpened is False or position.hasActiveCloseOrders is True:
            return

        lastPrice = position.security.lastPrice
        movingAverage = int(self.movingAverage.lastPoint.value)

        # Sell only when price is below MA.
        if lastPrice >= movingAverage:
            if ++self.skipsCount > 20:
                self.log.debug(
                    'Still no suitable prices for positions closing'
                    ' (last price: {0}, moving average: {1})...'
                    .format(
                        position.security.descalePrice(lastPrice),
                        position.security.descalePrice(movingAverage)))
                self.skipsCount = 0
            return
        self.skipsCount = 0

        self.log.debug(
            'Closing position: "last price {0}" < "moving average {1}"...'
            .format(
                position.security.descalePrice(lastPrice),
                position.security.descalePrice(movingAverage)))
        position.closeAtMarketPrice(closeOrderParams)
