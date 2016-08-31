/**************************************************************************
 *   Created: 2016/08/26 23:43:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::DdfPlus;

DdfPlus::Security::Security(
		Context &context,
		const Symbol &symbol,
		const trdk::MarketDataSource &source)
	: trdk::Security(context, symbol, source, true) {
	//...//
}

std::string DdfPlus::Security::GenerateDdfPlusCode() const {

	std::ostringstream result;

	switch (GetSymbol().GetSecurityType()) {
		
		case SECURITY_TYPE_FUTURES:
		
			if (GetSymbol().IsExplicit()) {
				AssertFail(
					"Explicit symbol for futures contracts is not supported.");
				throw Exception(
					"Explicit symbol for futures contracts is not supported");
			} else if (
					GetExpiration().year > 2019
					|| GetExpiration().year < 2010) {
				throw MethodDoesNotImplementedError(
					"Work with features from < 2010 or > 2019"
						" is not supported");
			}
		
			result
				<< GetSymbol().GetSymbol()
				<< char(GetExpiration().code)
				<< (GetExpiration().year - 2010);
		
			break;
		
		default:
		
			AssertFail("Security type is not supported.");
			throw Exception("Security type is not supported");
	
	}

	return result.str();

}
