/*******************************************************************************
 *   Created: 2017/09/20 19:54:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

namespace trdk {
namespace Interaction {
namespace FixProtocol {

struct Settings;
class Security;
class MarketDataSource;
class TradingSystem;
class Client;
class Policy;
class Handler;

namespace Incoming {
class Logon;
class Logout;
class Heartbeat;
class TestRequest;
class ResendRequest;
class Reject;
class MarketDataSnapshotFullRefresh;
class MarketDataIncrementalRefresh;
}

namespace Outgoing {
class StandardHeader;
class Message;
}
}
}
}
