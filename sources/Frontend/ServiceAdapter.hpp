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

	typedef trader__Position Position;

public:
	
	explicit ServiceAdapter(const QString &endpoint);
	~ServiceAdapter() throw();

public:

	const QString & GetEndpoint() const;

	void SetCurrentSymbol(const QString &);

public:

	double DescalePrice(xsd__positiveInteger) const;
	QString DescalePriceAndConvert(xsd__positiveInteger) const;
	xsd__positiveInteger ScalePrice(double) const;

public:

	void GetSecurityList(SecurityList &result) const;
	
	void GetLastNasdaqTrades(Trades &result) const;
	void GetLastBxTrades(Trades &result) const;

	void GetNasdaqBook(ExchangeBook &result) const;
	void GetBxBook(ExchangeBook &result) const;

	void GetCommonParams(CommonParams &result) const;

	void GetPositionInfo(Position &result) const;

public:

	void OrderBuy(const QString &venue, double price, uint64_t qty);
	void OrderBuyMkt(const QString &venue, uint64_t qty);
	void OrderSell(const QString &venue, double price, uint64_t qty);
	void OrderSellMkt(const QString &venue, uint64_t qty);
	void OrderShort(const QString &venue, double price, uint64_t qty);

private:

	class Implementation;
	Implementation *m_pimpl;

};
