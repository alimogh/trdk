

import trdk
import time


ratio = 2.850
positionVolume = 20000


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

        gld = self.gldBars.getBarByReversedIndex(0)
        dgl = self.dglBars.getBarByReversedIndex(0)

        if self.positions.count() > 0:
            return

        shortGldLongDlgRatio = 0
        shortGldLongDlg = False
        # When GLD (ask)/DGL (bid) > ratio * 1.001% -> Short GLD, Long DGL
        if dgl.minBidPrice != 0:
            shortGldLongDlgRatio\
                = float(gld.maxAskPrice) / float(dgl.minBidPrice)
            shortGldLongDlg = shortGldLongDlgRatio > (ratio * (1.001 / 100))

        longGldShortDglRatio = 0
        longGldShortDgl = False
        if dgl.maxAskPrice != 0:
            # When GLD (bid)/DGL (ask) < ratio * 0.999% -> Long GLD, Short DGL
            longGldShortDglRatio\
                = float(gld.minBidPrice) / float(dgl.maxAskPrice)
            longGldShortDgl = longGldShortDglRatio < (ratio * (0.999 / 100))

        self.log.debug(
            "Entry 1: GLD(Ask {0}) / DGL(Bid {1}) = {2} - {3};"
            " Entry 2: GLD(Bid {4}) / DGL(Ask {5}) = {6} - {7};"
                .format(
                    self.findSecurity('GLD').descalePrice(gld.maxAskPrice),
                    self.findSecurity('DGL').descalePrice(dgl.minBidPrice),
                    shortGldLongDlgRatio,
                    shortGldLongDlg,
                    self.findSecurity('GLD').descalePrice(gld.minBidPrice),
                    self.findSecurity('DGL').descalePrice(dgl.maxAskPrice),
                    longGldShortDglRatio,
                    longGldShortDgl))

        def calcQty(price):
            return positionVolume / price

        if shortGldLongDlg:
            self.log.info('Opening positions "Short GLD, Long DGL"...')
            gldPos = trdk.ShortPosition(
                self,
                self.findSecurity('GLD'),
                calcQty(gld.maxAskPrice),
                gld.maxAskPrice)
            dglPos = trdk.LongPosition(
                self,
                self.findSecurity('DGL'),
                calcQty(dgl.minBidPrice),
                dgl.minBidPrice)
            gldPos.openAtMarketPrice()
            dglPos.openAtMarketPrice()
#            gldPos.open(gldPos.openStartPrice)
#            dglPos.open(dglPos.openStartPrice)
        
        if longGldShortDgl:
            self.log.info('Opening positions "Long GLD, Short DGL"...')
            gldPos = trdk.LongPosition(
                self,
                self.findSecurity('GLD'),
                calcQty(gld.minBidPrice),
                gld.minBidPrice)
            dglPos = trdk.ShortPosition(
                self,
                self.findSecurity('DGL'),
                calcQty(dgl.maxAskPrice),
                dgl.maxAskPrice)
            gldPos.openAtMarketPrice()
            dglPos.openAtMarketPrice()
#            gldPos.open(gldPos.openStartPrice)
#            dglPos.open(dglPos.openStartPrice)

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
            position.closeAtMarketPrice()
#            position.close(closePrice)
