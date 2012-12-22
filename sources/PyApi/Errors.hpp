/**************************************************************************
 *   Created: 2012/11/09 00:21:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace PyApi {

	class Error : public Trader::Lib::ModuleError {
	public:
		explicit Error(const char *what) throw()
				: ModuleError(what) {
			//...//
		}
	};

	class ClientError : public Error {
	public:
		explicit ClientError(const char *what) throw()
				: Error(what) {
			//...//
		}
	};

	class PureVirtualMethodHasNoImplementation : public ClientError {
	public:
		explicit PureVirtualMethodHasNoImplementation(const char *what) throw()
				: ClientError(what) {
			//...//
		}
	};

} }
