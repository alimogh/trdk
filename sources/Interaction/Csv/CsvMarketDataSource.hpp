/**************************************************************************
 *   Created: 2012/10/27 14:56:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "Core/MarketDataSource.hpp"
#include "CsvSecurity.hpp"

namespace Trader { namespace Interaction { namespace Csv { 

	class MarketDataSource : public Trader::MarketDataSource {

	private:

		struct ByInstrument {
			//...//
		};

		struct ByTradesRequirements {
			//...//
		};

		struct SecurityHolder {
			
			boost::shared_ptr<Security> security;

			explicit SecurityHolder(const boost::shared_ptr<Security> &security)
					: security(security) {
				//...//
			}

			const std::string & GetSymbol() const {
				return security->GetSymbol();
			}

			const std::string & GetPrimaryExchange() const {
				return security->GetPrimaryExchange();
			}

			const std::string & GetExchange() const {
				return security->GetExchange();
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
					boost::multi_index::composite_key<
						SecurityHolder,
						boost::multi_index::const_mem_fun<
							SecurityHolder,
							const std::string &,
							&SecurityHolder::GetSymbol>,
						boost::multi_index::const_mem_fun<
							SecurityHolder,
							const std::string &,
							&SecurityHolder::GetPrimaryExchange>,
						boost::multi_index::const_mem_fun<
							SecurityHolder,
							const std::string &,
							&SecurityHolder::GetExchange>>>,
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
					const Trader::Lib::IniFile &,
					const std::string &section);
		virtual ~MarketDataSource();

	public:

		virtual void Connect();

	public:

		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					boost::shared_ptr<Trader::TradeSystem>,
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;
		
		virtual boost::shared_ptr<Trader::Security> CreateSecurity(
					const std::string &symbol,
					const std::string &primaryExchange,
					const std::string &exchange,
					boost::shared_ptr<const Trader::Settings>,
					bool logMarketData)
				const;

	private:

		void Subscribe(const boost::shared_ptr<Security> &) const;

		void ReadFile();

		bool ParseTradeLine(
				const std::string &line,
				boost::posix_time::ptime &,
				Trader::OrderSide &,
				std::string &symbol,
				std::string &exchange,
				Trader::ScaledPrice &,
				Trader::Qty &)
			const;

	private:

		const std::string m_pimaryExchange;

		std::ifstream m_file;
		SecurityList m_securityList;

		volatile long m_isStopped;
		std::unique_ptr<boost::thread> m_thread;

	};

} } }
