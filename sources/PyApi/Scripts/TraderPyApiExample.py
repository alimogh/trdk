
# Trader engine module
import Trader

# Strategy algorithm class must be inherited from trader.Algo
class SimpleExampleTradeAlgo(Trader.Algo):

	# Pure virtual method, must be implemented in strategy implementation.
	# This will be called by engine every time when:
	# 	- arrives new data for symbol
	#	  or timeout ${algo_update_period_ms} has expired
	def tryToOpenPositions(self):

		# Checking entry condition:
		spread = self.security.bidPrice - self.security.askPrice
		if spread > -0.01:
			# not the case for trade
			return

		# Creating position object (long position on this case):
		position = Trader.LongPosition(
			# security object
			self.security,			
			# number of shares
			int(10000 / self.security.askPrice),
			# start price
			self.security.bidPrice)
	
		# Sending IOC order (if order will be filled:
		# tryToClosePositions-method will be called, if not:
		# tryToOpenPositions will be called again):
		position.openOrCancel(position.openStartPrice)

		return position


	# Pure virtual method, must be implemented in strategy implementation.
	# This will be called by engine every time when:
	# 	- arrives new data for symbol
	#	  or timeout ${algo_update_period_ms} has expired
	#	- strategy has opened position
	#	- all previous orders has been canceled or filled
	def tryToClosePositions(self, position):
		
		takeProfitPrice = position.openPrice + 0.03
		stopLossPrice = position.openPrice - 0.03

		if self.security.askPrice >= takeProfitPrice:
			if position.hasActiveOrders:
				# order already sent and still active
				return
			# take profit: trying to close with IOC
			position.closeOrCancel(takeProfitPrice)
		elif self.security.askPrice <= stopLossPrice:
			# stop loss: cancel all active orders and close position at market price 
			position.cancelAtMarketPrice()
