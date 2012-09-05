
# Trader engine module
import Trader

# Strategy algorithm class must be inherited from trader.Algo
class LightspeedGatewayTest(Trader.Algo):

	# Pure virtual method, must be implemented in strategy implementation.
	# This will be called by engine every time when:
	# 	- arrives new data for symbol
	#	  or timeout ${algo_update_period_ms} has expired
	def tryToOpenPositions(self):

		Trader.logInfo("try-to-open-positions: " + self.security.symbol)

		# Creating position object (long position on this case):
		position = Trader.LongPosition(
			# security object
			self.security,
			# number of shares
			12,
			# start price
			0)
	
		# Sending IOC order (if order will be filled:
		# tryToClosePositions-method will be called, if not:
		# tryToOpenPositions will be called again):
		position.openAtMarketPrice()

		self.m_closeAttempts = 0

		return position


	# Pure virtual method, must be implemented in strategy implementation.
	# This will be called by engine every time when:
	# 	- arrives new data for symbol
	#	  or timeout ${algo_update_period_ms} has expired
	#	- strategy has opened position
	#	- all previous orders has been canceled or filled
	def tryToClosePositions(self, position):

		Trader.logInfo("try-to-close-positions: " + self.security.symbol)
		
		self.m_closeAttempts = self.m_closeAttempts + 1
		
		if position.hasActiveOrders:
			# order already sent and still active
			if self.m_closeAttempts > 10:
				Trader.logInfo("try-to-close-positions: " + self.security.symbol + " -> can't wait more, canceling")
				# cancel all active orders and close position at market price
				position.cancelAtMarketPrice()
		else:
			Trader.logInfo("try-to-close-positions: " + self.security.symbol + " -> close")
			position.closeAtMarketPrice()
