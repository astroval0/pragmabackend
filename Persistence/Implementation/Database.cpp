#include "Database.h"
#include <spdlog/spdlog.h>

std::unordered_map<FieldKey, const google::protobuf::Descriptor*> Database::protoTypes;
std::unordered_map<FieldKey, const char*> Database::classNames;
pbuf::DynamicMessageFactory messageFactory;

Database::Database(fs::path dbPath) : m_filename(dbPath), m_dbRaw(dbPath.string(), sql::OPEN_READWRITE | sql::OPEN_CREATE) {}

sql::Database* Database::GetRaw() {
	return &m_dbRaw;
}

std::unique_ptr<pbuf::Message> Database::CreateObjectOfFieldType(FieldKey key) {
	const pbuf::Descriptor* type = protoTypes.at(key);
	if (type == nullptr) {
		spdlog::error("Called CreateObjectOfFieldType with an invalid FieldKey");
		throw;
	}
	const pbuf::Message* prototype = messageFactory.GetPrototype(type);
	if (prototype == nullptr) {
		spdlog::error("null prototype");
		throw;
	}
	pbuf::Message* clone = prototype->New();
	if (clone == nullptr) {
		spdlog::error("null clone");
		throw;
	}
	return std::unique_ptr<pbuf::Message>(
		clone
	);
}

std::vector<std::unique_ptr<pbuf::Message>> Database::GetFields(sql::Statement& query, FieldKey key) {
	std::vector<std::unique_ptr<pbuf::Message>> output;
	while (query.executeStep()) {
		if (query.getColumnCount() != 1) {
			spdlog::warn("Multiple columns returned by query passed into GetField w FieldKey {}, ignoring all columns except first one", classNames.at(key));
		}
		const char* blob = static_cast<const char*>(query.getColumn(0).getBlob());
		int sz = query.getColumn(0).getBytes();
		if (sz < sizeof(FieldKey)) {
			spdlog::error("sz < {} in GetField, cell cannot contain a FieldKey", sizeof(FieldKey));
			throw;
		}
		FieldKey fieldKey;
		memcpy(&fieldKey, blob, sizeof(FieldKey));
		std::unique_ptr<pbuf::Message> object = CreateObjectOfFieldType(key);
		if (!object->ParseFromArray(blob + sizeof(FieldKey), sz - sizeof(FieldKey))) {
			spdlog::error("parse failure");
			throw;
		}
		output.push_back(std::move(object));
	}
	return output;
}

// Assumes that the 1st and 2nd bindings will be the table name and the key name respectively
std::unique_ptr<pbuf::Message> Database::GetField(sql::Statement& query, FieldKey key) {
	if (!query.executeStep()) {
		spdlog::error("Failed to get field using query passed to GetField");
		throw;
	}
	if (query.getColumnCount() != 1) {
		spdlog::warn("more than 1 column using query passed to GetField, ignoring other columns");
	}
	const char* blob = static_cast<const char*>(query.getColumn(0).getBlob());
	int sz = query.getColumn(0).getBytes();
	if (sz < sizeof(FieldKey)) {
		spdlog::error("sz < 4 in GetField, cell cannot contain a FieldKey");
		throw;
	}
	FieldKey fieldKey;
	memcpy(&fieldKey, blob, sizeof(FieldKey));
	std::unique_ptr<pbuf::Message> object = CreateObjectOfFieldType(key);
	if (!object->ParseFromArray(blob + sizeof(FieldKey), sz - sizeof(FieldKey))) {
		spdlog::error("Parsing error in GetField");
		throw;
	}
	return std::move(object);
}

sql::Statement Database::FormatStatement(std::string command, FieldKey key) {
	size_t tablePos = command.find("{table}");
	if (tablePos == std::string::npos) {
		spdlog::error("did not find {{table}} to replace while processing sql command {} in FormatStatement", command);
		throw;
	}
	command.replace(tablePos, sizeof("{table}") - 1, GetTableName());
	size_t colPos = command.find("{col}");
	if (colPos == std::string::npos) {
		spdlog::error("did not find {{col}} to replace while processing sql command {} in FormatStatement", command);
		throw;
	}
	command.replace(colPos, sizeof("{col}") - 1, GetFieldName(key));
	return sql::Statement(m_dbRaw, command);
}

void Database::AddPrototype(FieldKey key, const char* className) {
	classNames.insert({ key, className });
	protoTypes.insert({ key, pbuf::DescriptorPool::generated_pool()->FindMessageTypeByName(className) });
}

/** Makes the following assumptions:
* - statement is valid SQL
* - statement's first unfilled value is the value
Eg valid: INSERT INTO players (PlayerName) VALUES (?) WHERE HI=? B=2
*/
void Database::SetField(sql::Statement& statement, FieldKey key, const pbuf::Message* object) {
	try {
		std::vector<uint8_t> bytes(object->ByteSizeLong() + sizeof(FieldKey));
		object->SerializeToArray(bytes.data() + sizeof(FieldKey), object->ByteSizeLong());
		memcpy(bytes.data(), &key, sizeof(FieldKey));
		statement.bind(1, bytes.data(), bytes.size());
		statement.exec();
	}
	catch (const std::exception& e) {
		spdlog::error("Failed to set field with FieldKey {}", classNames.at(key));
		throw;
	}
}

bool IsFieldPopulated(sql::Statement& command) {
	return command.executeStep();
}

const char* Database::GetFieldName(FieldKey key) {
	const char* name = classNames.at(key);
	if (name == nullptr) {
		spdlog::error("Tried to get name of FieldKey {}, but it was not registered", static_cast<uint32_t>(key));
		throw;
	}
	return name;
}