
import ctypes
import msvcrt

api = ctypes.cdll.LoadLibrary("Trdk_dbg.dll")
api.trdk_GetCashBalance.restype = ctypes.c_double

api.trdk_InitLogToStdOut()
api.trdk_InitLog("Logs/Trdk.log")

api.trdk_CreateIbTwsBridge(
	"127.0.0.1",
	7496,
	"DU15239",
	"Globex",
	"20140214",
	ctypes.c_double(1.3))

print "Press Enter to continue..."
msvcrt.getch()

print "Position at start: ", api.trdk_GetPosition("DU15239", "GBPUSD")
print "Cash Balance at start: ", api.trdk_GetCashBalance("DU15239")

print "Press Enter to continue..."
msvcrt.getch()

print "Position at end: ", api.trdk_GetPosition("DU15239", "GBPUSD")
print "Cash Balance at end: ", api.trdk_GetCashBalance("DU15239")

print "Press Enter to continue..."
msvcrt.getch()

api.trdk_DestroyAllBridges()

print "Completed. Press Enter to exit."
msvcrt.getch()
