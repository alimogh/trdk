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
#include "Util.hpp"

using namespace trdk::Lib;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace mi = boost::multi_index;

////////////////////////////////////////////////////////////////////////////////

ContractExpiration::ContractExpiration(const gr::date &date) : m_date(date) {}

bool ContractExpiration::operator==(const ContractExpiration &rhs) const {
  return m_date == rhs.m_date;
}

bool ContractExpiration::operator!=(const ContractExpiration &rhs) const {
  return m_date != rhs.m_date;
}

bool ContractExpiration::operator<(const ContractExpiration &rhs) const {
  return m_date < rhs.m_date;
}

bool ContractExpiration::operator>(const ContractExpiration &rhs) const {
  return m_date > rhs.m_date;
}

std::ostream &trdk::Lib::operator<<(std::ostream &os,
                                    const ContractExpiration &expiration) {
  os << expiration.m_date << " (" << expiration.GetContract(true) << ')';
  return os;
}

std::ostream &trdk::Lib::operator<<(std::ostream &os,
                                    const ContractExpiration::Code &code) {
  os << char(code);
  return os;
}

ContractExpiration::Code ContractExpiration::GetCode() const {
  switch (m_date.month()) {
    case gr::Jan:
      return ContractExpiration::CODE_FEBRUARY;
    case gr::Feb:
      return ContractExpiration::CODE_MARCH;
    case gr::Mar:
      return ContractExpiration::CODE_APRIL;
    case gr::Apr:
      return ContractExpiration::CODE_MAY;
    case gr::May:
      return ContractExpiration::CODE_JUNE;
    case gr::Jun:
      return ContractExpiration::CODE_JULY;
    case gr::Jul:
      return ContractExpiration::CODE_AUGUST;
    case gr::Aug:
      return ContractExpiration::CODE_SEPTEMBER;
    case gr::Sep:
      return ContractExpiration::CODE_OCTOBER;
    case gr::Oct:
      return ContractExpiration::CODE_NOVEMBER;
    case gr::Nov:
      return ContractExpiration::CODE_DECEMBER;
    case gr::Dec:
      return ContractExpiration::CODE_JANUARY;
    default:
      AssertEq(gr::Jan, m_date.month());
      throw SystemException("Unknown expiration date month");
  }
}

std::uint16_t ContractExpiration::GetYear() const {
  uint16_t result = m_date.year();
  if (m_date.month() == gr::Dec) {
    ++result;
  }
  return result;
}

const gr::date &ContractExpiration::GetDate() const { return m_date; }

std::string ContractExpiration::GetContract(bool isShort) const {
  std::ostringstream result;

  result << GetCode();

  const auto &year = GetYear();
  if (year > 2019 || year < 2010) {
    throw MethodDoesNotImplementedError(
        "Work with features from < 2010 or > 2019 is not implemented");
  }
  result << (year - (isShort ? 2010 : 2000));

  return result.str();
}

////////////////////////////////////////////////////////////////////////////////

namespace {

struct ByExpirationDate {};

typedef boost::multi_index_container<
    ContractExpiration,
    mi::indexed_by<
        mi::ordered_unique<mi::tag<ByExpirationDate>,
                           mi::const_mem_fun<ContractExpiration,
                                             const gr::date &,
                                             &ContractExpiration::GetDate>>>>
    SymbolContracts;

typedef boost::unordered_map<std::string, SymbolContracts> ContractsBySymbol;
}

////////////////////////////////////////////////////////////////////////////////

class ExpirationCalendar::IteratorImplementation {
 public:
  typedef SymbolContracts::index<ByExpirationDate>::type Index;

 public:
  const Index &m_cont;
  Index::iterator m_it;

 public:
  explicit IteratorImplementation(const Index &cont,
                                  const Index::const_iterator &it)
      : m_cont(cont), m_it(it) {}

  void operator=(const IteratorImplementation &) = delete;
};

ExpirationCalendar::Iterator::Iterator() {}

ExpirationCalendar::Iterator::Iterator(
    std::unique_ptr<IteratorImplementation> &&impl)
    : m_pimpl(std::move(impl)) {}

ExpirationCalendar::Iterator::Iterator(const Iterator &rhs) {
  if (rhs.m_pimpl) {
    m_pimpl = boost::make_unique<IteratorImplementation>(*rhs.m_pimpl);
  }
}

ExpirationCalendar::Iterator::~Iterator() {}

ExpirationCalendar::Iterator &ExpirationCalendar::Iterator::operator=(
    const Iterator &rhs) {
  Iterator(rhs).Swap(*this);
  return *this;
}

void ExpirationCalendar::Iterator::Swap(Iterator &rhs) noexcept {
  m_pimpl.swap(rhs.m_pimpl);
}

ExpirationCalendar::Iterator::operator bool() const {
  return m_pimpl && m_pimpl->m_it != m_pimpl->m_cont.cend();
}

const ContractExpiration &ExpirationCalendar::Iterator::dereference() const {
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

void ExpirationCalendar::Iterator::decrement() {
  Assert(m_pimpl);
  if (m_pimpl->m_it == m_pimpl->m_cont.cbegin()) {
    m_pimpl.reset();
  } else {
    --m_pimpl->m_it;
  }
}

void ExpirationCalendar::Iterator::advance(const difference_type &offset) {
  Assert(m_pimpl);
  std::advance(m_pimpl->m_it, offset);
}

////////////////////////////////////////////////////////////////////////////////

class ExpirationCalendar::Implementation {
 public:
  ContractsBySymbol m_contracts;

 public:
  void operator=(const Implementation &) = delete;
};

ExpirationCalendar::ExpirationCalendar()
    : m_pimpl(boost::make_unique<Implementation>()) {}

ExpirationCalendar::ExpirationCalendar(const ExpirationCalendar &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}

ExpirationCalendar &ExpirationCalendar::operator=(
    const ExpirationCalendar &rhs) {
  ExpirationCalendar(rhs).Swap(*this);
  return *this;
}

ExpirationCalendar::~ExpirationCalendar() {}

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
    gr::date expirationDate;

    typedef boost::split_iterator<std::string::iterator> SpitIterator;
    for (auto it = boost::make_split_iterator(
             row, boost::first_finder(",", boost::is_iequal()));
         it != SpitIterator(); ++it) {
      std::string field = boost::copy_range<std::string>(*it);
      boost::trim(field);
      if (field.empty()) {
        break;
      }

      if (symbol.empty()) {
        Assert(expirationDate.is_not_a_date());
        symbol = std::move(field);

      } else if (expirationDate.is_not_a_date()) {
        try {
          expirationDate = ConvertToDateFromYyyyMmDd(field);
        } catch (const std::exception &ex) {
          boost::format error(
              "Wrong expiration date format in CSV-file %1%"
              " at line %2%: \"%3%\"");
          error % filePath % numberOfLines % ex.what();
          throw Exception(error.str().c_str());
        }
      } else {
        boost::format error("Too many fields in CSV-file %1% at line %2%");
        error % filePath % numberOfLines;
        throw Exception(error.str().c_str());
      }
    }

    if (symbol.empty() || expirationDate.is_not_a_date()) {
      boost::format error("Wrong CSV-file format in %1% at line %2%");
      error % filePath % numberOfLines;
      throw Exception(error.str().c_str());
    }

    const auto &insertResult = contracts[symbol].emplace(expirationDate);
    if (!insertResult.second) {
      boost::format error("Non-unique value in CSV-file %1% at line %2%");
      error % filePath % numberOfLines;
      throw Exception(error.str().c_str());
    }
  }

  contracts.swap(m_pimpl->m_contracts);
}

ExpirationCalendar::Iterator ExpirationCalendar::Find(
    const Symbol &symbol, const gr::date &startDate) const {
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
  const auto &begin = contract.lower_bound(startDate);
  if (begin == contract.end() || begin->GetDate() < startDate) {
    return Iterator();
  }

  return Iterator(boost::make_unique<IteratorImplementation>(contract, begin));
}

ExpirationCalendar::Iterator ExpirationCalendar::Find(
    const Symbol &symbol,
    pt::ptime contractStartTime,
    const pt::time_duration &sessionOpeningTime) const {
  if (contractStartTime.time_of_day() < sessionOpeningTime) {
    contractStartTime -= pt::hours(24);
  }
  return Find(symbol, contractStartTime.date());
}

ExpirationCalendar::Stat ExpirationCalendar::CalcStat() const {
  Stat result = {};
  for (const auto &symbol : m_pimpl->m_contracts) {
    ++result.numberOfSymbols;
    result.numberOfExpirations += symbol.second.size();
  }
  return result;
}

void ExpirationCalendar::Insert(const Symbol &symbol,
                                const ContractExpiration &expiration) {
  m_pimpl->m_contracts[symbol.GetSymbol()].emplace(expiration);
}

////////////////////////////////////////////////////////////////////////////////
