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

namespace msgpack { MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) { namespace adaptor {

	template<>
	template<bool isAscendingSort>
	struct pack<trdk::PriceBook::Side<isAscendingSort>> {
		template<typename Stream>
		msgpack::packer<Stream> & operator ()(
				msgpack::packer<Stream> &o,
				const trdk::PriceBook::Side<isAscendingSort> &v)
				const {
			const auto size = std::min<size_t>(5, v.GetSize());
			o.pack_array(size);
			for (size_t i = 0; i < size; ++i) {
				const auto &level = v.GetLevel(i);
				o.pack_array(2);
				o.pack(level.GetPrice());
				o.pack(level.GetQty().Get());
			}
			return o;
		}
	};

	template<>
	struct pack<trdk::EngineServer::DropCopyRecord> {

		template<typename Stream>
		packer<Stream> & operator ()(
				msgpack::packer<Stream> &stream,
				const trdk::EngineServer::DropCopyRecord &record)
				const {

			stream.pack_map(record.size());

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
