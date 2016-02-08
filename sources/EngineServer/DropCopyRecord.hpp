/*******************************************************************************
 *   Created: 2015/12/20 15:35:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk { namespace EngineServer {

	typedef boost::variant<
			std::string,
			boost::shared_ptr<const FinancialResult>,
			boost::uuids::uuid,
			boost::posix_time::ptime,
			double,
			intmax_t,
			uintmax_t,
			Qty,
			Lib::Currency,
			TimeInForce,
			OrderSide,
			OrderStatus,
			OrderType,
			OperationResult,
			TradingMode>
		DropCopyRecordField;

	typedef boost::unordered_map<
			std::string,
			DropCopyRecordField>
		DropCopyRecord;

} }

namespace trdk { namespace EngineServer { namespace Details {

	template <typename Stream>
	class DropCopyRecordFieldMsgpackVisitor : public boost::static_visitor<> {

	public:

		explicit DropCopyRecordFieldMsgpackVisitor(Stream &streamRef)
			: m_stream(streamRef) {
			//...//
		}

		void operator ()(
				const boost::shared_ptr<const FinancialResult> &record) {
			m_stream.pack_map(record->size());
			foreach (const auto &i, *record) {
				m_stream.pack(Lib::ConvertToIso(i.first));
				m_stream.pack(i.second);
			}
		}

		void operator ()(const Lib::Currency &value) {
			m_stream.pack(Lib::ConvertToIso(value));
		}

		template<typename T>
		void operator ()(const Lib::Numeric<T> &value) {
			m_stream.pack(value.Get());
		}

		void operator ()(const TimeInForce &value) {
			m_stream.pack(int(value));
		}

		void operator ()(const TradingMode &value) {
			m_stream.pack(int(value));
		}

		void operator ()(const OrderSide &value) {
			m_stream.pack(int(value));
		}

		void operator ()(const OrderStatus &value) {
			m_stream.pack(int(value));
		}

		void operator ()(const OrderType &value) {
			m_stream.pack(int(value));
		}

		void operator ()(const OperationResult &value) {
			m_stream.pack(int(value));
		}

		void operator ()(const boost::uuids::uuid &value) {
			m_stream.pack(boost::uuids::to_string(value));
		}

		void operator ()(const boost::posix_time::ptime &value) {
			m_stream.pack(Lib::ConvertToMicroseconds(value));
		}

		template<typename T>
		void operator ()(const T &value) {
			m_stream.pack(value);
		}

	private:

		Stream &m_stream;

	};

	class DropCopyLogStringVisitor
		: public boost::static_visitor<std::string> {

	public:

		std::string operator ()(
				const boost::shared_ptr<const FinancialResult> &record)
				const {
			std::ostringstream result;
			result << '[';
			for (auto i = record->begin(); i != record->end(); ++i) {
				result << Lib::ConvertToIso(i->first) << i->second;
				if (std::next(i) != record->end()) {
					result << ", ";
				}
			}
			result << ']';
			return result.str();
		}

		std::string operator ()(const Lib::Currency &value) const {
			return Lib::ConvertToIso(value);
		}

		std::string operator ()(const boost::uuids::uuid &value) const {
			return boost::uuids::to_string(value);
		}

		std::string operator ()(const boost::posix_time::ptime &value) const {
			std::ostringstream result;
			result << value << " (" << Lib::ConvertToTimeT(value) << ')';
			return result.str();
		}

		std::string operator ()(float value) const {
			std::ostringstream result;
			result << std::fixed << std::setprecision(8) << value;
			return result.str();
		}
		std::string operator ()(double value) const {
			std::ostringstream result;
			result << std::fixed << std::setprecision(8) << value;
			return result.str();
		}

		template<typename T>
		std::string operator ()(const T &value) const {
			return boost::lexical_cast<std::string>(value);
		}

	};

} } }

namespace trdk { namespace EngineServer {

	inline std::string ConvertToLogString(const DropCopyRecord &record) {
		std::ostringstream result;
		foreach (const auto &i, record) {
			using namespace trdk::EngineServer::Details;
			result
				<< i.first << '='
				<< boost::apply_visitor(DropCopyLogStringVisitor(), i.second)
				<< ';' << '\t';
		}
		return result.str();
	}

} }
