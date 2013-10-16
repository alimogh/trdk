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
    "BarService"]

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

    def getBarByIndex(self, index):
        """ Returns bar by index. First bar has index "zero".
        :type index: int
        :rtype: trdk.BarService.Bar
        """
        pass

    def getBarByReversedIndex(self, index):
        """ Returns bar by reversed index. Last bar has index "zero".
        :type index: int
        :rtype: trdk.BarService.Bar
        """
        pass

    def getOpenPriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.BarService.PriceStat
        """
        pass

    def getClosePriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.BarService.PriceStat
        """
        pass

    def getHighPriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: :rtype: trdk.BarService.PriceStat
        """
        pass

    def getLowPriceStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.BarService.PriceStat
        """
        pass

    def getTradingVolumeStat(self, numberOfBars):
        """
        :type numberOfBars: int
        :rtype: trdk.BarService.QtyStat
        """
        pass

###############################################################################
