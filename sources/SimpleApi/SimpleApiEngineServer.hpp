/**************************************************************************
 *   Created: 2014/04/29 23:27:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk { namespace SimpleApi {

	class EngineServer : private boost::noncopyable {

	public:

		typedef size_t EngineId;

		class Exception : public trdk::Lib::Exception {
		public:
			typedef trdk::Lib::Exception Base;
		public:
			explicit Exception(const char *what) noexcept;
		};

		class UnknownEngineError : public Exception {
		public:
			UnknownEngineError() noexcept;
		};

		class UnknownAccountError : public Exception {
		public:
			UnknownAccountError() noexcept;
		};

	public:

		EngineServer();
		~EngineServer();

	public:

		EngineId CreateEngine();

		void DestoryEngine(const EngineId &);
		void DestoryAllEngines();

		Engine & CheckEngine(const EngineId &);

		Engine & GetEngine(const EngineId &);
		const Engine & GetEngine(const EngineId &) const;

		trdk::EventsLog & GetLog();

	public:

		void InitLog(const boost::filesystem::path &);

	private:

		class Implementation;
		std::unique_ptr<Implementation> m_pimpl;

	};

} }
