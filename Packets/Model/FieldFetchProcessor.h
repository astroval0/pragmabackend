#pragma once
#include <PacketProcessor.h>
#include <google/protobuf/util/json_util.h>
#include <type_traits>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>
#include <FieldKey.h>
#include <Database.h>

namespace pbu = google::protobuf::util;

template<typename T>
class FieldFetchProcessor : public WebsocketPacketProcessor {
private:
	T m_defaultInventory;
	pbu::JsonPrintOptions m_printOpts;
	pbu::JsonParseOptions m_parseOpts;
	Database& m_dbRef;
	const FieldKey field;
public:
	FieldFetchProcessor(SpectreRpcType rpcType, std::string defaultFieldPath, FieldKey key, Database& dbRef) :
		WebsocketPacketProcessor(rpcType), m_dbRef(dbRef), field(key) {
		std::ifstream defaultFile(defaultFieldPath);
		std::stringstream buf;
		buf << defaultFile.rdbuf();
		std::string data = buf.str();
		if (!pbu::JsonStringToMessage(data, &m_defaultInventory, m_parseOpts).ok()) {
			spdlog::error("failed to parse default message");
			throw;
		}
	}
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override {
		sql::Statement query = m_dbRef.FormatStatement("SELECT {col} FROM {table} WHERE PlayerId=? LIMIT 1", field);
		query.bind(1, sock.GetPlayerId());
		m_dbRef.GetField<T>(query, field);
	}
	static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to FieldFetchProcessor must inherit from protobuf::Message");
};