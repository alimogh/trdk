
import trdk

class GoldArbitrage(trdk.Strategy):

    _bars = None

    def onServiceStart(self, service):
        self._bars = service

    def onServiceDataUpdate(self, service):
        pass

    def tryToClosePosition(self, position):
        pass
