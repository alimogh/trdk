
# Trader engine module
import Trader

################################################################################

# Strategy algorithm class must be inherited from Trader.Strategy
class Example_1(Trader.Strategy):

	# Constructor doesn't required, here just for example.
	def __init__(self, param):
		# Pass parameter to base class:
		super(self.__class__, self).__init__(param)
		# Place here initial procedure for this instance:
		self._foo = 10

	# Pure virtual method, must be implemented in strategy implementation.
	# Returns algorith name.
	def getName(self):
		return self.__class__.__name__

	# Virtual method. Not pure. Not required, if algorithm doen't use services.
	def notifyServiceStart(self, service):
		if service.getName() == 'Nonexistent service':
			# Using service object if algorithm requires it...
			pass
		else:
			# Send service object to base if algorithm doesn't required it...
			super(self.__class__, self).notifyServiceStart(service)

	# Pure virtual method, must be implemented in strategy implementation.
	# This will be called by engine every time when:
	#   - arrives new data for symbol
	#     or timeout ${strategy_update_period_ms} has expired
	#     or one of binded service has changed its state
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
			self.security.bidPrice,
			# strategy tag for statistic
			self.tag)
	
		# Sending IOC order (if order will be filled:
		# tryToClosePositions-method will be called, if not:
		# tryToOpenPositions will be called again):
		position.openOrCancel(position.openStartPrice)

		return position

	# Pure virtual method, must be implemented in strategy implementation.
	# This will be called by engine every time when:
	#   - arrives new data for symbol
	#	  or timeout ${strategy_update_period_ms} has expired
	#     or one of binded service has changed its state
	#   - strategy has opened position
	#   - all previous orders has been canceled or filled
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
			# stop loss: cancel all active orders and close
			# position at market price 
			position.cancelAtMarketPrice()

################################################################################

class Example_2(Trader.Strategy):

	def __init__(self, param):
		super(self.__class__, self).__init__(param)
		self._stop = False

	def getName(self):
		return self.__class__.__name__

	def notifyServiceStart(self, service):
		if service.tag == '5 minute bar':
			self._fiveMinsBars = service
		else:
			super(self.__class__, self).notifyServiceStart(service)
	
	def tryToOpenPositions(self):
		if self._stop == True:
			return
		self._testCount = 0
		Trader.logInfo('Open position...' + self._fiveMinsBars.size)
		return

	def tryToClosePositions(self, position):
		if position.hasActiveOrders:
			return
		self._testCount = self._testCount + 1
		if self._testCount < 10:
			Trader.logInfo('Skip...')
		else:
			Trader.logInfo('Close position...')
			position.cancelAtMarketPrice()
			self._stop = True

################################################################################			
