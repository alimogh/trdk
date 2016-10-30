/**************************************************************************
 *   Created: 2016/10/21 11:00:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FuncTestList.hpp"

using namespace trdk;
using namespace trdk::Tests;

int Tests::RunFuncTest(std::string &&testName) {

	boost::to_lower(testName);

	boost::function<void(void)> test;
	if (testName == "") {
		//...//
	} else {
		std::cerr
			<< "Func test \"" << testName << "\" is unknown."
			<< std::endl;
		return 1;
	}

	std::cout
		<< "[Tests]\t" << boost::posix_time::microsec_clock::local_time()
		<< " Starting func test \"" << testName << "\"..."
		<< std::endl;

	try {
		test();
	} catch (const std::exception &ex) {
		std::cerr
			<< "Unhandled exception: \"" << ex.what() << "\"." << std::endl;
		return 2;
	} catch (...) {
		AssertFailNoException();
		return 2;
	}

	std::cout
		<< "[Tests]\t"
		<< boost::posix_time::microsec_clock::local_time()
		<< " Func test \"" << testName << "\" is completed." << std::endl;

	return 0;

}

