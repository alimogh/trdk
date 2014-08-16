/**************************************************************************
 *   Created: 2012/10/27 14:56:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "CsvSecurity.hpp"
#include "Core/Context.hpp"

namespace trdk { namespace Interaction { namespace Csv { 

	class MarketDataSource : public trdk::MarketDataSource {

	private:

		typedef trdk::MarketDataSource Base;

		struct ByInstrument {
			//...//
		};

		struct ByTradesRequirements {
			//...//
		};

		struct SecurityHolder {
			
			Security *security;

			explicit SecurityHolder(Security &security)
					: security(&security) {
				//...//
			}

			const Lib::Symbol & GetSymbol() const {
				return security->GetSymbol();
			}

			bool IsTradesRequired() const {
				return security->IsTradesRequired();
			}

		};

		typedef boost::multi_index_container<
			SecurityHolder,
			boost::multi_index::indexed_by<
				boost::multi_index::hashed_unique<
					boost::multi_index::tag<ByInstrument>,
					boost::multi_index::const_mem_fun<
						SecurityHolder,
						const Lib::Symbol &,
						&SecurityHolder::GetSymbol>>,
				boost::multi_index::ordered_non_unique<
					boost::multi_index::tag<ByTradesRequirements>,
					boost::multi_index::const_mem_fun<
						SecurityHolder,
						bool,
						&SecurityHolder::IsTradesRequired>>>>
			SecurityList;
		typedef SecurityList::index<ByInstrument>::type SecurityByInstrument;
		typedef SecurityList::index<ByTradesRequirements>::type
			SecurityByTradesRequirements;

	public:

		explicit MarketDataSource(
				const std::string &tag,
				const trdk::Lib::IniSectionRef &,
				trdk::Context::Log &);
		virtual ~MarketDataSource();

	public:

		virtual void Connect(const trdk::Lib::IniSectionRef &);

		virtual void SubscribeToSecurities() {
			//...//
		}

	private:

		void Subscribe(Security &) const;

		void ReadFile();

		bool ParseTradeLine(
				const std::string &line,
				boost::posix_time::ptime &,
				trdk::OrderSide &,
				std::string &symbol,
				std::string &exchange,
				trdk::ScaledPrice &,
				trdk::Qty &)
			const;

	protected:

		virtual boost::shared_ptr<trdk::Security> CreateSecurity(
					Context &,
					const trdk::Lib::Symbol &)
				const;

	private:

		Context::Log &m_log;

		const std::string m_pimaryExchange;
		const std::string m_currency;

		std::ifstream m_file;
		SecurityList m_securityList;

		boost::atomic_bool m_isStopped;
		std::unique_ptr<boost::thread> m_thread;

	};

} } }
