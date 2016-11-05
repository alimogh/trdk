/**************************************************************************
 *   Created: 2012/09/23 14:31:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Module.hpp"
#include "ModuleSecurityList.hpp"
#include "Service.hpp"
#include "Security.hpp"
#include "EventsLog.hpp"
#include "TradingLog.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

Module::SecurityList::SecurityList() {
	//...//
}

Module::SecurityList::~SecurityList() {
	//...//
}

Module::SecurityList::Iterator::Iterator(Implementation *pimpl)
		: m_pimpl(pimpl) {
	Assert(m_pimpl);
}

Module::SecurityList::Iterator::Iterator(const Iterator &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

Module::SecurityList::Iterator::~Iterator() {
	delete m_pimpl;
}

Module::SecurityList::Iterator &
Module::SecurityList::Iterator::operator =(const Iterator &rhs) {
	Assert(this != &rhs);
	Iterator(rhs).Swap(*this);
	return *this;
}

void Module::SecurityList::Iterator::Swap(Iterator &rhs) {
	Assert(this != &rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}

Security & Module::SecurityList::Iterator::dereference() const {
	return *m_pimpl->iterator->second;
}

bool Module::SecurityList::Iterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

bool Module::SecurityList::Iterator::equal(
			const ConstIterator &rhs)
		const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

void Module::SecurityList::Iterator::increment() {
	++m_pimpl->iterator;
}

void Module::SecurityList::Iterator::decrement() {
	--m_pimpl->iterator;
}

void Module::SecurityList::Iterator::advance(const difference_type &n) {
	std::advance(m_pimpl->iterator, n);
}

Module::SecurityList::ConstIterator::ConstIterator(Implementation *pimpl)
	: m_pimpl(pimpl) {
	Assert(m_pimpl);
}

Module::SecurityList::ConstIterator::ConstIterator(const Iterator &rhs)
	: m_pimpl(new Implementation(rhs.m_pimpl->iterator)) {
	//...//
}

Module::SecurityList::ConstIterator::ConstIterator(const ConstIterator &rhs)
		: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

Module::SecurityList::ConstIterator::~ConstIterator() {
	delete m_pimpl;
}

Module::SecurityList::ConstIterator &
Module::SecurityList::ConstIterator::operator =(const ConstIterator &rhs) {
	Assert(this != &rhs);
	ConstIterator(rhs).Swap(*this);
	return *this;
}

void Module::SecurityList::ConstIterator::Swap(ConstIterator &rhs) {
	Assert(this != &rhs);
	std::swap(m_pimpl, rhs.m_pimpl);
}

const Security & Module::SecurityList::ConstIterator::dereference() const {
	return *m_pimpl->iterator->second;
}

bool Module::SecurityList::ConstIterator::equal(
			const ConstIterator &rhs)
		const {
	Assert(this != &rhs);
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

bool Module::SecurityList::ConstIterator::equal(const Iterator &rhs) const {
	return m_pimpl->iterator == rhs.m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::increment() {
	++m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::decrement() {
	--m_pimpl->iterator;
}

void Module::SecurityList::ConstIterator::advance(const difference_type &n) {
	std::advance(m_pimpl->iterator, n);
}

//////////////////////////////////////////////////////////////////////////

namespace {

	boost::atomic<Module::InstanceId> nextFreeInstanceId(1);
	
	std::string FormatStringId(
			const Module::InstanceId &instanceId,
			const std::string &typeName,
			const std::string &name,
			const std::string &tag) {
		std::ostringstream result;
		result << typeName << '.' << name;
		if (name != tag) {
			result << '.' << tag;
		}
		result << '.' << instanceId;
		return result.str();
	}

}

class Module::Implementation : private boost::noncopyable {

public:

	const InstanceId m_instanceId;

	Mutex m_mutex;

	Context &m_context;
	
	const std::string m_typeName;
	const std::string m_name;
	const std::string m_tag;
	const std::string m_stringId;

	Module::Log m_log;
	Module::TradingLog m_tradingLog;

	explicit Implementation(
			Context &context,
			const std::string &typeName,
			const std::string &name,
			const std::string &tag)
		: m_instanceId(nextFreeInstanceId++)
		, m_context(context)
		, m_typeName(typeName)
		, m_name(name)
		, m_tag(tag)
		, m_stringId(FormatStringId(m_instanceId, m_typeName, m_name, m_tag))
		, m_log(m_stringId, m_context.GetLog())
		, m_tradingLog(m_tag, m_context.GetTradingLog()) {
		Assert(nextFreeInstanceId.is_lock_free());
	}

};

Module::Module(
		Context &context,
		const std::string &typeName,
		const std::string &name,
		const std::string &tag)
	: m_pimpl(
		boost::make_unique<Implementation>(context, typeName, name, tag)) {
	//...//
}

Module::~Module() {
	//...//
}

const Module::InstanceId & Module::GetInstanceId() const {
	return m_pimpl->m_instanceId;
}

Module::Lock Module::LockForOtherThreads() {
	return Lock(m_pimpl->m_mutex);
}

const std::string & Module::GetTypeName() const noexcept {
	return m_pimpl->m_typeName;
}

const std::string & Module::GetName() const noexcept {
	return m_pimpl->m_name;
}

const std::string & Module::GetTag() const noexcept {
	return m_pimpl->m_tag;
}

const std::string & Module::GetStringId() const noexcept {
	return m_pimpl->m_stringId;
}

Context & Module::GetContext() {
	return m_pimpl->m_context;
}

const Context & Module::GetContext() const {
	return const_cast<Module *>(this)->GetContext();
}

Module::Log & Module::GetLog() const noexcept {
	return m_pimpl->m_log;
}

Module::TradingLog & Module::GetTradingLog() const noexcept {
	return m_pimpl->m_tradingLog;
}

std::string Module::GetRequiredSuppliers() const {
	return std::string();
}

void Module::OnServiceStart(const Service &) {
	//...//
}

void Module::OnSettingsUpdate(const trdk::Lib::IniSectionRef &) {
	//...//
}

void Module::RaiseSettingsUpdateEvent(const IniSectionRef &conf) {
	const auto lock = LockForOtherThreads();
	OnSettingsUpdate(conf);
}

namespace {
	
	typedef boost::mutex SettingsReportMutex;
	typedef SettingsReportMutex::scoped_lock SettingsReportLock;
	static SettingsReportMutex mutex;

	typedef std::map<
			std::string,
			std::list<std::pair<std::string, std::string>>>
		SettingsReportCache; 
	static SettingsReportCache settingsReportcache;

}

//////////////////////////////////////////////////////////////////////////

std::ostream & trdk::operator <<(std::ostream &oss, const Module &module) {
	oss << module.GetStringId();
	return oss;
}

//////////////////////////////////////////////////////////////////////////
