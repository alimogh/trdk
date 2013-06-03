

import trdk
import time


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
#        if self.checkOnline() is False:
#            return
        self.checkEntry()

    def checkOnline(self):
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

    def checkEntry(self):

        gld = self.gldBars.getBarByReversedIndex(0)
        dgl = self.dglBars.getBarByReversedIndex(0)

        if self.positions.count() > 0:
            self.log.debug('Has open positions...')
            return

        ratioGe = 0
        if dgl.minBidPrice != 0:
            ratioGe = float(gld.maxAskPrice) / float(dgl.minBidPrice)

        ratioLe = 0
        if dgl.maxAskPrice != 0:
            ratioLe = float(gld.minBidPrice) / float(dgl.maxAskPrice)

        self.log.debug(
            "Entry 1: GLD(Ask {0}) / DGL(Bid {1}) = {2};"
            + " Entry 2: GLD(Bid {3}) / DGL(Ask {4}) = {5};"
                .format(
                    self.findSecurity('GLD').descalePrice(gld.maxAskPrice),
                    self.findSecurity('DGL').descalePrice(dgl.minBidPrice),
                    ratioGe,
                    self.findSecurity('GLD').descalePrice(gld.minBidPrice),
                    self.findSecurity('DGL').descalePrice(dgl.maxAskPrice),
                    ratioLe))

        # If opening of 5 minute candlestick GLD(Ask)/DGL(BID)>=2.850 .. 
        if ratioGe >= 2.850:
            # Short GLD @ Ask, Long DGL @ Bid
            self.log.info('Opening positions by "Entry 1"...')
            gldPos = trdk.ShortPosition(
                self,
                self.findSecurity('GLD'),
                1,
                gld.maxAskPrice)
            dglPos = trdk.LongPosition(
                self,
                self.findSecurity('DGL'),
                1,
                dgl.minBidPrice)
            gldPos.openAtMarketPrice()
            dglPos.openAtMarketPrice()
#            gldPos.open(gldPos.openStartPrice)
#            dglPos.open(dglPos.openStartPrice)
        
        # If opening of 5 minute candlestick GLD(Bid)/DGL(Ask)<=2.842 .. 
        if 0 < ratioLe <= 2.842:
            # Long GLD @ Bid, Short DGL @ Ask
            self.log.info('Opening positions by "Entry 2"...')
            gldPos = trdk.LongPosition(
                self,
                self.findSecurity('GLD'),
                1,
                gld.minBidPrice)
            dglPos = trdk.ShortPosition(
                self,
                self.findSecurity('DGL'),
                1,
                dgl.maxAskPrice)
            gldPos.openAtMarketPrice()
            dglPos.openAtMarketPrice()
#            gldPos.open(gldPos.openStartPrice)
#            dglPos.open(dglPos.openStartPrice)

    def onPositionUpdate(self, position):
        if position.isOpened:
            # Profit Targets:
            # Sell positions @ GLD/DGL ratio of 2.847
            closePrice = int(position.openStartPrice * 2.847)
            self.log.debug(
                'Closing {0} with {1}.'
                    .format(
                        position.security.symbol,
                        position.security.descalePrice(closePrice)))
            position.close(closePrice)
        elif position.isClosed:
            # Risk Management:
            # If one order gets filled without the other, sell
            # immediately at market order.
            self.log.debug(
                'Closing all at market price as {0} closed.'
                    .format(position.security.symbol))
            map(
                lambda position: position.cancelAtMarketPrice(),
                self.positions)
