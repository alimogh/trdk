/**************************************************************************
 *   Created: 2012/09/23 20:57:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class ServiceAdapter : private boost::noncopyable {

public:

	class Error : public std::exception {
	public:
		explicit Error(const char *what);
	};

	class ServiceError : public Error {
	public:
		explicit ServiceError(const char *what);
	};

	class ConnectionError : public Error {
	public:
		explicit ConnectionError(const char *what);
	};

	struct Security {
		QString symbol;
		uint16_t scale;
	};
	typedef std::map<uint64_t, Security> SecurityList;

	typedef std::list<trader__Trade> Trades;

	typedef trader__ExchangeBook ExchangeBook;

	typedef trader__CommonParams CommonParams;

public:
	
	explicit ServiceAdapter(const QString &endpoint);
	~ServiceAdapter() throw();

public:

	const QString & GetEndpoint() const;

	void SetCurrentSymbol(const QString &);

public:

	QString DescaleAndConvert(const xsd__positiveInteger) const;

public:

	void GetSecurityList(SecurityList &result) const;
	
	void GetLastNasdaqTrades(Trades &result) const;
	void GetLastBxTrades(Trades &result) const;

	void GetNasdaqBook(ExchangeBook &result) const;
	void GetBxBook(ExchangeBook &result) const;

	void GetCommonParams(CommonParams &result) const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
