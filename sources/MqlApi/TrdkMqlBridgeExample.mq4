

#import "TrdkMqlBridge_test.dll"

// Inits logging.
void trdk_InitLog(string logFilePath);

// Initiates connection to the IB TWS. Must be called before first IB operation.
// "twsHost" and "twsPort" can be diffrent for each TWS instance (see TWS
// settings "Global Configuration" -> "API"). If "twsHost" is empty ("") -
// localhost will be used, if twsPort is 0 (zero) - default port 7496 will be
// used.
bool trdk_CreateBridge(string twsHost, int twsPort, string account);

// Diconnects from the IB TWS. trdk_DestroyBridge or trdk_DestroyAllBridges
// must be called after last IB operation.
void trdk_DestroyBridge(string account);
// Diconnects all bridges for all accounts. trdk_DestroyBridge or
// trdk_DestroyAllBridges must be called after last IB operation.
void trdk_DestroyAllBridges();

// Buy. Returns order ID.
int trdk_Buy(string account, string symbol, int qty, double price);
// Buy at market price. Returns order ID.
int trdk_BuyMkt(string account, string symbol, int qty);

// Sells. Returns order ID.
int trdk_Sell(string account, string symbol, int qty, double price);
// Sells at market price. Returns order ID.
int trdk_SellMkt(string account, string symbol, int qty);

// Current postion by sybol.
int trdk_GetPosition(string account, string symbol);


int init() {

   // Inits log. Please use logging with test version.
   // Provide here path that suitable for you and send
   // me this log-file for issues resolving.
   trdk_InitLog("C:\\TRDK\\Logs\\event.log");

   // Initiates connection to the IB TWS. Must be called before any
   // operations whith IB. For default connection parameters can be used
   // trdk_CreateBridge("", 0, "DU15060").
   if (!trdk_CreateBridge("127.0.0.1", 7496, "DU15060")) {
      Print("Failed to create bridge to IB TWS!");
   }

}


int start() {

   Print("Test started.");
   Print("Position at start: ", Symbol(), trdk_GetPosition("DU15060", Symbol()));
   
   for (int i = 0; i < 10; i++) {
      if (i % 2 != 0) {
         Print("Testing long positions opening...");
         Print("    order ID is ", trdk_BuyMkt("DU15060", Symbol(), 100001));
      } else {
         Print("Testing short positions opening...");
         Print("    order ID is ", trdk_SellMkt("DU15060", Symbol(), 100002));
      }
   }

   Print("Position at end: ", Symbol(), trdk_GetPosition("DU15060", Symbol()));
   
   trdk_DestroyAllBridges();
   Print("Test competed.");

   return(0);

}


