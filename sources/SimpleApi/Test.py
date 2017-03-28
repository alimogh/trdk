
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary("TrdkTwsBridge_dbg.dll")

api.GetNetLiquidationValueTotal.restype						= ctypes.c_int
api.GetNetLiquidationValueUsSecurities.restype				= ctypes.c_int
api.GetNetLiquidationValueUsCommodities.restype				= ctypes.c_int
api.GetEquityWithLoanValueTotal.restype						= ctypes.c_int
api.GetEquityWithLoanValueUsSecurities.restype				= ctypes.c_int
api.GetEquityWithLoanValueUsCommodities.restype				= ctypes.c_int
api.GetSecuritiesGrossPositionValueTotal.restype			= ctypes.c_int
api.GetSecuritiesGrossPositionUsSecurities.restype			= ctypes.c_int
api.GetCashTotal.restype									= ctypes.c_int
api.GetCashUsSecurities.restype								= ctypes.c_int
api.GetCashUsCommodities.restype							= ctypes.c_int
api.GetCurrentInitialMarginTotal.restype					= ctypes.c_int
api.GetCurrentInitialMarginUsSecurities.restype				= ctypes.c_int
api.GetCurrentInitialMarginUsCommodities.restype			= ctypes.c_int
api.GetCurrentMaintenanceMarginTotal.restype				= ctypes.c_int
api.GetCurrentMaintenanceMarginUsSecurities.restype			= ctypes.c_int
api.GetCurrentMaintenanceMarginUsCommodities.restype		= ctypes.c_int
api.GetCurrentAvailableFundsTotal.restype					= ctypes.c_int
api.GetCurrentAvailableFundsUsSecurities.restype			= ctypes.c_int
api.GetCurrentAvailableFundsUsCommodities.restype			= ctypes.c_int
api.GetCurrentExcessLiquidityTotal.restype					= ctypes.c_int
api.GetCurrentExcessLiquidityUsSecurities.restype			= ctypes.c_int
api.GetCurrentExcessLiquidityUsCommodities.restype			= ctypes.c_int

api.trdk_InitLogToStdOut()

def print_current():
	print "Net Liquidation Value_______ {0}\t{1}\t{2}".format(	api.GetNetLiquidationValueTotal(),			api.GetNetLiquidationValueUsSecurities(),		api.GetNetLiquidationValueUsCommodities());
	print "Equity With Loan Value______ {0}\t{1}\t{2}".format(	api.GetEquityWithLoanValueTotal(),			api.GetEquityWithLoanValueUsSecurities(),		api.GetEquityWithLoanValueUsCommodities());
	print "Securities Gross Position___ {0}\t{1}".format(		api.GetSecuritiesGrossPositionValueTotal(),	api.GetSecuritiesGrossPositionUsSecurities())
	print "Cash________________________ {0}\t{1}\t{2}".format(	api.GetCashTotal(),							api.GetCashUsSecurities(),						api.GetCashUsCommodities())
	print "Current Initial Margin______ {0}\t{1}\t{2}".format(	api.GetCurrentInitialMarginTotal(),			api.GetCurrentInitialMarginUsSecurities(),		api.GetCurrentInitialMarginUsCommodities())
	print "Current Maintenance Margin__ {0}\t{1}\t{2}".format(	api.GetCurrentMaintenanceMarginTotal(),		api.GetCurrentMaintenanceMarginUsSecurities(),	api.GetCurrentMaintenanceMarginUsCommodities())
	print "Current Available Funds_____ {0}\t{1}\t{2}".format(	api.GetCurrentAvailableFundsTotal(),		api.GetCurrentAvailableFundsUsSecurities(),		api.GetCurrentAvailableFundsUsCommodities())
	print "Current Excess Liquidity____ {0}\t{1}\t{2}".format(	api.GetCurrentExcessLiquidityTotal(),		api.GetCurrentExcessLiquidityUsSecurities(),	api.GetCurrentExcessLiquidityUsCommodities())

while (True):
	print_current()
	print "_________________________________________"
	print "Completed. Press Enter to continue."
	msvcrt.getch()
