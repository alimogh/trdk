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

	class Error : public Exception {
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

	typedef std::list<trader__FirstUpdate> FirstUpdateData;

public:
	
	explicit ServiceAdapter(const QString &endpoint);
	~ServiceAdapter() throw();

public:

	const QString & GetEndpoint() const;

public:

	void GetSecurityList(SecurityList &result) const;
	void GetFirstUpdateData(
			const QString &symbol,
			FirstUpdateData &result)
		const;

private:

	class Implementation;
	Implementation *m_pimpl;

};
