/**************************************************************************
 *   Created: 2013/12/15 21:58:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"

BOOL WINAPI DllMain(
			_In_  HINSTANCE /*dllHandle*/,
			_In_  DWORD reason,
			_In_  LPVOID /*reserved*/) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		default:
			AssertEq(DLL_PROCESS_ATTACH, reason);
	}
	return TRUE;
}
