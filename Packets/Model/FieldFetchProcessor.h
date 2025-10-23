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
	T m_defaultFieldData;
	pbu::JsonParseOptions m_parseOpts;
	PlayerDatabase& m_dbRef;
	const FieldKey field;
public:
	FieldFetchProcessor(SpectreRpcType rpcType, std::string defaultFieldPath, FieldKey key, PlayerDatabase& dbRef) :
		WebsocketPacketProcessor(rpcType), m_dbRef(dbRef), field(key) {
		std::ifstream defaultFile(defaultFieldPath);
		std::stringstream buf;
		buf << defaultFile.rdbuf();
		std::string data = buf.str();
		auto status = pbu::JsonStringToMessage(data, &m_defaultFieldData, m_parseOpts);
		if (!status.ok()) {
			spdlog::error("failed to parse default message: {}", status.message());
			throw;
		}
	}
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override {
		sql::Statement query = m_dbRef.FormatStatement("SELECT {col} FROM {table} WHERE PlayerId=? LIMIT 1", field);
		query.bind(1, sock.GetPlayerId());
		std::unique_ptr<T> data = m_dbRef.GetField<T>(query, field);
		if (data == nullptr) {
			sock.SendPacket(m_defaultFieldData);
			sql::Statement writeStatement = m_dbRef.FormatStatement(
				"INSERT OR REPLACE INTO {table} (PlayerId, {col}) VALUES (?, ?)",
				field
			);
			writeStatement.bind(1, sock.GetPlayerId());
			m_dbRef.SetField(writeStatement, field, &m_defaultFieldData, 2);
		}
		else {
			sock.SendPacket(*data);
		}
	}
	static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to FieldFetchProcessor must inherit from protobuf::Message");
};