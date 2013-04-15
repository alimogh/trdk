/**************************************************************************
 *   Created: 2012/11/09 00:21:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi {

	class Error : public trdk::Lib::ModuleError {
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
