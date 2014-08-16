/**************************************************************************
 *   Created: 2012/09/23 14:31:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Security.hpp"
#include "Context.hpp"
#include "Fwd.hpp"
#include "Api.h"

////////////////////////////////////////////////////////////////////////////////

namespace trdk {

	class TRDK_CORE_API Module : private boost::noncopyable {

	public:

		class TRDK_CORE_API Log;

		class TRDK_CORE_API SecurityList;

		typedef uintmax_t InstanceId;

	protected:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		
	public:

		explicit Module(
					trdk::Context &,
					const std::string &typeName,
					const std::string &name,
					const std::string &tag);
		virtual ~Module();

	public:

		trdk::Module::InstanceId GetInstanceId() const;

		trdk::Context & GetContext();
		const trdk::Context & GetContext() const;

		const std::string & GetTypeName() const throw();
		const std::string & GetName() const throw();
		const std::string & GetTag() const throw();

	public:

		trdk::Module::Log & GetLog() const throw();

	public:

		//! Returns list of required services.
		virtual std::string GetRequiredSuppliers() const;

	public:

		virtual void OnServiceStart(const trdk::Service &);

	public:

		void UpdateSettings(const trdk::Lib::IniSectionRef &);

	protected:

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniSectionRef &)
				= 0;

		Mutex & GetMutex() const;

		void ReportSettings(const trdk::SettingsReport::Report &) const;

	private:

		class Implementation;
		Implementation *m_pimpl;

	};

}

////////////////////////////////////////////////////////////////////////////////

class trdk::Module::Log : private boost::noncopyable {
public:
	explicit Log(const Module &);
	~Log();
public:
	void Debug(const char *str) throw() {
		m_log.DebugEx(
			[this, str]() -> boost::format {
				return std::move(GetFormat() % str);
			});
	}
	template<typename Params>
	void Debug(const char *str, const Params &params) throw() {
		m_log.DebugEx(
			[this, str, &params]() -> boost::format {
				boost::format result((GetFormat() % str).str());
				trdk::Lib::Format(params, result);
				return std::move(result);
			});
	}
	template<typename Callback>
	void DebugEx(const Callback &callback) throw() {
		m_log.DebugEx(
			[this, &callback]() -> boost::format {
				return std::move(GetFormat() % callback().str());
			});
	}
public:
	void Info(const char *str) throw() {
		m_log.InfoEx(
			[this, str]() -> boost::format {
				return std::move(GetFormat() % str);
			});
	}
	template<typename Params>
	void Info(const char *str, const Params &params) throw() {
		m_log.InfoEx(
			[this, str, &params]() -> boost::format {
				boost::format result((GetFormat() % str).str());
				trdk::Lib::Format(params, result);
				return std::move(result);
			});
	}
	template<typename Callback>
	void InfoEx(const Callback &callback) throw() {
		m_log.InfoEx(
			[this, &callback]() -> boost::format {
				return std::move(GetFormat() % callback().str());
			});
	}
public:
	void Warn(const char *str) throw() {
		m_log.WarnEx(
			[this, str]() -> boost::format {
				return std::move(GetFormat() % str);
			});
	}
	template<typename Params>
	void Warn(const char *str, const Params &params) throw() {
		m_log.WarnEx(
			[this, str, &params]() -> boost::format {
				boost::format result((GetFormat() % str).str());
				trdk::Lib::Format(params, result);
				return std::move(result);
			});
	}
	template<typename Callback>
	void WarnEx(const Callback &callback) throw() {
		m_log.WarnEx(
			[this, &callback]() -> boost::format {
				return std::move(GetFormat() % callback().str());
			});
	}
public:
	void Error(const char *str) throw() {
		m_log.ErrorEx(
			[this, str]() -> boost::format {
				return std::move(GetFormat() % str);
			});
	}
	template<typename Params>
	void Error(const char *str, const Params &params) throw() {
		m_log.ErrorEx(
			[this, str, &params]() -> boost::format {
				boost::format result((GetFormat() % str).str());
				trdk::Lib::Format(params, result);
				return std::move(result);
			});
	}
	template<typename Callback>
	void ErrorEx(const Callback &callback) throw() {
		m_log.ErrorEx(
			[this, &callback]() -> boost::format {
				return std::move(GetFormat() % callback().str());
			});
	}
public:
	template<typename Params>
	void Trading(const char *str) throw() {
		m_log.Trading(m_tag, str);
	}
	template<typename Callback>
	void TradingEx(const Callback &callback) throw() {
		m_log.TradingEx(m_tag, callback);
	}
	template<typename Params>
	void Trading(const char *str, const Params &params) throw() {
		m_log.Trading(m_tag, str, params);
	}
private:
	boost::format GetFormat() const {
		return m_format;
	}
private:
	const std::string m_tag;
	trdk::Context::Log &m_log;
	const boost::format m_format;
};

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API trdk::Module::SecurityList {

public:

	class ConstIterator;
		
	class TRDK_CORE_API Iterator
			: public boost::iterator_facade<
				Iterator,
				trdk::Security,
				boost::bidirectional_traversal_tag> {
		friend class trdk::Module::SecurityList::ConstIterator;
	public:
		class Implementation;
	public:
		explicit Iterator(Implementation *);
		Iterator(const Iterator &);
		~Iterator();
	public:
		Iterator & operator =(const Iterator &);
		void Swap(Iterator &);
	public:
		trdk::Security & dereference() const;
		bool equal(const Iterator &) const;
		bool equal(const ConstIterator &) const;
		void increment();
		void decrement();
		void advance(difference_type);
	private:
		Implementation *m_pimpl;
	};

	class TRDK_CORE_API ConstIterator
			: public boost::iterator_facade<
				ConstIterator,
				const trdk::Security,
				boost::bidirectional_traversal_tag> {
		friend class trdk::Module::SecurityList::Iterator;
	public:
		class Implementation;
	public:
		explicit ConstIterator(Implementation *);
		explicit ConstIterator(const Iterator &);
		ConstIterator(const ConstIterator &);
		~ConstIterator();
	public:
		ConstIterator & operator =(const ConstIterator &);
		void Swap(ConstIterator &);
	public:
		const trdk::Security & dereference() const;
		bool equal(const Iterator &) const;
		bool equal(const ConstIterator &) const;
		void increment();
		void decrement();
		void advance(difference_type);
	private:
		Implementation *m_pimpl;
	};
		
public:

	SecurityList();
	virtual ~SecurityList();

public:

	virtual size_t GetSize() const = 0;
	virtual bool IsEmpty() const = 0;

	virtual Iterator GetBegin() = 0;
	virtual ConstIterator GetBegin() const = 0;

	virtual Iterator GetEnd() = 0;
	virtual ConstIterator GetEnd() const = 0;

	virtual Iterator Find(const trdk::Lib::Symbol &) = 0;
	virtual ConstIterator Find(const trdk::Lib::Symbol &) const = 0;

};

//////////////////////////////////////////////////////////////////////////

namespace std {
	TRDK_CORE_API std::ostream & operator <<(
				std::ostream &,
				const trdk::Module &)
			throw();
}

//////////////////////////////////////////////////////////////////////////
