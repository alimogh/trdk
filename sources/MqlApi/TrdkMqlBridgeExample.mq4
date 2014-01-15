

#import "TrdkMqlBridge_test.dll"

// Inits logging.
void trdk_InitLog(string logFilePath);
// Initiates connection to the IB TWS. Must be calld before first IB operation.
bool trdk_CreateBridge();
// Diconnects from the IB TWS. Must be calld after last IB operation.
void trdk_DeleteBridge();

// Buys. If account not set - uses default.
int trdk_Buy(string symbol, int qty, double price, string account = "");
// Buys at market price. If account not set - uses default.
int trdk_BuyMkt(string symbol, int qty, string account = "");

// Sells. If account not set - uses default.
int trdk_Sell(string symbol, int qty, double price, string account = "");
// Sells at market price. If account not set - uses default.
int trdk_SellMkt(string symbol, int qty, string account = "");

// Current postion by sybol.
int trdk_GetPosition(string symbol);


int init() {

   // Inits log. Please use logging with test version.
   // Provide here path that suitable for you and send
   // me this log-file for issues resolving.
   trdk_InitLog("C:\\TRDK\\Logs\\event.log");

   // Initiates connection to the IB TWS and so on.
   // Must be called before any operations whith IB.
   if (!trdk_CreateBridge()) {
      Print("Failed to create bridge to IB TWS!");
   }

}


int start() {

   Print("Test started.");
   
   for (int i = 0; i < 10; i++) {
      if (i % 2 != 0) {
         Print("Testing long positions opening...");
         Print("    order ID is ", trdk_BuyMkt(Symbol(), 100001));
      } else {
         Print("Testing short positions opening...");
         Print("    order ID is ", trdk_SellMkt(Symbol(), 100002));
      }
   }
   
   trdk_DeleteBridge();
   Print("Test competed.");

   return(0);

}


