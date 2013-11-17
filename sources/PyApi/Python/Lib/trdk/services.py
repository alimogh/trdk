"""

 Trading Robot Development Kit - Services module

    Author: Eugene V. Palchukovsky
    E-mail: eugene@palchukovsky.com
   Project: Trading Robot Development Kit
       URL: http://robotdk.com
 Copyright: Eugene V. Palchukovsky

"""

###############################################################################

import trdk

###############################################################################

__all__ = [
    "BarService",
    "MovingAverageService"]

###############################################################################


class BarService(trdk.ServiceInfo):

    class Bar(object):

        time = float  # Bar start time.
        size = int  # Bar size, seconds.

        maxAskPrice = int
        openAskPrice = int
        closeAskPrice = int

        minBidPrice = int
        openBidPrice = int
        closeBidPrice = int

        openTradePrice = int
        closeTradePrice = int

        highTradePrice = int
        lowTradePrice = int

        tradingVolume = int

    class Stat(object):
        pass

    class PriceStat(Stat):
        max = int
        min = int

    class QtyStat(Stat):
        max = int
        min = int

    barSize = int  # Each bar size, seconds.
    size = int  # Number of bars.
    isEmpty = bool

    lastBar = trdk.services.BarService.Bar  # Last bar.

    def getBar(self, index):
        """ Returns bar by index. First bar has index "zero".
        :type index: int
        :rtype: trdk.services.BarService.Bar
        """
        pass

    def getBarByReversedIndex(self, index):
        """ Returns bar by reversed index. Last bar has index "zero".
        :type index: int
        :rtype: trdk.services.BarService.Bar
        """
        pass

    def getOpenPriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.services.BarService.PriceStat
        """
        pass

    def getClosePriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.services.BarService.PriceStat
        """
        pass

    def getHighPriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: :rtype: trdk.services.BarService.PriceStat
        """
        pass

    def getLowPriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.services.BarService.PriceStat
        """
        pass

    def getTradingVolumeStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.services.BarService.QtyStat
        """
        pass

###############################################################################


class MovingAverageService(trdk.ServiceInfo):

    class Point(object):
        value = float

    isEmpty = bool

    lastPoint = trdk.services.MovingAverageService.Point  # Last value point.

    historySize = int  # Number of points in history.

    def getHistoryPoint(self, index):
        """ Returns value point from history by index. First value has index
            "zero".
        :type index: int
        :rtype: trdk.services.MovingAverageService.Point
        """
        pass

    def getHistoryPointByReversedIndex(self, index):
        """ Returns value point from history by reversed index. Last bar has index
            "zero".
        :type index: int
        :rtype: trdk.services.MovingAverageService.Point
        """
        pass

###############################################################################
