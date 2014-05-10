
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary("Trdk_dbg.dll")
api.GetImpliedVolatility.restype = ctypes.c_double

api.trdk_InitLogToStdOut()
api.trdk_InitLog("Logs/Trdk.log")

print api.GetImpliedVolatility("EUR.USD", "Globex", "20140606", ctypes.c_double(1.365), "Call", "6E")
# print api.GetImpliedVolatility("EUR.USD", "Globex", "20140530", ctypes.c_double(1.375), "Call", "6E5")

print "Completed. Press Enter to exit."
msvcrt.getch()
