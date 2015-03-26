/**************************************************************************
 *   Created: 2015/03/22 21:34:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

class Client : public boost::enable_shared_from_this<Client> {

public:

	class Handler {

	public:
		
		virtual ~Handler() {
			//...//
		}

		virtual void OnConnectionClosed(Client &) = 0;

		virtual void OnDataSent(Client &, const char *data, size_t size) = 0;
		virtual void OnNewData(Client &, const char *data, size_t size) = 0;

	};
	
private:
	
	explicit Client(boost::asio::io_service &, Handler &, int flag);

public:

	~Client();

public:

	static boost::shared_ptr<Client> Create(
			boost::asio::io_service &,
			Handler &handler,
			int flag);

public:

	void Send(const boost::shared_ptr<std::vector<char>> &);
	void Send(const char *);

	void Start();

	boost::asio::ip::tcp::socket & GetSocket() {
		return m_socket;
	}

	int GetFlag() const {
		return m_flag;
	}

private:

	void StartRead();

	void OnDataSent(
			const boost::shared_ptr<std::vector<char>> &,
			const boost::system::error_code &,
			size_t bytesTransferred);

	void OnNewData(
			const boost::system::error_code &,
			size_t bytesTransferred);

private:

	const int m_flag;

	Handler &m_handler;

	std::vector<char> m_inBuffer;

	boost::asio::ip::tcp::socket m_socket;

};
