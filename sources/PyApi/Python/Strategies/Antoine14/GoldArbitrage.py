import trdk
import time

ratio = 2.860
positionVolume = 20000
orderDisplaySize = 100

openOrderParams = trdk.OrderParams()
openOrderParams.goodInSeconds = 60
openOrderParams.isMarketPrice = True
openOrderParams.displaySize = orderDisplaySize

closeOrderParams = trdk.OrderParams()
openOrderParams.displaySize = orderDisplaySize

assert \
    orderDisplaySize % 100 == 0, \
    "The Display Size should be a multiple of the round lot size for this security."


# noinspection PyCallByClass,PyTypeChecker
class GoldArbitrage(trdk.Strategy):

    ###########################################################################

    class Long(trdk.LongPosition):

        entryType = None

        def __init__(self, strategy, security, qty, entryType):
            super(self.__class__, self).__init__(
                strategy,
                security,
                qty,
                security.askPrice)
            self.entryType = entryType

    class Short(trdk.ShortPosition):

        entryType = None

        def __init__(self, strategy, security, qty, entryType):
            super(self.__class__, self).__init__(
                strategy,
                security,
                qty,
                security.bidPrice)
            self.entryType = entryType

    ###########################################################################

    gldBars = None
    dglBars = None

    ###########################################################################

    def getRequiredSuppliers(self):
        return \
            '5 minute bars[GLD], 5 minute bars[DGL]' \
            ', Broker Positions[GLD], Broker Positions[DGL]'

    ###########################################################################

    def onBrokerPositionUpdate(self, security, qty, isInitial):

        if isInitial is False:
            return

        if security.symbol == 'GLD':
            if qty < 0:
                entryType = 'shortGldLongDlg'
            else:
                entryType = 'longGldShortDgl'
        elif security.symbol == 'DGL':
            if qty > 0:
                entryType = 'shortGldLongDlg'
            else:
                entryType = 'longGldShortDgl'
        else:
            return

        if qty < 0:
            position = GoldArbitrage.Short(self, security, qty, entryType)
        elif qty > 0:
            position = GoldArbitrage.Long(self, security, qty, entryType)
        else:
            return

        self.log.debug(
            'Restoring {0} {1} position: {2}...'
            .format(
                position.type,
                position.security.symbol,
                position.activeQty))
        position.restoreOpenState()

    ###########################################################################

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
        if self.positions.count() == 0:
            self._checkEntry()
        else:
            map(
                lambda position: self._checkExit(position),
                self.positions)

    def onPositionUpdate(self, position):

        if position.isCompleted is True:
            isCompletedStr = "completed"
        else:
            isCompletedStr = "not completed"
        if position.isCanceled is True:
            isCanceledStr = "canceled"
        else:
            isCanceledStr = "not canceled"

        self.log.debug(
            "{0} {1} position changed:"
            " {2} -> {3}({4}) -> {5}({6}) = {7}"
            " ({8}, {9})"
            .format(
                position.security.symbol,
                position.type,
                position.planedQty,
                position.openedQty,
                position.security.descalePrice(position.openPrice),
                position.closedQty,
                position.security.descalePrice(position.closePrice),
                position.activeQty,
                isCompletedStr,
                isCanceledStr))

        if position.isCompleted is False or position.isCanceled is True:
            return
        self.log.info('{0} closed first.'.format(position.security.symbol))
        # Risk Management:
        # If one order gets filled without the other, sell
        # immediately at market order.
        map(
            lambda position: position.cancelAtMarketPrice(),
            self.positions)

    ###########################################################################

    def findGld(self):
        return self.findSecurity('GLD')

    def findDgl(self):
        return self.findSecurity('DGL')

    ###########################################################################

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

        gld = self.findGld()
        gldBar = self.gldBars.getBarByReversedIndex(0)

        dgl = self.findDgl()
        dglBar = self.dglBars.getBarByReversedIndex(0)

        currentShortGldLongDlgRatio = 0
        checkShortGldLongDlgRatio = 0
        shortGldLongDlg = False
        # When GLD (ask)/DGL (bid) > ratio * 1.001% -> Short GLD, Long DGL
        if dglBar.minBidPrice != 0:
            currentShortGldLongDlgRatio \
                = float(gldBar.maxAskPrice) / float(dglBar.minBidPrice)
            checkShortGldLongDlgRatio = ratio * (1.001 / 100)
            shortGldLongDlg \
                = currentShortGldLongDlgRatio > checkShortGldLongDlgRatio
        self.log.debug(
            'Entry 1: "GLD(Ask {0}) / DGL(Bid {1}) = {2}"'
            ' < "{3} * 1.001% = {4}" -> {5}'
            .format(
                gld.descalePrice(gldBar.maxAskPrice),
                dgl.descalePrice(dglBar.minBidPrice),
                currentShortGldLongDlgRatio,
                ratio,
                checkShortGldLongDlgRatio,
                shortGldLongDlg))

        currentLongGldShortDglRatio = 0
        checkLongGldShortDglRatio = 0
        longGldShortDgl = False
        if dglBar.maxAskPrice != 0:
            # When GLD (bid)/DGL (ask) < ratio * 0.999% -> Long GLD, Short DGL
            currentLongGldShortDglRatio \
                = float(gldBar.minBidPrice) / float(dglBar.maxAskPrice)
            checkLongGldShortDglRatio = ratio * (0.999 / 100)
            longGldShortDgl \
                = currentLongGldShortDglRatio < checkLongGldShortDglRatio
        self.log.debug(
            'Entry 2: "GLD(Bid {0}) / DGL(Ask {1}) = {2}"'
            ' > "{3} * 0.999% = {4}" -> {5}'
            .format(
                gld.descalePrice(gldBar.minBidPrice),
                dgl.descalePrice(dglBar.maxAskPrice),
                currentLongGldShortDglRatio,
                ratio,
                checkLongGldShortDglRatio,
                longGldShortDgl))

        def calcQty(security, price):
            return security.scalePrice(positionVolume) / price

        if shortGldLongDlg:
            gldQty = calcQty(gld, gldBar.maxAskPrice)
            dglQty = calcQty(dgl, dglBar.minBidPrice)
            gldPos = GoldArbitrage.Short(self, gld, gldQty, 'shortGldLongDlg')
            dglPos = GoldArbitrage.Long(self, dgl, dglQty, 'shortGldLongDlg')
            self.log.info(
                'Opening positions "Short GLD {0}/{4}, Long DGL {1}/{5}"'
                ' (volume: {2}, visible qty: {3}, good in: {6})...'
                .format(
                    gldQty,
                    dglQty,
                    positionVolume,
                    openOrderParams.displaySize,
                    gld.descalePrice(gldPos.openStartPrice),
                    dgl.descalePrice(dglPos.openStartPrice),
                    openOrderParams.goodInSeconds))
            self._openPositions(gldPos, dglPos)

        if longGldShortDgl:
            gldQty = calcQty(gld, gldBar.minBidPrice)
            dglQty = calcQty(dgl, dglBar.maxAskPrice)
            gldPos = GoldArbitrage.Long(self, gld, gldQty, 'longGldShortDgl')
            dglPos = GoldArbitrage.Short(self, dgl, dglQty, 'longGldShortDgl')
            self.log.info(
                'Opening positions "Short GLD {0}/{4}, Long DGL {1}/{5}"'
                ' (volume: {2}, visible qty: {3}, good in: {6})...'
                .format(
                    gldQty,
                    dglQty,
                    positionVolume,
                    openOrderParams.displaySize,
                    gld.descalePrice(gldPos.openStartPrice),
                    dgl.descalePrice(dglPos.openStartPrice),
                    openOrderParams.goodInSeconds))
            self._openPositions(gldPos, dglPos)

    def _openPositions(self, gld, dgl):
        if openOrderParams.isMarketPrice is True:
            gld.openAtMarketPrice(openOrderParams)
            dgl.openAtMarketPrice(openOrderParams)
        else:
            gld.open(gld.openStartPrice, openOrderParams)
            dgl.open(dgl.openStartPrice, openOrderParams)

    def _checkExit(self, position):

        if position.isOpened is False or position.hasActiveCloseOrders is True:
            self.log.debug(
                '{0} {1} position has active orders...'
                .format(position.security.symbol, position.type))
            return

        if position.entryType == 'shortGldLongDlg':
            # Exit trade when GLD (bid)/DGL (ask) = ratio
            gldPrice = self.gldBars.getBarByReversedIndex(0).minBidPrice
            dglPrice = self.dglBars.getBarByReversedIndex(0).maxAskPrice
            message = 'Exit 1: "GLD(Bid {0}) / DGL(Ask {1}) = {2}"'
            message += ' == {3} -> {4};'
        else:
            # Exit trade when GLD (ask)/DGL (bid) = ratio
            assert position.entryType == 'longGldShortDgl'
            gldPrice = self.gldBars.getBarByReversedIndex(0).maxAskPrice
            dglPrice = self.dglBars.getBarByReversedIndex(0).minBidPrice
            message = 'Exit 2: "GLD(Ask {0}) / DGL(Bid {1}) = {2}"'
            message += ' == {3} -> {4};'

        currentRatio = float(gldPrice) / float(dglPrice)
        currentRatio = round(currentRatio, 2)
        isExitTime = currentRatio == ratio

        self.log.info(
            message.format(
                self.findGld().descalePrice(gldPrice),
                self.findDgl().descalePrice(dglPrice),
                currentRatio,
                ratio,
                isExitTime))
        if isExitTime is not True:
            return

        position.closeAtMarketPrice(closeOrderParams)

    ###########################################################################
