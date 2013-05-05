
import trdk
import time

class GoldArbitrage(trdk.Strategy):

    _bars = None

    def onServiceStart(self, service):
        self._bars = service

    def onServiceDataUpdate(self, service):
        lastBar = service.getBarByReversedIndex(0)
        self.log.debug(
            "{0} {1}: {2} - {3}"
                .format(
                    service.size,
                    time.ctime(lastBar.time),
                    self.security.descalePrice(lastBar.minBidPrice),
                    self.security.descalePrice(lastBar.maxAskPrice)))
        pass

    def tryToClosePosition(self, position):
        pass
