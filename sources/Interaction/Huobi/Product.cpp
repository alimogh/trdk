//
//    Created: 2018/07/27 6:52 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Product.hpp"
#include "Request.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Huobi;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

namespace {
boost::optional<OrderRequirements> GetOrderRequirements(
    const std::string &symbol) {
  if (symbol == "AST_BTC") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "ACT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "ADX_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "ABT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "APPC_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "AIDOC_BTC") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "BAT_BTC") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "BCH_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "BCX_BTC") {
    return boost::make_optional(OrderRequirements{1, 100000000});
  }
  if (symbol == "BTG_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "BCD_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "BTM_BTC") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "BIFI_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 50000});
  }
  if (symbol == "BLZ_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "CMT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 10000000});
  }
  if (symbol == "CVC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 2000000});
  }
  if (symbol == "CHAT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "DASH_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "DGD_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "DBC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "DAT_BTC") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "DTA_BTC") {
    return boost::make_optional(OrderRequirements{1, 100000000});
  }
  if (symbol == "ETC_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 10000});
  }
  if (symbol == "ETH_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "ELF_BTC") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "EOS_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "EKO_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "EVX_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "ELA_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 50000});
  }
  if (symbol == "ENG_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 200000});
  }
  if (symbol == "EDU_BTC") {
    return boost::make_optional(OrderRequirements{1, 100000000});
  }
  if (symbol == "GNT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 5000000});
  }
  if (symbol == "GNX_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "GAS_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "HT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "HSR_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 20000});
  }
  if (symbol == "ITC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 5000000});
  }
  if (symbol == "ICX_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 100000});
  }
  if (symbol == "IOST_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "KNC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 2000000});
  }
  if (symbol == "LTC_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 10000});
  }
  if (symbol == "LET_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "LUN_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 20000});
  }
  if (symbol == "LSK_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 50000});
  }
  if (symbol == "LINK_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "MANA_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "MTL_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 300000});
  }
  if (symbol == "MCO_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "MDS_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "MEE_BTC") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "MTN_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "MTX_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "NEO_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "NAS_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "OMG_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 200000});
  }
  if (symbol == "OST_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "OCN_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ONT_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "PAY_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "POWR_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "PROPY_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "QASH_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "QSP_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "QTUM_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "QUN_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "RDN_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "RCN_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "REQ_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "RPX_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "RUFF_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SALT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 100000});
  }
  if (symbol == "SBTC_BTC") {
    return boost::make_optional(OrderRequirements{
        0.0001,
    });
  }
  if (symbol == "SMT_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SNT_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 2000000});
  }
  if (symbol == "STORJ_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 2000000});
  }
  if (symbol == "SWFTC_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SOC_BTC") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "STK_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SRN_BTC") {
    return boost::make_optional(OrderRequirements{0.01, 300000});
  }
  if (symbol == "SNC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "TNB_BTC") {
    return boost::make_optional(OrderRequirements{1, 50000000});
  }
  if (symbol == "TNT_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "TRX_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "TOPC_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "THETA_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "UTK_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "VEN_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "WAX_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 1000000});
  }
  if (symbol == "WPR_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "WICC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "XRP_BTC") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "XEM_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "YEE_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ZEC_BTC") {
    return boost::make_optional(OrderRequirements{0.001, 5000});
  }
  if (symbol == "ZRX_BTC") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "ZIL_BTC") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ZLA_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "CTXC_BTC") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "BTC_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 1000});
  }
  if (symbol == "BCH_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "CVC_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "DTA_USDT") {
    return boost::make_optional(OrderRequirements{1, 20000000});
  }
  if (symbol == "DASH_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "DBC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "ETH_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 1000});
  }
  if (symbol == "ETC_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 200000});
  }
  if (symbol == "EOS_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 1000000});
  }
  if (symbol == "ELF_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "ELA_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "GNT_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "HT_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "HSR_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 20000});
  }
  if (symbol == "ITC_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 500000});
  }
  if (symbol == "IOST_USDT") {
    return boost::make_optional(OrderRequirements{1, 20000000});
  }
  if (symbol == "LET_USDT") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "LTC_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 100000});
  }
  if (symbol == "MDS_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 5000000});
  }
  if (symbol == "NEO_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "NAS_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "OMG_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 1000000});
  }
  if (symbol == "QTUM_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 1000000});
  }
  if (symbol == "RUFF_USDT") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "SNT_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "STORJ_USDT") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "SMT_USDT") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "TRX_USDT") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "THETA_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 3000000});
  }
  if (symbol == "VEN_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "XRP_USDT") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "XEM_USDT") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "ZEC_USDT") {
    return boost::make_optional(OrderRequirements{0.001, 5000});
  }
  if (symbol == "ZIL_USDT") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ACT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "ADX_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "ABT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "APPC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 100000});
  }
  if (symbol == "AIDOC_ETH") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "BAT_ETH") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "BTM_ETH") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "BLZ_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "CMT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 10000000});
  }
  if (symbol == "CVC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 2000000});
  }
  if (symbol == "CHAT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "DGD_ETH") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "DBC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "DAT_ETH") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "DTA_ETH") {
    return boost::make_optional(OrderRequirements{1, 100000000});
  }
  if (symbol == "EOS_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "ELF_ETH") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "EVX_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "EKO_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ELA_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 50000});
  }
  if (symbol == "ENG_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 200000});
  }
  if (symbol == "EDU_ETH") {
    return boost::make_optional(OrderRequirements{1, 100000000});
  }
  if (symbol == "GNT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 5000000});
  }
  if (symbol == "GAS_ETH") {
    return boost::make_optional(OrderRequirements{0.001, 10000});
  }
  if (symbol == "GNX_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "HT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "HSR_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 20000});
  }
  if (symbol == "ITC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 5000000});
  }
  if (symbol == "ICX_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 100000});
  }
  if (symbol == "IOST_ETH") {
    return boost::make_optional(OrderRequirements{1, 1000000});
  }
  if (symbol == "LET_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "LUN_ETH") {
    return boost::make_optional(OrderRequirements{0.001, 20000});
  }
  if (symbol == "LSK_ETH") {
    return boost::make_optional(OrderRequirements{0.001, 50000});
  }
  if (symbol == "LINK_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "MANA_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "MCO_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "MDS_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "MEE_ETH") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "MTN_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "MTX_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "NAS_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "OMG_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 200000});
  }
  if (symbol == "OST_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "OCN_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ONT_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "PAY_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "POWR_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "PROPY_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "QASH_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "QSP_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "QTUM_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 100000});
  }
  if (symbol == "QUN_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "RDN_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "RCN_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "REQ_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "RUFF_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SALT_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 100000});
  }
  if (symbol == "SMT_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SWFTC_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SOC_ETH") {
    return boost::make_optional(OrderRequirements{1, 5000000});
  }
  if (symbol == "STK_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "SRN_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 300000});
  }
  if (symbol == "SNC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "TNT_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "TNB_ETH") {
    return boost::make_optional(OrderRequirements{1, 50000000});
  }
  if (symbol == "TRX_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "TOPC_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "THETA_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "UTK_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "VEN_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 500000});
  }
  if (symbol == "WICC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "WPR_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 1000000});
  }
  if (symbol == "WAX_ETH") {
    return boost::make_optional(OrderRequirements{0.01, 1000000});
  }
  if (symbol == "YEE_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ZIL_ETH") {
    return boost::make_optional(OrderRequirements{1, 10000000});
  }
  if (symbol == "ZLA_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  if (symbol == "CTXC_ETH") {
    return boost::make_optional(OrderRequirements{0.1, 1000000});
  }
  return boost::none;
}
}  // namespace

boost::unordered_map<std::string, Product> Huobi::RequestProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, Product> result;
  ptr::ptree response;
  try {
    response = boost::get<1>(
        PublicRequest("v1/common/symbols", "", context, log).Send(session));
    for (const auto &node : response.get_child("data")) {
      const auto &data = node.second;
      auto symbol = data.get<std::string>("base-currency") + "_" +
                    data.get<std::string>("quote-currency");
      boost::to_upper(symbol);
      result.emplace(
          std::move(symbol),
          Product{data.get<std::string>("symbol"), GetOrderRequirements(symbol),
                  data.get<uint8_t>("price-precision"),
                  data.get<uint8_t>("amount-precision")});
    }
  } catch (const std::exception &ex) {
    log.Error(
        R"(Failed to read supported product list: "%1%". Message: "%2%".)",
        ex.what(),                                // 1
        Rest::ConvertToString(response, false));  // 2
    throw Exception(ex.what());
  }
  if (result.empty()) {
    throw Exception("Exchange doesn't have products");
  }
  return result;
}
