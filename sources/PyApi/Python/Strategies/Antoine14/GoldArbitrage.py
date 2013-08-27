

import trdk
import time


ratio = 2.860
positionVolume = 20000
orderDisplaySize = 30


# noinspection PyCallByClass,PyTypeChecker
class GoldArbitrage(trdk.Strategy):

    gldBars = None
    dglBars = None

    def onServiceStart(self, service):
        security = service.securities.find('GLD')
        if security is not None:
            assert self.gldBars is None
            self.gldBars = service
            return
        security = service.securities.find('DGL')
        if security is not None:
            assert self.dglBars is None
            self.dglBars = service
            return
        assert False

    def onServiceDataUpdate(self, service):
        if self.positions.count() > 0:
            return
        if self._checkOnline() is False:
            return
        self._checkEntry()

    def _checkOnline(self):
        if self.gldBars.size < 1 or self.dglBars.size < 1:
            return False
        gld = self.gldBars.getBarByReversedIndex(0)
        dgl = self.dglBars.getBarByReversedIndex(0)
        # Trade restrictions:
        # Obviously if DGL and/or GLD are not open for trading then do not
        # trade. Possible that GLD opens before DGL in premarket trading
        isOnline = gld.time == dgl.time
        self.log.debug(
            'Online check: GLD({0}) == DGL({1}) = {2}'
                .format(
                    time.ctime(gld.time),
                    time.ctime(dgl.time),
                    isOnline))
        return isOnline

    def _checkEntry(self):

        gld = self.findSecurity('GLD')
        gldBar = self.gldBars.getBarByReversedIndex(0)

        dgl = self.findSecurity('DGL')
        dglBar = self.dglBars.getBarByReversedIndex(0)

        shortGldLongDlgRatio = 0
        shortGldLongDlg = False
        # When GLD (ask)/DGL (bid) > ratio * 1.001% -> Short GLD, Long DGL
        if dglBar.minBidPrice != 0:
            shortGldLongDlgRatio\
                = float(gldBar.maxAskPrice) / float(dglBar.minBidPrice)
            shortGldLongDlg = shortGldLongDlgRatio > (ratio * (1.001 / 100))

        longGldShortDglRatio = 0
        longGldShortDgl = False
        if dglBar.maxAskPrice != 0:
            # When GLD (bid)/DGL (ask) < ratio * 0.999% -> Long GLD, Short DGL
            longGldShortDglRatio\
                = float(gldBar.minBidPrice) / float(dglBar.maxAskPrice)
            longGldShortDgl = longGldShortDglRatio < (ratio * (0.999 / 100))

        self.log.debug(
            "Entry 1: GLD(Ask {0}) / DGL(Bid {1}) = {2} - {3};"
            " Entry 2: GLD(Bid {4}) / DGL(Ask {5}) = {6} - {7};"
                .format(
                    gld.descalePrice(gldBar.maxAskPrice),
                    dgl.descalePrice(dglBar.minBidPrice),
                    shortGldLongDlgRatio,
                    shortGldLongDlg,
                    gld.descalePrice(gldBar.minBidPrice),
                    dgl.descalePrice(dglBar.maxAskPrice),
                    longGldShortDglRatio,
                    longGldShortDgl))

        def calcQty(security, price):
            result = security.scalePrice(positionVolume) / price
            # Resolving error: "The Display Size should be a multiple of the
            # round lot size for this security:
            result -= result % 100
            assert result % 100 == 0
            return result

        if shortGldLongDlg:
            gldQty = calcQty(gld, gldBar.maxAskPrice)
            dglQty = calcQty(dgl, dglBar.minBidPrice)
            self.log.info(
                'Opening positions "Short GLD {0}, Long DGL {1}"'
                ' (volume: {2}, visible qty: {3})...'
                    .format(gldQty, dglQty, positionVolume, orderDisplaySize))
            gldPos = trdk.ShortPosition(self, gld, gldQty, gldBar.maxAskPrice)
            dglPos = trdk.LongPosition(self, dgl, dglQty, dglBar.minBidPrice)
            gldPos.open(gldPos.openStartPrice, orderDisplaySize)
            dglPos.open(dglPos.openStartPrice, orderDisplaySize)
        
        if longGldShortDgl:
            gldQty = calcQty(gld, gldBar.minBidPrice)
            dglQty = calcQty(dgl, dglBar.maxAskPrice)
            self.log.info(
                'Opening positions "Short GLD {0}, Long DGL {1}"'
                ' (volume: {2}, visible qty: {3})...'
                    .format(gldQty, dglQty, positionVolume, orderDisplaySize))
            gldPos = trdk.LongPosition(self, gld, gldQty, gldBar.minBidPrice)
            dglPos = trdk.ShortPosition(self, dgl, dglQty, dglBar.maxAskPrice)
            gldPos.open(gldPos.openStartPrice, orderDisplaySize)
            dglPos.open(dglPos.openStartPrice, orderDisplaySize)

    def onPositionUpdate(self, position):
        if position.isCompleted:
            if position.isCanceled is not True:
                self.log.info(
                    '{0} closed first.'.format(position.security.symbol))
            # Risk Management:
            # If one order gets filled without the other, sell
            # immediately at market order.
            map(
                lambda position: position.cancelAtMarketPrice(),
                self.positions)
        elif position.isOpened and position.hasActiveCloseOrders is False:
            # Profit Targets:
            # Sell positions @ GLD/DGL ratio of ratio
            closePrice = int(position.openStartPrice * ratio)
            self.log.info(
                'Closing {0} with {1}.'
                    .format(
                        position.security.symbol,
                        position.security.descalePrice(closePrice)))
            position.close(closePrice, orderDisplaySize)
