/**************************************************************************
 *   Created: 2015/03/19 21:44:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Interaction { namespace Itch {

	class DataHandler {

	public:
	
		virtual ~DataHandler() {
			//...//
		}
	
	public:

		virtual void OnNewOrder(
				const boost::posix_time::ptime &time,
				bool isBuy,
				const char *pair,
				const Itch::OrderId &,
				double price,
				double amount)
				= 0;
	
		virtual void OnOrderModify(
				const boost::posix_time::ptime &time,
				const char *pair,
				const Itch::OrderId &,
				double amount)
				= 0;
	
		virtual void OnOrderCancel(
				const boost::posix_time::ptime &time,
				const char *pair,
				const Itch::OrderId &)
				= 0;

		virtual void Flush(
				const boost::posix_time::ptime &,
				const Lib::TimeMeasurement::Milestones &)
				= 0;
	
	public:

		virtual void OnDebug(const std::string &message) = 0;
		virtual void OnErrorFromServer(const std::string &error) = 0;
		virtual void OnConnectionClosed(
				const std::string &reason,
				bool isError)
				= 0;
	
	};

} } }
