
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary("Trdk_dbg.dll")
api.GetImpliedVolatility.restype = ctypes.c_double

api.trdk_InitLogToStdOut()

print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140606",  ctypes.c_double(1.365), "Put", "6E")
print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140606",  ctypes.c_double(1.37),  "Put", "6E")
print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140606", ctypes.c_double(1.37), "Call", "6E")
print api.GetImpliedVolatility("EUR.USD", "GLOBEX", "20140606", ctypes.c_double(1.375), "Call", "6E")

print "Completed. Press Enter to exit."
msvcrt.getch()
