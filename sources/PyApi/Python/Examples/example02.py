"""

 Trading Robot Development Kit strategy script example

    Author: Eugene V. Palchukovsky
    E-mail: eugene@palchukovsky.com
   Project: Trading Robot Development Kit
       URL: http://robotdk.com
 Copyright: Eugene V. Palchukovsky

-------------------------------------------------------------------------------

INI-file:

; Initializing 5 minutes bar service:
[Service.5 minute bar]
    symbols = symbols.ini
    uses = Trades
    module = Services
    factory = BarService
    size = 5 minutes ; seconds / minutes / hours / days
    log = none

; Initializing example strategy:
[Strategy.Test]

    symbols = symbols.ini   ; symbols collection (instance of Example02 will be
                            ; created for each symbol from symbols.ini)

    uses = 5 minute bar ; strategy uses bar service

    module = PyApi  ; PyApi module (for each service or strategy that
                    ; implemented in Python)

    class = Example02 ; strategy Python class name

    script_file_path = D:\TradingRobotDK\Examples\example02.py ; path to script

"""

import trdk
import time


class Example02(trdk.Strategy):

    _bars = None

    def onServiceStart(self, service):
        assert self._bars is None, "Subscribed only to one service."
        assert \
            service.tag == "5 minute bar", \
            "INI-file has configuration only for this service."
        # Caching service reference:
        self._bars = service

    def onServiceDataUpdate(self, service):

        assert service == self._bars, "Events only for registered services."

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
        position = trdk.LongPosition(
            # strategy:
            self,
            # number of shares:
            10000,
            # start price (used in reports):
            openPrice.max)

        self.log.info('Opening position...')
        # Opening new position for current strategy instance: Sending MKT-order
        # (method onPositionUpdate will be called at each position state
        # update):
        position.openAtMarketPrice()
        assert(self.positions.count() == 1) # Position object now in list.

    # Virtual method. Notifies about position state update. Optional.
    def onPositionUpdate(self, position):
        # Here can be logic for new position state.
        self.log.debug("Position state changed.")

    # Example method: how position can be closed. See onLevel1Update for call.
    def tryToClosePosition(self, position):

        if position.hasActiveOrders:
            return

        if isinstance(position, trdk.LongPosition):
            # Here can be special logic for long position.
            pass
        else:
            assert(isinstance(position, trdk.ShortPosition))
            # Here can be special logic for short position.
            pass

        self._testCount += 1
        if self._testCount < 10:
            self.log.debug('Skip closing...')
        else:
            self.log.info('Close position...')
            position.cancelAtMarketPrice()
            self._stop = True
