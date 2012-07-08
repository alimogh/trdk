/**************************************************************************
 *   Created: 2012/07/09 17:43:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include <list>
#include "Foreach.hpp"
#include "Assert.hpp"

template<typename Slot, typename Connection>
class SignalConnection : public Connection {

public:

	typedef Connection Base;

public:
	
	SignalConnection()
			: Base() {
		//...//
	}

	explicit SignalConnection(const Base &connection)
			: Base(connection) {
		//...//
	}

};

template<typename ConnectionT>
class SignalConnectionList {

public:

	typedef ConnectionT Connection;
	typedef std::list<Connection> List;

public:

	SignalConnectionList() {
		//...//
	}

	SignalConnectionList(const SignalConnectionList &rhs)
			: m_list(rhs.m_list) {
		//...//
	}
	
	~SignalConnectionList() {
		try {
			Diconnect();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

	const SignalConnectionList & operator =(const SignalConnectionList &rhs) {
		SignalConnectionList(rhs).Swap(*this);
		return *this;
	}
	
	void Swap(SignalConnectionList &rhs) {
		rhs.m_list.swap(m_list);
	}

public:

	bool IsConnected() const {
		if (m_list.empty()) {
			return false;
		}
		foreach (const auto &c, m_list) {
			if (c.connected()) {
				return false;
			}
		}
		return true;
	}

	void Diconnect() {
		foreach (auto &c, m_list) {
			c.disconnect();
		}
	}

	void Insert(const Connection &c) {
		m_list.push_back(c);
	}

	void InsertSafe(const Connection &c) {
		try {
			Insert(c);
		} catch (...) {
			c.disconnect();
			throw;
		}
	}

private:

	List m_list;
	
};
