
import ctypes
import msvcrt

api = ctypes.cdll.LoadLibrary("Trdk_dbg.dll")
api.trdk_ResolveFutOpt.restype = ctypes.c_uint
api.trdk_GetLastPrice.restype = ctypes.c_double
api.trdk_GetAskPrice.restype = ctypes.c_double
api.trdk_GetBidPrice.restype = ctypes.c_double
api.trdk_GetLastQty.restype = ctypes.c_int
api.trdk_GetAskQty.restype = ctypes.c_int
api.trdk_GetBidQty.restype = ctypes.c_int
api.trdk_GetTradedVolume.restype = ctypes.c_int

api.trdk_InitLogToStdOut()
# api.trdk_InitLog("Logs/Trdk.log")

put = api.trdk_ResolveFutOpt("EUR.USD", "Globex", "20140606", ctypes.c_double(1.335), "Put")
call = api.trdk_ResolveFutOpt("EUR.USD", "Globex", "20140606", ctypes.c_double(1.335), "Call")

print "Put ", api.trdk_GetLastPrice(put), " / ", api.trdk_GetLastQty(put), " / ", api.trdk_GetAskPrice(put), " / ", api.trdk_GetAskQty(put), " / ", api.trdk_GetBidPrice(put), " / ", api.trdk_GetBidQty(put), " / ", api.trdk_GetTradedVolume(put)
print "Call ", api.trdk_GetLastPrice(call), " / ", api.trdk_GetLastQty(call), " / ", api.trdk_GetAskPrice(call), " / ", api.trdk_GetAskQty(call), " / ", api.trdk_GetBidPrice(call), " / ", api.trdk_GetBidQty(call), " / ", api.trdk_GetTradedVolume(call)

print "Press Enter to continue..."
msvcrt.getch()

print api.trdk_GetLastPrice(put), " / ", api.trdk_GetLastQty(put), " / ", api.trdk_GetAskPrice(put), " / ", api.trdk_GetAskQty(put), " / ", api.trdk_GetBidPrice(put), " / ", api.trdk_GetBidQty(put), " / ", api.trdk_GetTradedVolume(put)
print api.trdk_GetLastPrice(call), " / ", api.trdk_GetLastQty(call), " / ", api.trdk_GetAskPrice(call), " / ", api.trdk_GetAskQty(call), " / ", api.trdk_GetBidPrice(call), " / ", api.trdk_GetBidQty(call), " / ", api.trdk_GetTradedVolume(call)

print "Press Enter to continue..."
api.trdk_DestroyAllBridges()

print "Completed. Press Enter to exit."
msvcrt.getch()
