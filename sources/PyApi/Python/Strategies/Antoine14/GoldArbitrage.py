

import trdk
import time


strategies = dict()


class GoldArbitrage(trdk.Strategy):

    bars = None


    def __init__(self, param):
        super(self.__class__, self).__init__(param)
        strategies[self.security.symbol] = self


    def onServiceStart(self, service):
        self.bars = service


    def onServiceDataUpdate(self, service):
        oppName = "GLD"
        if self.security.symbol == "GLD":
            oppName = "DGL"
        oppService = strategies[oppName].bars
        if oppService.size < 1:
            return
        lastBar = service.getBarByReversedIndex(0)
        oppLastBar = oppService.getBarByReversedIndex(0)
        # Trade restrictions:
        # Obviously if DGL and/or GLD are not open for trading then do not trade.
        # Possible that GLD opens before DGL in premarket trading
        isOnline = oppLastBar.time == lastBar.time
        self.log.debug(
            "Online check: {0}({1}) == {2}({3}) = {4}"
                .format(
                    self.security.symbol,
                    time.ctime(lastBar.time),
                    oppName,
                    time.ctime(oppLastBar.time),
                    isOnline))
        if isOnline == True:
            if service.security.symbol == "GLD":
                strategies["GLD"].checkEntry(lastBar, oppLastBar)
            else:
                strategies["GLD"].checkEntry(oppLastBar, lastBar)


    def checkEntry(self, gld, dgl):

        if self.positions.count() > 0:
            self.log.debug("Has open positions...")
            return

        ratioGe = 0
        if dgl.minBidPrice != 0:
            ratioGe = float(gld.maxAskPrice) / float(dgl.minBidPrice)

        ratioLe = 0
        if dgl.maxAskPrice != 0:
            ratioLe = float(gld.minBidPrice) / float(dgl.maxAskPrice)

        self.log.debug(
            "Entry 1: GLD(Ask {0}) / DGL(Bid {1}) = {2}; Entry 2: GLD(Bid {3}) / DGL(Ask {4}) = {5};"
                .format(
                    self.security.descalePrice(gld.maxAskPrice), self.security.descalePrice(dgl.minBidPrice), ratioGe,
                    self.security.descalePrice(gld.minBidPrice), self.security.descalePrice(dgl.maxAskPrice), ratioLe))

        # If opening of 5 minute candlestick GLD(Ask)/DGL(BID)>=2.850 .. 
        if ratioGe >= 2.850:
            # Short GLD @ Ask, Long DGL @ Bid
            self.log.info('Opening positions by "Entry 1"...')
            gldPos = trdk.ShortPosition(self, 1, gld.maxAskPrice)
            dglPos = trdk.LongPosition(self, 1, dgl.minBidPrice)
            gldPos.openAtMarketPrice()
            dglPos.openAtMarketPrice()
#            gldPos.open(gldPos.openStartPrice)
#            dglPos.open(dglPos.openStartPrice)
        
        # If opening of 5 minute candlestick GLD(Bid)/DGL(Ask)<=2.842 .. 
        if ratioLe > 0 and ratioLe <= 2.842:
            # Long GLD @ Bid, Short DGL @ Ask
            self.log.info('Opening positions by "Entry 2"...')
            gldPos = trdk.LongPosition(self, 1, gld.minBidPrice)
            dglPos = trdk.ShortPosition(self, 1, dgl.maxAskPrice)
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
                "Closing {0} with {1}."
                    .format(
                        self.security.symbol,
                        self.security.descalePrice(closePrice)))
            position.close(closePrice)
        elif position.isClosed:
            # Risk Management:
            # If one order gets filled without the other, sell
            # immediately at market order.
            self.log.debug(
                "Closing all at market price."
                    .format(
                        self.security.symbol,
                        self.security.descalePrice(closePrice)))
            map(
                lambda position: position.cancelAtMarketPrice(),
                self.positions)

