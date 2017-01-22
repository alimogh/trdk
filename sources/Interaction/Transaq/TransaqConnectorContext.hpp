/**************************************************************************
 *   Created: 2016/10/30 22:17:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace Interaction { namespace Transaq {

	////////////////////////////////////////////////////////////////////////////////

	class ConnectorContext : private boost::noncopyable {

	public:

		typedef std::unique_ptr<
				const char,
				boost::function<void(const char *)>>
			ResultPtr;

		typedef void(NewDataSlotSignature)(
				const boost::property_tree::ptree &,
				const trdk::Lib::TimeMeasurement::Milestones &,
				const trdk::Lib::TimeMeasurement::Milestones::TimePoint &);
		typedef boost::function<NewDataSlotSignature> NewDataSlot;

	public:

		virtual ~ConnectorContext() {
			//...//
		}

	public:

		virtual boost::signals2::scoped_connection SubscribeToNewData(
				const NewDataSlot &)
				const
				= 0;

		virtual ResultPtr SendCommand(
				const char *data,
				const Lib::TimeMeasurement::Milestones &)
				= 0;

	};

	////////////////////////////////////////////////////////////////////////////////

	class TransaqConnectorContext : public ConnectorContext {

	private:

		template<typename SlotSignature>
		struct SignalTrait {
			typedef boost::signals2::signal<
				SlotSignature,
				boost::signals2::optional_last_value<
					typename boost::function_traits<SlotSignature>
						::result_type>,
				int,
				std::less<int>,
				boost::function<SlotSignature>,
				typename boost::signals2::detail::extended_signature<
						boost::function_traits<SlotSignature>::arity,
						SlotSignature>
					::function_type,
				boost::signals2::dummy_mutex>
			Signal;
		};

		typedef bool(ApiCallback)(const char *data, TransaqConnectorContext *);
		typedef bool(ApiSetCallback)(ApiCallback *, TransaqConnectorContext *);
		typedef const char *(ApiSendCommand)(const char *data);
		typedef bool (ApiFreeMemory)(const char *data);
		typedef const char *(ApiInitialize)(const char *logPath, int logLevel);
		typedef const char *(ApiUnInitialize)();

	public:

		explicit TransaqConnectorContext(const Context &, ModuleEventsLog &);
		virtual ~TransaqConnectorContext();

	public:

		virtual boost::signals2::scoped_connection SubscribeToNewData(
				const NewDataSlot &)
				const
				override;

		virtual ResultPtr SendCommand(
				const char *data,
				const Lib::TimeMeasurement::Milestones &)
				override;

	private:

		bool OnNewData(const char *);
		static bool RaiseNewDataEvent(
				const char *,
				TransaqConnectorContext *)
				noexcept;

		void FreeMemory(const char *) const noexcept;

	private:

		const Context &m_context;
		ModuleEventsLog &m_log;

		const Lib::Dll m_dll;
		ApiSendCommand *const m_sendCommand;
		ApiFreeMemory *const m_freeMemory;
		ApiUnInitialize *m_unInitialize;

		mutable SignalTrait<NewDataSlotSignature>::Signal m_signal;

	};

	////////////////////////////////////////////////////////////////////////////////

} } }
