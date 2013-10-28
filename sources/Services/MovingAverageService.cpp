/**************************************************************************
 *   Created: 2013/10/23 19:44:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "MovingAverageService.hpp"
#include "BarService.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

////////////////////////////////////////////////////////////////////////////////

namespace {

	enum MaType {
		MA_TYPE_SIMPLE,
		MA_TYPE_EXPONENTIAL,
		MA_TYPE_SMOOTHED,
		MA_TYPE_HULL,
		numberOfMaTypes
	};

	std::map<MaType, std::string> GetTypes() {
		std::map<MaType, std::string> result;
		static_assert(numberOfMaTypes == 4, "MA Types list changed.");
		result[MA_TYPE_SIMPLE] = "simple";
		result[MA_TYPE_EXPONENTIAL] = "exponential";
		result[MA_TYPE_SMOOTHED] = "smoothed";
		result[MA_TYPE_HULL] = "hull";
		AssertEq(numberOfMaTypes, result.size());
		return result;
	}

	enum MaSource {
		MA_SOURCE_CLOSE_PRICE,
		numberOfMaSources
	};

	std::map<MaSource, std::string> GetSources() {
		std::map<MaSource, std::string> result;
		static_assert(numberOfMaSources == 1, "MA Sources list changed.");
		result[MA_SOURCE_CLOSE_PRICE] = "close_price";
		AssertEq(numberOfMaSources, result.size());
		return result;
	}

}


////////////////////////////////////////////////////////////////////////////////

MovingAverageService::Error::Error(const char *what) throw()
		: Exception(what) {
	//...//
}

MovingAverageService::ValueDoesNotExistError::ValueDoesNotExistError(
			const char *what) throw()
		: Error(what) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

class MovingAverageService::Implementation : private boost::noncopyable {

public:

	MovingAverageService &m_service;
	
	MaType m_type;
	MaSource m_source;

public:

	explicit Implementation(
				MovingAverageService &service,
				const IniFileSectionRef &configuration)
			: m_service(service),
			m_type(MA_TYPE_SIMPLE),
			m_source(MA_SOURCE_CLOSE_PRICE) {

		const std::string typeKey = "type";
		const std::string sourceKey = "source";

		//! @todo to const std::map<MaType, std::string> &types in new MS VC
		std::map<MaType, std::string> types = GetTypes();
		if (configuration.IsKeyExist(typeKey)) {
			const auto &typeStr = configuration.ReadKey(typeKey);
			bool exists = false;
			foreach (const auto &i, types) {
				if (boost::iequals(i.second, typeStr)) {
					m_type = i.first;
					exists = true;
					break;
				}
			}
			if (!exists) {
				m_service.GetLog().Error(
					"Unknown type of Moving Average specified: \"%1%\"."
						"Supported: simple (default), exponential, smoothed"
						" and hull.",
					typeStr);
				throw Exception("Unknown type of Moving Average");
			}
		}

		//! @todo to std::map<MaSource, std::string> &sources in new MS VC
		std::map<MaSource, std::string> sources = GetSources();
		if (configuration.IsKeyExist(sourceKey)) {
			const auto &sourceStr = configuration.ReadKey(sourceKey);
			bool exists = false;
			foreach (const auto &i, sources) {
				if (boost::iequals(i.second, sourceStr)) {
					m_source = i.first;
					exists = true;
					break;
				}
			}
			if (!exists) {
				m_service.GetLog().Error(
					"Unknown Moving Average source specified: \"%1%\"."
						"Supported: close price (default).",
					sourceStr);
				throw Exception("Unknown Moving Average source");
			}
		}

		m_service.GetLog().Info(
			"Stated %1% from \"%2%\".",
			boost::make_tuple(types[m_type], sources[m_source]));

	}

	const BarService & CastToBarService(const Service &service) const {
		const BarService *const result
			= dynamic_cast<const BarService *>(&service);
		if (!result) {
			m_service.GetLog().Error(
				"Service \"%1%\" can be used as data source.",
				service);
			throw Error("Unknown service used as source");
		}
		return *result;
	}

};

////////////////////////////////////////////////////////////////////////////////

MovingAverageService::MovingAverageService(
			Context &context,
			const std::string &tag,
			const IniFileSectionRef &configuration)
		: Service(context, "MovingAverageService", tag) {
	m_pimpl = new Implementation(*this, configuration);
}

MovingAverageService::~MovingAverageService() {
	delete m_pimpl;
}

bool MovingAverageService::OnServiceDataUpdate(const Service &service) {
	const auto &barService = m_pimpl->CastToBarService(service);
	AssertLt(0, barService.GetSize());
	return true;
}

size_t MovingAverageService::GetSize() const {
	return 0;
}

bool MovingAverageService::IsEmpty() const {
	return GetSize() > 0;
}

ScaledPrice MovingAverageService::GetValue(size_t /*index*/) const {
//	if (index >= GetSize()) {
		throw ValueDoesNotExistError(
			IsEmpty()
				?	"BarService is empty"
				:	"Index is out of range of BarService");
//	}
}

ScaledPrice MovingAverageService::GetValueByReversedIndex(
			size_t /*index*/)
		const {
//	if (index >= GetSize()) {
		throw ValueDoesNotExistError(
			IsEmpty()
				?	"MovingAverageService is empty"
				:	"Index is out of range of BarService");
//	}
}

ScaledPrice MovingAverageService::GetLastValue() const {
	return GetValue(0);
}

void MovingAverageService::UpdateAlogImplSettings(
			const IniFileSectionRef &/*configuration*/) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

#ifdef BOOST_WINDOWS
	boost::shared_ptr<trdk::Service> CreateMovingAverageService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new MovingAverageService(context, tag, configuration));
	}
#else
	extern "C" boost::shared_ptr<trdk::Service> CreateMovingAverageService(
				Context &context,
				const std::string &tag,
				const IniFileSectionRef &configuration) {
		return boost::shared_ptr<trdk::Service>(
			new MovingAverageService(context, tag, configuration));
	}
#endif

////////////////////////////////////////////////////////////////////////////////
