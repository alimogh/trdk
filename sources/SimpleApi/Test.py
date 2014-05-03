
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary("Trdk_dbg.dll")
api.trdk_ResolveFutOpt.restype = ctypes.c_uint
# api.trdk_GetLastPrice.restype = ctypes.c_double

api.trdk_InitLogToStdOut()
api.trdk_InitLog("Logs/Trdk.log")

put = api.trdk_ResolveFutOpt("EUR.USD", "Globex", "20140606", ctypes.c_double(1.335), "Put", "6E", 1140501, 250, 1)
call = api.trdk_ResolveFutOpt("EUR.USD", "Globex", "20140606", ctypes.c_double(1.335), "Call", "6E", 1140502, 2253, 1)


print "Completed. Press Enter to exit."
msvcrt.getch()
