"""

INI-file:

; Initializing test service:
[Service.Example03]

    symbols = symbols.ini   ; symbols collection (instance of TestService will
                            ; be created for each symbol from symbols.ini)

    uses = Trades   ; service uses trades and will be subscribed to new
                    ; trades: on each new trade method
                    ; TestService.onNewTrade will be called

    module = PyApi  ; PyApi module (for each service or strategy that
                    ; implemented in Python)

    class = ExampleService ; service Python class name

    script_file_path = Examples/example03.py ; path to script

; Initializing example strategy:
[Strategy.Example03]

    symbols = symbols.ini   ; symbols collection (instance of Example02 will be
                            ; created for each symbol from symbols.ini)

    uses = Example03    ; strategy uses service Example03

    module = PyApi  ; PyApi module (for each service or strategy that
                    ; implemented in Python)

    class = ExampleStrategy ; strategy Python class name

    script_file_path = Examples/example03.py ; path to script

"""

import trader

###############################################################################

class ExampleService(trader.Service):

    _lastDirectionChangePrice = 0

    lastTickPrice = 0
    priceDirection = None

    def onNewTrade(self, security, time, price, qty, side):

        isServiceStateChanged = False
        if int(self.lastTickPrice) == 0:
            # First tick.
            pass

        elif self._lastDirectionChangePrice > price:

            deviation = self._lastDirectionChangePrice - price

            if security.descalePrice(deviation) < 0.50:
                # Do nothing - too small a price deviation, less the 0.50.
                pass

            elif self.priceDirection == 'falls' or self.priceDirection is None:
                self.log.debug(
                    'Price now "growing" ({0} -> {1}: +{2}).'
                        .format(
                            security.descalePrice(self._lastDirectionChangePrice),
                            security.descalePrice(price),
                            security.descalePrice(deviation)))
                self.priceDirection = 'growing'
                self._lastDirectionChangePrice = price
                # Direction changed, need notify service subscribers:
                isServiceStateChanged = True

        elif self.lastTickPrice < price:

            deviation = price - self._lastDirectionChangePrice

            if security.descalePrice(deviation) < 0.50:
                # Do nothing - too small a price deviation, less the 0.50.
                pass

            elif self.priceDirection == 'growing' or self.priceDirection is None:
                self.log.debug(
                    'Price now "falls" ({0} -> {1}: -{2}).'
                        .format(
                            security.descalePrice(self._lastDirectionChangePrice),
                            security.descalePrice(price),
                            security.descalePrice(deviation)))
                self._lastDirectionChangePrice = price
                self.priceDirection = 'falls'
                # Direction changed, need notify service subscribers:
                isServiceStateChanged = True

        self.lastTickPrice = price
        return isServiceStateChanged

###############################################################################

class ExampleStrategy(trader.Strategy):

    def onServiceDataUpdate(self, service):

        # This method will be called only at price direction changes (see
        # TestService and TestService.onNewTrade result for details).

        assert \
            isinstance(service, ExampleService), \
            "Strategy subscribed only to ExampleService."

        if self.positions.count() > 0:
            assert(self.positions.count() == 1)
            # Has opened position, checking whether we should close it
            if service.priceDirection == 'growing':
                # Price still growing, do nothing:
                return
            # Checking conditions for each active position:
            for position in self.positions:
                assert \
                    isinstance(position, trader.LongPosition), \
                    'This strategy works only with "long" positions.'
                if position.openStartPrice >= service.lastTickPrice:
                    self.log.debug('Opening long position as price "growing"...')
                    position.cancelAtMarketPrice()
                elif position.isOpened:
                    # Price falling, but not yet too low, so just canceling
                    # orders that not yet fulfilled:
                    position.cancelAllOrders()

        elif service.priceDirection == 'growing':
            self.log.debug('Opening long position as price "growing"...')
            position = trader.LongPosition(
                # strategy:
                self,
                # number of shares:
                10000,
                # start price (used in reports):
                service.lastTickPrice)
            position.openOrCancel(service.lastTickPrice)
            assert(self.positions.count() == 1) # Position object now in list.

###############################################################################
