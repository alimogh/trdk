/**************************************************************************
 *   Created: 2012/6/12/ 21:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#pragma once

template<typename MessageT>
class IqMessageParser {

public:

	typedef MessageT Message;
	typedef typename Message::const_iterator Iterator;

	class Error : public Exception {
	public:
		explicit Error(const char *what)
				: Exception(what) {
			//...//
		}
	};
	class IsNotIqFeedMessageError : public Error {
	public:
		IsNotIqFeedMessageError()
				: Error("Message is not IQFeed message") {
			//...//
		}
	};
	class FieldDoesntExistError : public Error {
	public:
		FieldDoesntExistError()
				: Error("Field doesn't exist in IQFeed message") {
			//...//
		}
	};
	class FieldHasInvalidFormatError : public Error {
	public:
		FieldHasInvalidFormatError()
				: Error("IQFeed message field has invalid format") {
			//...//
		}
	};
	class RequiredFieldIsEmptyError : public Error {
	public:
		RequiredFieldIsEmptyError()
			: Error("IQFeed message field required, but is empty") {
				//...//
		}
	};

private:

	typedef boost::algorithm::split_iterator<Iterator> SplitIterator;

public:

	IqMessageParser(Iterator messageBegin, Iterator messageEnd)
			: m_messageBegin(messageBegin),
			m_messageEnd(messageEnd) {
		Reset();
	}

public:

	void Reset() {
		SplitIterator split(
			m_messageBegin,
			m_messageEnd,
			boost::first_finder(",", boost::is_equal()));
		if (split == SplitIterator()) {
			throw IsNotIqFeedMessageError();
		}
		m_split = split;
		m_lastFieldNum = 1;
	}

public:

	Iterator GetBegin() const {
		return m_messageBegin;
	}

	Iterator GetEnd() const {
		return m_messageEnd;
	}

public:

	bool GetFieldAsBoolean(size_t fieldNum, bool isRequired) const {
		std::string strVal;
		GetStringField(fieldNum, isRequired, strVal);
		if (strVal == "T") {
			return true;
		} else if (strVal == "F") {
			return false;
		} else {
			throw FieldHasInvalidFormatError();
		}
	}

	double GetFieldAsDouble(size_t fieldNum, bool isRequired) const {
		double result = .0;
		GetTypedField(fieldNum, isRequired, result);
		return result;
	}

	int GetFieldAsInt(size_t fieldNum, bool isRequired) const {
		int result = 0;
		GetTypedField(fieldNum, isRequired, result);
		return result;
	}

	unsigned int GetFieldAsUnsignedInt(size_t fieldNum, bool isRequired) const {
		unsigned int result = 0;
		GetTypedField(fieldNum, isRequired, result);
		return result;
	}

	std::string GetFieldAsString(size_t fieldNum, bool isRequired) const {
		std::string result;
		GetStringField(fieldNum, isRequired, result);
		return result;
	}

	boost::posix_time::ptime GetFieldAsTime(size_t fieldNum, bool isRequired) const {
		return boost::posix_time::time_from_string(
			GetFieldAsString(fieldNum, isRequired));
	}

private:

	template<typename T>
	void GetTypedField(size_t fieldNum, bool isRequired, T &resultAndDefVal) const {
		std::string strVal;
		GetStringField(fieldNum, isRequired, strVal);
		if (strVal.empty()) {
			Assert(!isRequired);
			return;
		}
		try {
			resultAndDefVal = boost::lexical_cast<T>(strVal);
		} catch (const boost::bad_lexical_cast &ex) {
			Log::Error(
				"IQFeed message field has invalid format: \"%1%\".",
				ex.what());
			throw FieldHasInvalidFormatError();
		}
	}

	void GetStringField(
				size_t fieldNum,
				bool isRequired,
				std::string &resultAndDefVal)
			const {
		if (m_lastFieldNum > fieldNum) {
			AssertFail("Non-optimal use of IqMessageParser");
			const_cast<IqMessageParser *>(this)->Reset();
		}
		SplitIterator split = m_split;
		for (size_t i = m_lastFieldNum; i < fieldNum; ++i) {
			if (++split == SplitIterator()) {
				throw FieldDoesntExistError();
			}
		}
		std::string strVal = boost::copy_range<std::string>(*split);
		m_split = split;
		m_lastFieldNum = fieldNum;
		if (strVal.empty()) {
			if (isRequired) {
				throw RequiredFieldIsEmptyError();
			}
			return;
		}
		strVal.swap(resultAndDefVal);
	}

private:

	Iterator m_messageBegin;
	Iterator m_messageEnd;
	
	mutable size_t m_lastFieldNum;
	mutable SplitIterator m_split;

};
