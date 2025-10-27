#pragma once
#include <PacketProcessor.h>
#include <google/protobuf/util/json_util.h>
#include <type_traits>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <FieldKey.h>
#include <PlayerDatabase.h>

namespace pbu = google::protobuf::util;

template<typename T>
class FieldFetchProcessor : public WebsocketPacketProcessor {
private:
	PlayerDatabase& m_dbRef;
	const FieldKey field;
public:
	FieldFetchProcessor(SpectreRpcType rpcType, FieldKey key, PlayerDatabase& dbRef) :
		WebsocketPacketProcessor(rpcType), m_dbRef(dbRef), field(key) {
	}

	FieldFetchProcessor(SpectreRpcType rpcType, FieldKey key) :
		FieldFetchProcessor(rpcType, key, PlayerDatabase::Get()) {
	}

	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override {
		sql::Statement query = m_dbRef.FormatStatement("SELECT {col} FROM {table} WHERE PlayerId=? LIMIT 1", field);
		query.bind(1, sock.GetPlayerId());
		std::unique_ptr<T> data = m_dbRef.GetField<T>(query, field);
		if (data == nullptr) {
			spdlog::error("No field value to return");
			throw;
		}
		sock.SendPacket(*data, packet.GetResponseType(), packet.GetRequestId());
	}
	static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to FieldFetchProcessor must inherit from protobuf::Message");
};