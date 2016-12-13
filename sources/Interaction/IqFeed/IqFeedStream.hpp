/**************************************************************************
 *   Created: 2016/11/20 20:35:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Common/NetworkStreamClientService.hpp"

namespace trdk { namespace Interaction { namespace IqFeed {

	////////////////////////////////////////////////////////////////////////////////

	class ClientService : public Lib::NetworkStreamClientService {

	public:

		explicit ClientService(
				const std::string &name,
				IqFeed::MarketDataSource &);
		virtual ~ClientService() noexcept;

	public:

		IqFeed::MarketDataSource & GetSource();
		const IqFeed::MarketDataSource & GetSource() const;

		void SubscribeToMarketData(const IqFeed::Security &);

	protected:

		virtual boost::posix_time::ptime GetCurrentTime() const override;

		virtual void LogDebug(const char *) const override;
		virtual void LogInfo(const std::string &) const override;
		virtual void LogError(const std::string &) const override;

		virtual void OnConnectionRestored() override;
		virtual void OnStopByError(const std::string &) override;

	private:

		IqFeed::MarketDataSource &m_source;

	};

	////////////////////////////////////////////////////////////////////////////////

	class Stream : public ClientService {

	public:

		explicit Stream(IqFeed::MarketDataSource &);
		virtual ~Stream() noexcept;
	
	protected:

		virtual std::unique_ptr<trdk::Lib::NetworkStreamClient> CreateClient()
				override;

	};

	////////////////////////////////////////////////////////////////////////////////

	class History : public ClientService {

	public:

		explicit History(IqFeed::MarketDataSource &);
		virtual ~History() noexcept;

	protected:

		virtual std::unique_ptr<trdk::Lib::NetworkStreamClient> CreateClient()
				override;

	};

	////////////////////////////////////////////////////////////////////////////////

} } }
