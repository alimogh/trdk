/**************************************************************************
 *   Created: 2015/05/22 16:12:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Exception.hpp"

namespace trdk { namespace EngineServer {

	class Settings
		: private boost::noncopyable,
		public boost::enable_shared_from_this<Settings> {

	public:

		typedef std::unordered_map<
				std::string /* section */,
				std::unordered_map<std::string /* key */, std::string /* value */>>
			ClientSettingsGroup;

		typedef std::unordered_map<std::string /* group */, ClientSettingsGroup>
			ClientSettings;

	private:

		typedef boost::shared_mutex Mutex;
		typedef boost::shared_lock<Mutex> ReadLock;
		typedef boost::unique_lock<Mutex> WriteLock;

	public:

		class Transaction : private boost::noncopyable {
		public:
			explicit Transaction(
					const boost::shared_ptr<Settings> &,
					const std::string &groupName);
			Transaction(Transaction &&);
			virtual ~Transaction();
		public:
			Settings & GetSettings() {
				return *m_settings;
			}
			const Settings & GetSettings() const {
				return const_cast<Transaction *>(this)->GetSettings();
			}
		public:
			void Set(
					const std::string &section,
					const std::string &key,
					const std::string &value);
			void CopyFromActual();
		public:
			void Commit();
		protected:
			void CheckBeforeChange();
			EngineServer::Exception OnError(const std::string &error);
			virtual void OnKeyStore(
				const std::string &section,
				const std::string &key,
				std::string &value)
				const
				= 0;
		private:
			void Store();
		protected:
			const boost::shared_ptr<Settings> m_settings;
			const std::string m_groupName;
			ClientSettingsGroup m_clientSettings;
			bool m_hasErrors;
		private:
			std::unique_ptr<WriteLock> m_lock;
			bool m_isCommitted;
		};
		class EngineTransaction : public Transaction {
		public:
			explicit EngineTransaction(const boost::shared_ptr<Settings> &);
			EngineTransaction(EngineTransaction &&);
			virtual ~EngineTransaction();
		protected:
			virtual void OnKeyStore(
				const std::string &section,
				const std::string &key,
				std::string &value)
				const;
		};
		class StrategyTransaction : public Transaction {
		public:
			explicit StrategyTransaction(
					const boost::shared_ptr<Settings> &,
					const std::string &groupName);
			StrategyTransaction(StrategyTransaction &&);
			virtual ~StrategyTransaction();
		public:
			void Start();
			void Stop();
		protected:
			virtual void OnKeyStore(
				const std::string &section,
				const std::string &key,
				std::string &value)
				const;
		};

	public:
	
		explicit Settings(
				const boost::filesystem::path &,
				const std::string &serviceName);
		explicit Settings(
				const boost::filesystem::path &,
				const std::string &serviceName,
				const std::string &engineId);

	public:

		const std::string & GetEngeineId() const;
		const boost::filesystem::path & GetFilePath() const;

	public:

		template<typename Callback>
		void GetClientSettings(const Callback &callback) const {
			const ReadLock lock(m_mutex);
			callback(m_clientSettins);
		}

	public:

		EngineTransaction StartEngineTransaction();
		StrategyTransaction StartStrategyTransaction(
				const std::string &strategyId);

	private:

		void LoadClientSettings(const WriteLock &);

	private:

		const std::string m_engineId;

		const boost::filesystem::path m_baseSettingsPath;
		const boost::filesystem::path m_actualSettingsPath;
		
		mutable Mutex m_mutex;

		ClientSettings m_clientSettins;
	
	};

} }
