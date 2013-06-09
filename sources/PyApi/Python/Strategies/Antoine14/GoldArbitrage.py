

import trdk
import time


ratio = 2.850
positionVolume = 20000
orderDisplaySize = 3


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
            return security.scalePrice(positionVolume) / price

        if shortGldLongDlg:
            self.log.info('Opening positions "Short GLD, Long DGL"...')
            gldPos = trdk.ShortPosition(
                self,
                gld,
                calcQty(gld, gldBar.maxAskPrice),
                gldBar.maxAskPrice)
            dglPos = trdk.LongPosition(
                self,
                dgl,
                calcQty(dgl, dglBar.minBidPrice),
                dglBar.minBidPrice)
            gldPos.open(gldPos.openStartPrice, orderDisplaySize)
            dglPos.open(dglPos.openStartPrice, orderDisplaySize)
        
        if longGldShortDgl:
            self.log.info('Opening positions "Long GLD, Short DGL"...')
            gldPos = trdk.LongPosition(
                self,
                gld,
                calcQty(gld, gldBar.minBidPrice),
                gldBar.minBidPrice)
            dglPos = trdk.ShortPosition(
                self,
                dgl,
                calcQty(dgl, dglBar.maxAskPrice),
                dglBar.maxAskPrice)
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
            # Sell positions @ GLD/DGL ratio of 2.847
            closePrice = int(position.openStartPrice * 2.847)
            self.log.info(
                'Closing {0} with {1}.'
                    .format(
                        position.security.symbol,
                        position.security.descalePrice(closePrice)))
            position.close(closePrice, orderDisplaySize)
