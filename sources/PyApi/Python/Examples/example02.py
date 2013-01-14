
import trader
import time

class Example02(trader.Strategy):

    _bars = None

    def onServiceStart(self, service):
        assert(self._bars is None)
        # INI-file has configuration only for this service:
        if service.tag == "5 minute bar":
            # Caching service reference:
            self._bars = service

    def onServiceDataUpdate(self, service):

        # Events only for registered services
        assert(service == self._bars)

        if self.positions.count() > 0:
            # Some positions are opened, checking each for closing...
            map(self.tryToClosePosition, self.positions)
            return

        lastBar = self._bars.getBarByReversedIndex(0)
        openPrice = self._bars.getOpenPriceStat(10)
        self.log.debug(
            "{0}: new data ({1} / {2})..."
                .format(
                    time.ctime(lastBar.time),
                    self.security.descalePrice(lastBar.openPrice),
                    self.security.descalePrice(openPrice.max)))

        # Check condition (price "scaled"):
        if openPrice.max <= 137449:
            # Skip opening as max open price less then required:
            return

        self._testCount = 0
        position = trader.LongPosition(
            # strategy
            self,
            # number of shares
            10000,
            # start price
            openPrice.max)

        self.log.info('Opening position...')
        # Opening new position for current strategy instance: Sending MKT-order
        # (method onPositionUpdate will be called at each position state
        # update):
        position.openAtMarketPrice()
        assert(self.positions.count() == 1) # Position object now in list

    # Virtual method. Notifies about position state update. Optional.
    def onPositionUpdate(self, position):
        # Here can be logic for new position state.
        self.log.debug("Position state changed.")

    # Example method: how position can be closed. See onLevel1Update for call.
    def tryToClosePosition(self, position):

        if position.hasActiveOrders:
            return

        if isinstance(position, trader.LongPosition):
            # Here can be special logic for long position.
            pass
        else:
            assert(isinstance(position, trader.ShortPosition))
            # Here can be special logic for short position.
            pass

        self._testCount = self._testCount + 1
        if self._testCount < 10:
            self.log.debug('Skip closing...')
        else:
            self.log.info('Close position...')
            position.cancelAtMarketPrice()
            self._stop = True
