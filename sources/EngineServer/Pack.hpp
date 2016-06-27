/*******************************************************************************
 *   Created: 2016/01/12 13:46:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "DropCopyRecord.hpp"
#include "Core/PriceBook.hpp"

namespace trdk { namespace EngineServer { namespace Details {

	typedef uint32_t PackSizeT;

	template<typename Stream, bool isAscendingSort>
	msgpack::packer<Stream> & Pack(
			msgpack::packer<Stream> &o,
			const trdk::PriceBook::Side<isAscendingSort> &v) {
		const auto size = std::min<size_t>(5, v.GetSize());
		o.pack_array(PackSizeT(size));
		for (size_t i = 0; i < size; ++i) {
			const auto &level = v.GetLevel(i);
			o.pack_array(2);
			o.pack(level.GetPrice());
			o.pack(level.GetQty().Get());
		}
		return o;
	}

} } }

namespace msgpack { namespace v1 { namespace adaptor {

	template<>
	struct pack<trdk::PriceBook::Bid> {
		template<typename Stream>
		msgpack::packer<Stream> & operator ()(
				msgpack::packer<Stream> &o,
				const trdk::PriceBook::Bid &v)
				const {
			return trdk::EngineServer::Details::Pack(o, v);
		}
	};
	template<>
	struct pack<trdk::PriceBook::Ask> {
		template<typename Stream>
		msgpack::packer<Stream> & operator ()(
				msgpack::packer<Stream> &o,
				const trdk::PriceBook::Ask &v)
				const {
			return trdk::EngineServer::Details::Pack(o, v);
		}
	};

	template<>
	struct pack<trdk::EngineServer::DropCopyRecord> {

		template<typename Stream>
		packer<Stream> & operator ()(
				msgpack::packer<Stream> &stream,
				const trdk::EngineServer::DropCopyRecord &record)
				const {

			stream.pack_map(
				trdk::EngineServer::Details::PackSizeT(record.size()));

			foreach (const auto &i, record) {

				stream.pack(i.first);

				using namespace trdk::EngineServer::Details;
				typedef DropCopyRecordFieldMsgpackVisitor<decltype(stream)>
					Visitor;
				Visitor visitor(stream);
				boost::apply_visitor(visitor, i.second);

			}

			return stream;

		}

	};

} } }
