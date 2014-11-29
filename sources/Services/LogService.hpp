/**************************************************************************
 *   Created: 2014/11/28 02:43:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"

namespace trdk { namespace Services {

	class TRDK_SERVICES_API LogService : public trdk::Service {

	public:

		explicit LogService(
					Context &,
					const std::string &tag,
					const Lib::IniSectionRef &);
		virtual ~LogService();

	public:

		virtual boost::posix_time::ptime OnSecurityStart(
					const trdk::Security &);

		virtual bool OnBookUpdateTick(
				const trdk::Security &,
				const trdk::BookUpdateTick &,
				const trdk::Lib::TimeMeasurement::Milestones &);


	protected:

		virtual void UpdateAlogImplSettings(
					const trdk::Lib::IniSectionRef &);

	private:

		std::map<std::string, std::ofstream> m_files;


	};

} }
