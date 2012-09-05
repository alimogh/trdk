/**************************************************************************
 *   Created: 2012/08/28 23:28:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

#include "GatewayMessage.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	template<typename BufferT>
	class GatewayTsMessage : public GatewayMessage {

	public:

		enum Type {
			TYPE_LOGIN_ACCEPTED	= 'A',
			TYPE_LOGIN_REJECTED	= 'J',
			TYPE_HEARTBEAT		= 'H',
			TYPE_DEBUG			= '+',
			TYPE_ORDER_ACCEPTED,
			TYPE_ORDER_REJECTED,
			TYPE_ORDER_CANCELED,
			TYPE_ORDER_EXECUTED
		};

		typedef BufferT Buffer;
		typedef typename Buffer::const_iterator Iterator;

	public:

		class Error : public GatewayMessage::Error {
		public:
			explicit Error(const char *what, Iterator messageBegin, Iterator messageEnd)
					: GatewayMessage::Error(what),
					m_buffer(messageBegin, messageEnd) {
				//...//
			}
			virtual ~Error() {
				//...//
			}
		public:
			virtual const std::string & GetSubject() const {
				return m_buffer;
			}
		private:
			std::string m_buffer;
		};
	
		class MessageNotGatawayMessageError : public Error {
		public:
			MessageNotGatawayMessageError(Iterator messageBegin, Iterator messageEnd)
					: Error(
						"Message not Lightspeed Gateway trade system message",
						messageBegin,
						messageEnd) {
				//...//
			}
		};

		class FieldDoesntExistError : public Error {
		public:
			FieldDoesntExistError(Iterator messageBegin, Iterator messageEnd)
					: Error(
						"Field doesn't exist in Lightspeed Gateway trade system message",
						messageBegin,
						messageEnd) {
				//...//
			}
		};
		
		class FieldHasInvalidFormatError : public Error {
		public:
			FieldHasInvalidFormatError(Iterator messageBegin, Iterator messageEnd)
					: Error(
						"Lightspeed Gateway trade system message field has invalid format",
						messageBegin,
						messageEnd) {
				//...//
			}
		};

		class FieldHasInvalidLenError : public Error {
		public:
			FieldHasInvalidLenError(Iterator messageBegin, Iterator messageEnd)
					: Error(
						"Lightspeed Gateway trade system message has invalid length",
						messageBegin,
						messageEnd) {
				//...//
			}
		};

	private:

		enum DataType {
			DATA_TYPE_ORDER_ACCEPTED	= 'A',
			DATA_TYPE_ORDER_REJECTED	= 'J',
			DATA_TYPE_ORDER_CANCELED	= 'C',
			DATA_TYPE_ORDER_EXECUTED	= 'E'
		};

	public:

		GatewayTsMessage(Iterator messageBegin, Iterator messageEnd)
				: m_messageBegin(messageBegin),
				m_messageLogicalBegin(m_messageBegin),
				m_messageEnd(messageEnd),
				m_logicalLen(std::distance(messageBegin, messageEnd)) {
			if (!m_logicalLen) {
				throw MessageNotGatawayMessageError(m_messageBegin, m_messageEnd);
			}
			switch (*m_messageBegin) {
				case TYPE_DEBUG:
				case TYPE_LOGIN_ACCEPTED:
				case TYPE_LOGIN_REJECTED:
				case TYPE_HEARTBEAT:
					m_isDataMessage = false;
					m_type = Type(*m_messageBegin);
					break;
				case 'S':
					if (m_logicalLen < m_timestampFieldSize + 2) {
						throw FieldHasInvalidLenError(m_messageBegin, m_messageEnd);
					}
					--m_logicalLen;
					std::advance(m_messageLogicalBegin, 1);
					m_isDataMessage = true;
					switch (*(m_messageLogicalBegin + m_timestampFieldSize)) {
						case DATA_TYPE_ORDER_ACCEPTED:
							m_type = TYPE_ORDER_ACCEPTED;
							break;
						case DATA_TYPE_ORDER_REJECTED:
							m_type = TYPE_ORDER_REJECTED;
							break;
						case DATA_TYPE_ORDER_CANCELED:
							m_type = TYPE_ORDER_CANCELED;
							break;
						case DATA_TYPE_ORDER_EXECUTED:
							m_type = TYPE_ORDER_EXECUTED;
							break;
						default:
							throw MessageNotGatawayMessageError(m_messageBegin, m_messageEnd);
					}
					break;
				default:
					throw MessageNotGatawayMessageError(m_messageBegin, m_messageEnd);
			}
			switch (m_type) {
				case TYPE_LOGIN_ACCEPTED:
					CheckMessageLen(21);
					break;
				case TYPE_LOGIN_REJECTED:
					CheckMessageLen(2);
					break;
				case TYPE_HEARTBEAT:
					CheckMessageLen(1);
					break;
				case TYPE_DEBUG:
					break;
				case TYPE_ORDER_ACCEPTED:
					CheckMessageLen(106 - 10);
					break;
				case TYPE_ORDER_REJECTED:
					CheckMessageLen(26);
					break;
				case TYPE_ORDER_CANCELED:
					CheckMessageLen(32);
					break;
				case TYPE_ORDER_EXECUTED:
					CheckMessageLen(70);
					break;
			};
		}

	public:

		Iterator GetBegin() const {
			return m_messageBegin;
		}

		Iterator GetEnd() const {
			return m_messageEnd;
		}

		Type GetType() const {
			return m_type;
		}
	
		Len GetMessageLogicalLen() const {
			return m_logicalLen;
		}

	public:

		std::string GetAsString(bool contentOnly) const {
			const auto begin = contentOnly
				?	!m_isDataMessage
					?	m_messageLogicalBegin + 1
					:	m_messageLogicalBegin + m_timestampFieldSize + 1
				:	m_messageBegin;
			std::string result(begin, m_messageEnd);
			boost::trim(result);
			return result;
		}

		std::string GetFieldAsString(FieldStart offset, Len len) const {
			std::string result;
			GetRawField(offset, len, result);
			return result;
		}

		Char GetCharField(FieldStart offset) const {
			if (m_logicalLen < offset + 1) {
				throw FieldHasInvalidFormatError(m_messageBegin, m_messageEnd);
			}
			return *(m_messageLogicalBegin + offset);
		}

		std::string GetAlphanumField(FieldStart offset, Len len) const {
			std::string result;
			GetTrimedField(offset, len, result, false, true, " ");
			return result;
		}

		Numeric GetNumericField(FieldStart offset, Len len) const {
			std::string strVal;
			GetTrimedField(offset, len, strVal, true, true, " ");
			return Cast<Numeric>(strVal);
		}

		Price GetPriceField(FieldStart offset, Len len) const {
			std::string strVal;
			GetRawField(offset, len, strVal);
			const auto dotPos = strVal.find('.');
			if (dotPos != std::string::npos) {
				return Cast<Price>(strVal);
			}
			AssertGe(strVal.size(), 4);
			Price result = strVal.size() > 4
				?	Cast<Price>(strVal.substr(0, strVal.size() - 4))
				:	.0;
			result += Cast<Price>(strVal.substr(strVal.size() - 4)) / 10000;
			return result;
		}

	public:

		const char * GetLoginRejectReasonField(FieldStart offset) const {
			switch (GetCharField(offset)) {
				case 'A':
					return
						"Not Authorized. There was an invalid username"
							" and password combination in the Login Request Message.";
				case 'S':
					return
						"Session not available. The Requested Session in the Login"
							" Request Packet was either invalid or not available.";
				default:
					throw FieldHasInvalidFormatError(m_messageBegin, m_messageEnd);
			}
		}

		const char * GetOrderRejectReasonField(FieldStart offset) const {
			switch (GetCharField(offset)) {
				case 'A':
					return "Odd lot to venue";
				case 'C':
					return "Destination for order is closed or currently down";
				case 'E':
					return "Max order size rule";
				case 'F':
					return "Max position size rule";
				case 'G':
					return "Rule update in progress";
				case 'I':
					return "Price not available";
				case 'J':
					return "Short order with long position";
				case 'K':
					return "Sell order without long position";
				case 'L':
					return "Potential oversell";
				case 'M':
					return "Sell shares more than long";
				case 'N':
					return "Nonshortable";
				case 'P':
					return "Insufficient day-trading buying power";
				case 'R':
					return "Protection price";
				case 'U':
					return "Marked PnL cutoff rule";
				case 'V':
					return "Over selling";
				case 'W':
					return "Not well formed, one or more fields are not valid";
				case 'Y':
					return "Invalid account number";
				case 'Z':
					return "Max order size";
				case '3':
					return "ARCA odd lots rule";
				case '4':
					return "Wash Sale Rule";
				case '5':
					return "Clearly erroneous risk check";
				case '6':
					return "Max BP per stock rule";
				case 'O':
					return "Other error";
				default:
					throw FieldHasInvalidFormatError(m_messageBegin, m_messageEnd);
			}
		}

		const char * GetOrderCancelReasonField(FieldStart offset) const {
			switch (GetCharField(offset)) {
				case 'U':
					return "User requested cancel";
				case 'T':
					return "Order was timed out or otherwise no longer valid";
				case 'V':
					return "Order became no longer valid for some other reason";
				case 'M':
					return "Order was manually canceled outside of the system";
				case 'O':
					return "Other reason";
				default:
					throw FieldHasInvalidFormatError(m_messageBegin, m_messageEnd);
			}
		}

	private:

		template<typename Target>
		inline Target Cast(const std::string &arg) const {
			try {
				return boost::lexical_cast<Target>(arg);
			} catch (const boost::bad_lexical_cast &) {
				throw FieldHasInvalidFormatError(m_messageBegin, m_messageEnd);
			}
		}

	private:

		void GetRawField(FieldStart offset, Len len, std::string &result) const {
			if (m_logicalLen < offset + len) {
				throw FieldHasInvalidFormatError(m_messageBegin, m_messageEnd);
			}
			const auto fieldBegin = m_messageLogicalBegin + offset;
			const auto fieldEnd = fieldBegin + len;
			std::string resultTmp(fieldBegin, fieldEnd);
			resultTmp.swap(result);
		}

		void GetTrimedField(
					FieldStart offset,
					Len len,
					std::string &result,
					bool trimLeft,
					bool trimRight,
					const char *predStr)
				const {
			Assert(trimLeft || trimRight);
			std::string resultTmp;
			GetRawField(offset, len, resultTmp);
			const auto pred = boost::is_any_of(predStr);
			if (trimLeft) {
				boost::trim_left_if(resultTmp, pred);
			}
			if (trimRight) {
				boost::trim_right_if(resultTmp, pred);
			}
			resultTmp.swap(result);
		}

		void CheckMessageLen(Len expectedLen) const {
			if (m_logicalLen != expectedLen) {
				throw FieldHasInvalidLenError(m_messageBegin, m_messageEnd);
			}
		}

	private:

		Iterator m_messageBegin;
		Iterator m_messageLogicalBegin;
		Iterator m_messageEnd;
		Len m_logicalLen;
		bool m_isDataMessage;
	
		Type m_type;

		static const Len m_timestampFieldSize;

	};

	template<typename BufferT>
	const typename GatewayTsMessage<BufferT>::Len GatewayTsMessage<BufferT>::m_timestampFieldSize = 8;

} } }
