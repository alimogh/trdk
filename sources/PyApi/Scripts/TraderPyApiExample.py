
import Trader

class TestAlgo(Trader.Algo):

	def tryToOpenPositions(self):
		position = Trader.LongPosition(self.security, 1, 1)
		position.openAtMarketPrice()
		return position

	def tryToClosePositions(self, position):
		position.cancelAtMarketPrice()
