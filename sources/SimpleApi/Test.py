
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary("Trdk_dbg.dll")
api.GetImpliedVolatility.restype = ctypes.c_double

api.trdk_InitLogToStdOut()

while (True):
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905",  ctypes.c_double(1.365), "Put", "6E")
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905",  ctypes.c_double(1.37),  "Put", "6E")
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905", ctypes.c_double(1.37), "Call", "6E")
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905", ctypes.c_double(1.375), "Call", "6E")

	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905",  ctypes.c_double(1.38), "Call", "6E")
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905",  ctypes.c_double(1.38),  "Put", "6E")
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905", ctypes.c_double(1.385), "Call", "6E")
	print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140905", ctypes.c_double(1.385), "Put", "6E")

	print "Completed. Press Enter to continue."
	msvcrt.getch()

