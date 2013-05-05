
import trdk
import time

strategies = dict()
lastIndex = -1

class GoldArbitrage(trdk.Strategy):

    bars = None
    lastSize = 0

    def __init__(self, param):
        super(self.__class__, self).__init__(param)
        strategies[self.security.symbol] = self

    def onServiceStart(self, service):
        self.bars = service

    def onServiceDataUpdate(self, service):
        if self.security.symbol == "GLD":
            if strategies["DGL"].bars.size >= service.size:
                self.checkEntry(service.size - 1)
        elif self.security.symbol == "DGL":
            if service.size < lastIndex:
                return
            gld = strategies["GLD"]
            if gld.bars.size < service.size:
                return
            gld.checkEntry(service.size - 1)

    def checkEntry(self, index):

        if self.positions.count() > 0 or strategies["DGL"].positions.count() > 0:
            return
            
        gld = self.bars.getBarByIndex(index)
        dgl = strategies["DGL"].bars.getBarByIndex(index)

        self.lastSize = index
            
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
            dglPos = trdk.LongPosition(strategies["DGL"], dgl.minBidPrice, 1)
            gldPos.open(gldPos.openStartPrice)
            dglPos.open(dglPos.openStartPrice)
        
        # If opening of 5 minute candlestick GLD(Bid)/DGL(Ask)<=2.842 .. 
        if ratioLe > 0 and ratioLe <= 2.842:
            # Long GLD @ Bid, Short DGL @ Ask
            self.log.info('Opening positions by "Entry 2"...')
            gldPos = trdk.LongPosition(self, 1, gld.minBidPrice)
            dglPos = trdk.ShortPosition(strategies["DGL"], dgl.maxAskPrice, 1)
            gldPos.open(gldPos.openStartPrice)
            dglPos.open(dglPos.openStartPrice)

    def tryToClosePosition(self, position):
        if position.isOpened:
            # Profit Targets:
            # Sell positions @ GLD/DGL ratio of 2.847
            closePrice = position.openStartPrice * 2.847
            self.log.debug(
                "Closing {0} with {1}"
                    .format(
                        self.security.symbol,
                        self.security.descalePrice(closePrice)))
            position.close(closePrice)
        pass
