
#import "TrdkMqlBridge_test.dll"

// Inits logging.
void trdk_InitLog(string logFilePath);

// Initiates connection to the IB TWS. Must be called before first IB operation.
// "twsHost" and "twsPort" can be diffrent for each TWS instance (see TWS
// settings "Global Configuration" -> "API"). If "twsHost" is empty ("") -
// localhost will be used, if twsPort is 0 (zero) - default port 7496 will be
// used.
bool trdk_CreateBridge(
	string twsHost,
	int twsPort,
	string account,
	string exchange,
	string expirationDate,
	double strike);

// Diconnects from the IB TWS. trdk_DestroyBridge or trdk_DestroyAllBridges
// must be called after last IB operation.
void trdk_DestroyBridge(string account);
// Diconnects all bridges for all accounts. trdk_DestroyBridge or
// trdk_DestroyAllBridges must be called after last IB operation.
void trdk_DestroyAllBridges();

// Buy. Returns order ID.
int trdk_Buy(string account, string symbol, string option, int qty, double price);
// Buy at market price. Returns order ID.
int trdk_BuyMkt(string account, string option, string symbol, int qty);

// Sells. Returns order ID.
int trdk_Sell(string account, string symbol, string option, int qty, double price);
// Sells at market price. Returns order ID.
int trdk_SellMkt(string account, string option, string symbol, int qty);

// Current postion by symbol.
int trdk_GetPosition(string account, string symbol);
// All positions by account. Returs positions count. symbolsResult and sizeResult
// if it required get positions count only.
int trdk_GetAllPositions(
	string account,
	string &symbolsResult[],
	int symbolsResultBufferSize,
	int &sizeResult[],
	int sizeResultBufferSize);

// Current cash balance by account.
double trdk_GetCashBalance(string account);

int init() {

   // Inits log. Please use logging with test version.
   // Provide here path that suitable for you and send
   // me this log-file for issues resolving.
   trdk_InitLog("C:\\TRDK\\Logs\\event.log");

   // Initiates connection to the IB TWS. Must be called before any
   // operations whith IB. For default connection parameters can be used
   // trdk_CreateBridge("", 0, "DU15070", ...).
   if (!trdk_CreateBridge("127.0.0.1", 7496, "DU15070", "Globex", "20140214", 1.3)) {
      Print("Failed to create bridge to IB TWS!");
   }

}


int start() {

   int i;

   Print("Test started.");
   
   Print("Cash balance at start: ", trdk_GetCashBalance("DU15070"));
   
   Print("Position at start: ", Symbol(), " - ", trdk_GetPosition("DU15070", Symbol()));

   Print("Test position iterate at start...");
   string symbols[16]; 
   int sizes[16]; 
   int positionsCount = trdk_GetAllPositions(
      "DU15070",
      symbols,
      ArraySize(symbols),
      sizes,
      ArraySize(sizes));
   for (i = 0; i < positionsCount && i < ArraySize(symbols) && i < ArraySize(sizes); i++) {
      Print("    ", symbols[i], sizes[i]);
   }

   for (i = 0; i < 10; i++) {
      if (i % 2 != 0) {
         Print(
            "Testing buy order. Order ID: ",
            trdk_BuyMkt("DU15070", Symbol(), "Put", 100001));
      } else {
         Print(
            "Testing sell order. Order ID: ",
            trdk_SellMkt("DU15070", Symbol(), "Call", 100002));
      }
   }

   Print("Cash balance at end: ", trdk_GetCashBalance("DU15070"));

   Print("Position at end: ", Symbol(), " - ", trdk_GetPosition("DU15070", Symbol()));

   Print("Test position iterate at end...");
   positionsCount = trdk_GetAllPositions(
      "DU15070",
      symbols,
      ArraySize(symbols),
      sizes,
      ArraySize(sizes));
   for (i = 0; i < positionsCount && i < ArraySize(symbols) && i < ArraySize(sizes); i++) {
      Print("    ", symbols[i], sizes[i]);
   }
   
   trdk_DestroyAllBridges();
   Print("Test competed.");

   return(0);

}
