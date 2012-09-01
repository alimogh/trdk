/**************************************************************************
 *   Created: 2012/08/30 00:50:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "GatewayMessage.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	template<typename BufferT>
	class GatewayClientMessage : public GatewayMessage {

	public:

		enum Type {
			TYPE_LOGIN_REQUEST	= 'L',
			TYPE_NEW_ORDER		= 'O',
			TYPE_HEARTBEAT		= 'R'
		};

		typedef BufferT Buffer;
		typedef std::basic_ostream<typename Buffer::char_type, typename Buffer::traits_type>
			MessageFormatter;

	public:

		class Error : public GatewayMessage::Error {
		public:
			explicit Error(const char *what, const std::string &subject)
					: GatewayMessage::Error(what),
					m_subject(subject) {
				//...//
			}
			virtual ~Error() {
				//...//
			}
		public:
			virtual const std::string & GetSubject() const {
				return m_subject;
			}
		private:
			std::string m_subject;
		};

		class FieldTooLongError : public Error {
		public:
			FieldTooLongError(const std::string &field)
					: Error(
						"Lightspeed Gateway client message field too long",
						field) {
				//...//
			}
		};

	public:

		explicit GatewayClientMessage(Type type)
				: m_formatter(&m_buffer) {
			AppendField(Char(type));
		}

	public:

		Buffer & GetMessage() {
			return m_buffer;
		}

		Len GetMessageLen() const {
			return m_buffer.size();
		}

	public:

		void AppendField(Char ch) {
			m_formatter.put(ch);
		}

		void AppendField(BuySellIndicator ch) {
			AppendField(Char(ch));
		}

		void AppendField(const std::string &val, Len len) {
			AppendField(val, len, ' ', std::ios::left);
		}

		void AppendField(Numeric val, Len len) {
			AppendField(boost::lexical_cast<std::string>(val), len, ' ', std::ios::right);
		}

		void AppendField(double val, Len len) {
			AppendField(boost::lexical_cast<std::string>(val), len, '0', std::ios::right);
		}

		void AppendSpace(Len len) {
			m_formatter << std::setw(len) << ' ';
		}

	private:

		void AppendField(
					const std::string &val,
					Len len,
					char fill,
					std::ios::fmtflags flags) {
			if (val.size() > len) {
				throw FieldTooLongError(val);
			}
			m_formatter.flags(flags);
			m_formatter << std::setfill(fill) << std::setw(len) << val;
		}

	private:

		Buffer m_buffer;
		MessageFormatter m_formatter;

	};

} } }

