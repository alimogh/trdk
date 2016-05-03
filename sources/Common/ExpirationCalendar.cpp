/**************************************************************************
 *   Created: 2016/05/02 16:54:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ExpirationCalendar.hpp"

using namespace trdk::Lib;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace mi = boost::multi_index;

////////////////////////////////////////////////////////////////////////////////

namespace {

	struct ByExpirationDate {
	};

	typedef boost::multi_index_container<
		ExpirationCalendar::Contract,
		mi::indexed_by<
			mi::ordered_unique<
				mi::tag<ByExpirationDate>,
				mi::member<
					ExpirationCalendar::Contract,
					gr::date,
					&ExpirationCalendar::Contract::expirationDate>>>>
		SymbolContracts;

	typedef boost::unordered_map<std::string, SymbolContracts>
		ContractsBySymbol;

}

////////////////////////////////////////////////////////////////////////////////

class ExpirationCalendar::IteratorImplementation {

public:

	typedef SymbolContracts::index<ByExpirationDate>::type Index;

public:

	const Index &m_cont;
	Index::iterator m_it;

public:

	explicit IteratorImplementation(
			const Index &cont,
			const Index::const_iterator &it)
		: m_cont(cont)
		, m_it(it) {
		//...//
	}

	bool IsEnd() const {
		return m_it == m_cont.end();
	}

	IteratorImplementation(const IteratorImplementation &) = delete;
	void operator =(const IteratorImplementation &) = delete;

};

ExpirationCalendar::Iterator::Iterator() {
	//...//
}

ExpirationCalendar::Iterator::Iterator(
		const boost::shared_ptr<IteratorImplementation> &impl)
	: m_pimpl(impl) {
	//...//
}

ExpirationCalendar::Iterator::~Iterator() {
	//...//
}

ExpirationCalendar::Iterator::operator bool() const {
	return m_pimpl && !m_pimpl->IsEnd();
}

const ExpirationCalendar::Contract & ExpirationCalendar::Iterator::dereference()
		const {
	Assert(m_pimpl);
	return *m_pimpl->m_it;
}

bool ExpirationCalendar::Iterator::equal(const Iterator &rhs) const {
	Assert(m_pimpl);
	return m_pimpl->m_it == rhs.m_pimpl->m_it;
}

void ExpirationCalendar::Iterator::increment() {
	Assert(m_pimpl);
	++m_pimpl->m_it;
}

void ExpirationCalendar::Iterator::advance(const difference_type &offset) {
	Assert(m_pimpl);
	std::advance(m_pimpl->m_it, offset);
}

////////////////////////////////////////////////////////////////////////////////

class ExpirationCalendar::Implementation {

public:

	static const pt::time_duration m_maxExpirationPeriod;

	ContractsBySymbol m_contracts;

public:

	void operator =(const Implementation &) = delete;

};

const pt::time_duration
ExpirationCalendar::Implementation::m_maxExpirationPeriod
	= pt::hours(24 * int(30 * 1.5));

ExpirationCalendar::ExpirationCalendar()
	: m_pimpl(new Implementation) {
	//...//
}

ExpirationCalendar::ExpirationCalendar(const ExpirationCalendar &rhs)
	: m_pimpl(new Implementation(*rhs.m_pimpl)) {
	//...//
}

ExpirationCalendar & ExpirationCalendar::operator =(
		const ExpirationCalendar &rhs) {
	ExpirationCalendar(rhs).Swap(*this);
	return *this;
}

ExpirationCalendar::~ExpirationCalendar() {
	//...//
}

void ExpirationCalendar::Swap(ExpirationCalendar &rhs) noexcept {
	std::swap(m_pimpl, rhs.m_pimpl);
}

void ExpirationCalendar::ReloadCsv(const fs::path &filePath) {

	ContractsBySymbol contracts;
	
	std::ifstream file(filePath.string().c_str());
	if (!file) {
		boost::format error("Filed to open CSV-file %1% with expiration info");
		error % filePath;
		throw Exception(error.str().c_str());
	}

	for (size_t numberOfLines = 1; file.good(); ++numberOfLines) {

		std::string row;
		std::getline(file, row);
		boost::trim(row);
		if (row.empty()) {
			continue;
		}

		std::string symbol;
		Contract contract = {};

		typedef boost::split_iterator<std::string::iterator> SpitIterator;
		for (
				auto it = boost::make_split_iterator(
					row,
					boost::first_finder(",", boost::is_iequal()));
				it != SpitIterator();
				++it) {
			
			std::string field = boost::copy_range<std::string>(*it);
			boost::trim(field);
			if (field.empty()) {
				break;
			}
			
			if (symbol.empty()) {
			
				Assert(contract.expirationDate.is_not_a_date());
				symbol = std::move(field);
			
			} else if (contract.expirationDate.is_not_a_date()) {
			
				uint32_t expirationDateInt;
				try {
					expirationDateInt = boost::lexical_cast<uint32_t>(field);
				} catch (const boost::bad_lexical_cast &) {
					boost::format error(
						"Wrong expiration date format in CSV-file %1%"
							" at line %2%");
					error % filePath % numberOfLines;
					throw Exception(error.str().c_str());
				}

				struct {
					uint16_t year;
					uint8_t month;
					uint8_t day;
				} date;
				date.year = uint16_t(expirationDateInt / 10000);
				date.month = uint8_t(
					(expirationDateInt - (date.year * 10000)) / 100);
				if (date.month < 1 || date.month > 12) {
					boost::format error(
						"Wrong expiration month format in CSV-file %1%"
							" at line %2%");
					error % filePath % numberOfLines;
					throw Exception(error.str().c_str());
				}
				date.day = uint8_t(
					expirationDateInt
					- (date.year * 10000)
					- (date.month * 100));
				if (date.day < 1 || date.day > 31) {
					boost::format error(
						"Wrong expiration month day format in CSV-file %1%"
							" at line %2%");
					throw Exception(error.str().c_str());
				}
				contract.expirationDate
					= gr::date(date.year, date.month, date.day);

			} else {
			
				boost::format error(
					"Too many fields in CSV-file %1% at line %2%");
				error % filePath % numberOfLines;
				throw Exception(error.str().c_str());
			
			}
		
		}

		if (symbol.empty() || contract.expirationDate.is_not_a_date()) {
			boost::format error("Wrong CSV-file format in %1% at line %2%");
			error % filePath % numberOfLines;
			throw Exception(error.str().c_str());
		}

		contract.year = contract.expirationDate.year();

		switch (contract.expirationDate.month()) {
			case gr::Jan:
				contract.code = CODE_FEBRUARY;
				break;
			case gr::Feb:
				contract.code = CODE_MARCH;
				break;
			case gr::Mar:
				contract.code = CODE_APRIL;
				break;
			case gr::Apr:
				contract.code = CODE_MAY;
				break;
			case gr::May:
				contract.code = CODE_JUNE;
				break;
			case gr::Jun:
				contract.code = CODE_JULY;
				break;
			case gr::Jul:
				contract.code = CODE_AUGUST;
				break;
			case gr::Aug:
				contract.code = CODE_SEPTEMBER;
				break;
			case gr::Sep:
				contract.code = CODE_OCTOBER;
				break;
			case gr::Oct:
				contract.code = CODE_NOVEMBER;
				break;
			case gr::Nov:
				contract.code = CODE_DECEMBER;
				break;
			case gr::Dec:
				contract.code = CODE_JANUARY;
				++contract.year;
				break;
			default:
				AssertEq(gr::Jan, contract.expirationDate.month());
				throw SystemException("Unknown expiration date month");
		}

		const auto &insertResult = contracts[symbol].emplace(
			std::move(contract));
		if (!insertResult.second) {
			boost::format error("Non-unique value in CSV-file %1% at line %2%");
			error % filePath % numberOfLines;
			throw Exception(error.str().c_str());
		}

	}

	for (const auto &symbol: contracts) {
		const auto &index = symbol.second.get<ByExpirationDate>();
		Assert(!index.empty());
		for (auto i = std::next(index.begin()); i != index.end(); ++i) {
			const auto prevExpirationDate = std::prev(i)->expirationDate;
			const pt::ptime prev(prevExpirationDate);
			const pt::ptime current(i->expirationDate);
			if (prev + m_pimpl->m_maxExpirationPeriod < current) {
				boost::format error(
					"No info about expiration between %1% and %2% for \"%3%\""
					" in CSV-file %4%");
				error
					% prevExpirationDate
					% i->expirationDate
					% symbol.first
					% filePath;
				throw Exception(error.str().c_str());
			}
		}
	}

	contracts.swap(m_pimpl->m_contracts);

}

ExpirationCalendar::Iterator ExpirationCalendar::Find(
		const Symbol &symbol,
		const pt::ptime &start)
		const {

	if (symbol.GetSecurityType() != SECURITY_TYPE_FUTURES) {
		boost::format error(
			"No info about expiration for \"%1%\" - wrong security type");
		error % symbol;
		throw Exception(error.str().c_str());
	}

	const auto &contractPos = m_pimpl->m_contracts.find(symbol.GetSymbol());
	if (contractPos == m_pimpl->m_contracts.cend()) {
		return Iterator();
	}

	const auto &contract = contractPos->second.get<ByExpirationDate>();
	const auto &begin = contract.lower_bound(start.date());
	if (begin == contract.end()) {
		return Iterator();
	}
	
	const pt::ptime expirationDate(begin->expirationDate);
	if (
			expirationDate < start
			|| start + m_pimpl->m_maxExpirationPeriod < expirationDate) {
		return Iterator();
	}

	return Iterator(
		boost::make_shared<IteratorImplementation>(contract, begin));

}

ExpirationCalendar::Stat ExpirationCalendar::CalcStat() const {
	Stat result = {};
	for (const auto &symbol: m_pimpl->m_contracts) {
		++result.numberOfSymbols;
		result.numberOfExpirations += symbol.second.size();
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////
