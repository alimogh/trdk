
import ctypes
import msvcrt

api = ctypes.windll.LoadLibrary("Trdk_dbg.dll")
api.trdk_GetImpliedVolatilityLast.restype = ctypes.c_double
api.trdk_GetFopLastPrice.restype = ctypes.c_double
api.trdk_GetFopBidPrice.restype = ctypes.c_double
api.trdk_GetFopAskPrice.restype = ctypes.c_double

api.trdk_InitLogToStdOut()


def print_fop(strike, right):

    strike = ctypes.c_double(strike)

    ivLast = api.trdk_GetImpliedVolatilityLast("EUR.USD", "GLOBEX", "20140905", strike, right, "6E")

    tradeQty = api.trdk_GetFopLastQty("EUR.USD", "GLOBEX", "20140905",  strike, right, "6E")
    tradePrice = api.trdk_GetFopLastPrice("EUR.USD", "GLOBEX", "20140905",  strike, right, "6E")

    bidQty = api.trdk_GetFopBidQty("EUR.USD", "GLOBEX", "20140905",  strike, right, "6E")
    bidPrice = api.trdk_GetFopBidPrice("EUR.USD", "GLOBEX", "20140905",  strike, right, "6E")

    askQty = api.trdk_GetFopAskQty("EUR.USD", "GLOBEX", "20140905",  strike, right, "6E")
    askPrice = api.trdk_GetFopAskPrice("EUR.USD", "GLOBEX", "20140905",  strike, right, "6E")

    print "{0}/{1} - {2}\t{3}/{4}\t{5}/{6}\t{7}/{8}".format(strike, right, ivLast, tradePrice, tradeQty, bidPrice, bidQty, askPrice, askQty)

while (True):

    print_fop(1.365, "Put")
    print_fop(1.37,  "Put")
    print_fop(1.37, "Call")
    print_fop(1.375, "Call")
    print "______________________"
    print_fop(1.255, "Put")
    print_fop(1.26,  "Put")
    print_fop(1.45, "Call")
    print_fop(1.455, "Call")

    print "-------------------"
    print "-------------------"
    print "-------------------"

    print "Completed. Press Enter to continue."
    msvcrt.getch()

