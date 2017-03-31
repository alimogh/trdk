
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary('RobotEngine_dbg.dll')

api.trdk_EnableStdOutLog.restype = ctypes.c_int

api.trdk_InitLog.restype = ctypes.c_int
api.trdk_InitLog.argtypes = [ctypes.c_char_p]

api.GetCashBalance.restype = ctypes.c_double
api.trdk_GetCashBalance.restype = ctypes.c_double
api.GetCashBalance.argtypes = [ctypes.c_char_p]
api.trdk_GetCashBalance.argtypes = [ctypes.c_char_p]

api.GetEquityWithLoanValue.restype = ctypes.c_double
api.trdk_GetEquityWithLoanValue.restype = ctypes.c_double
api.GetEquityWithLoanValue.argtypes = [ctypes.c_char_p]
api.trdk_GetEquityWithLoanValue.argtypes = [ctypes.c_char_p]

api.GetMaintenanceMargin.restype = ctypes.c_double
api.trdk_GetMaintenanceMargin.restype = ctypes.c_double
api.GetMaintenanceMargin.argtypes = [ctypes.c_char_p]
api.trdk_GetMaintenanceMargin.argtypes = [ctypes.c_char_p]

api.GetExcessLiquidity.restype = ctypes.c_double
api.trdk_GetExcessLiquidity.restype = ctypes.c_double
api.GetExcessLiquidity.argtypes = [ctypes.c_char_p]
api.trdk_GetExcessLiquidity.argtypes = [ctypes.c_char_p]

api.GetOptionPremium.restype =\
	api.trdk_GetOptionPremium.restype =\
	ctypes.c_double
api.GetOptionPremium.argtypes =\
	api.trdk_GetOptionPremium.argtypes =\
	[ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_double, ctypes.c_double, ctypes.c_char_p, ctypes.c_double]

api.trdk_EnableStdOutLog()
api.trdk_InitLog('RobotEngine.log');


def print_test():

	print ''

	print 'Account:'
	print '  Cash _______________ {0}'.format(api.GetCashBalance(''))
	print '  Equity with loan ___ {0}'.format(api.GetEquityWithLoanValue(''))
	print '  Maintenance margin _ {0}'.format(api.GetMaintenanceMargin(''))
	print '  Excess liquidity ___ {0}'.format(api.GetExcessLiquidity(''))
	print ''

	symbol = 'AAPL'
	request_date = 20170301.00
	option_premium = api.GetOptionPremium(symbol, 'USD', 'SMART', 20170421.00, 140.0, 'Put', request_date)
	print 'Option premium for {0} at {1}: {2}'.format(symbol, request_date, option_premium)
	print ''


while (True):
	print_test()
	print '_________________________________________'
	print 'Press "e" to exit or press any other key to refresh.'
	if msvcrt.getch() == 'e':
		break
