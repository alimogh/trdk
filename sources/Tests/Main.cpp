/**************************************************************************
 *   Created: 2013/11/11 22:42:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"

namespace {

	struct CloseStopper : private boost::noncopyable {
		explicit CloseStopper(bool wait)
				: m_wait(wait) {
			//...//
		}
		~CloseStopper() {
			if (m_wait) {
				std::cout << "Press Enter to exit." << std::endl;
				getchar();
			}
		}
	private:
		const bool m_wait;
	};

}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	bool wait = false;
	for (int i = 1; !wait && i < argc; ++i) {
		wait = _stricmp(argv[i], "wait") == 0;
	}
	const CloseStopper closeStopper(wait);
	return RUN_ALL_TESTS();
}
