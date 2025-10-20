#include "Database.h"
#include <spdlog/spdlog.h>

std::unordered_map<FieldKey, const google::protobuf::Descriptor*> Database::protoTypes;
std::unordered_map<FieldKey, const char*> Database::classNames;

Database::Database(fs::path dbPath) : m_filename(dbPath), m_dbRaw(dbPath.string(), sql::OPEN_READWRITE | sql::OPEN_CREATE) {
	OnDbLoad();
}

sql::Database* Database::GetRaw() {
	return &m_dbRaw;
}

std::shared_ptr<pbuf::Message> Database::CreateObjectOfFieldType(FieldKey key) {
	const pbuf::Descriptor* type = protoTypes.at(key);
	if (type == nullptr) {
		spdlog::error("Called CreateObjectOfFieldType with an invalid FieldKey");
		throw;
	}
	pbuf::DynamicMessageFactory factory;
	return std::shared_ptr<pbuf::Message>(
		pbuf::DynamicMessageFactory().GetPrototype(type)->New()
	);
}

std::vector<std::shared_ptr<pbuf::Message>> Database::GetFields(sql::Statement& query, FieldKey key) {
	query.bind(1, GetTableName());
	query.bind(2, GetFieldName(key));
	std::vector<std::shared_ptr<pbuf::Message>> output;
	while (query.executeStep()) {
		if (query.getColumnCount() != 1) {
			spdlog::warn("Multiple columns returned by query passed into GetField w FieldKey {}, ignoring all columns except first one", classNames.at(key));
		}
		const void* blob = query.getColumn(0).getBlob();
		int sz = query.getColumn(0).getBytes();
		if (sz < 4) {
			spdlog::error("sz < 4 in GetField, cell cannot contain a FieldKey");
			throw;
		}
		FieldKey fieldKey;
		memcpy(&fieldKey, blob, sizeof(FieldKey));
		std::shared_ptr<pbuf::Message> object = CreateObjectOfFieldType(key);
		object->ParseFromArray(reinterpret_cast<const char*>(blob) + 4, sz - 4);
		output.push_back(object);
	}
	return output;
}

// Assumes that the 1st and 2nd bindings will be the table name and the key name respectively
std::shared_ptr<pbuf::Message> Database::GetField(sql::Statement& query, FieldKey key) {
	query.bind(1, GetTableName());
	query.bind(2, GetFieldName(key));
	if (!query.executeStep()) {
		spdlog::error("Failed to get field using query passed to GetField");
		throw;
	}
	if (query.getColumnCount() != 1) {
		spdlog::warn("more than 1 column using query passed to GetField, ignoring other columns");
	}
	const void* blob = query.getColumn(0).getBlob();
	int sz = query.getColumn(0).getBytes();
	if (sz < 4) {
		spdlog::error("sz < 4 in GetField, cell cannot contain a FieldKey");
		throw;
	}
	FieldKey fieldKey;
	memcpy(&fieldKey, blob, sizeof(FieldKey));
	std::shared_ptr<pbuf::Message> object = CreateObjectOfFieldType(key);
	object->ParseFromArray(reinterpret_cast<const char*>(blob) + 4, sz - 4);
	return object;
}

sql::Statement Database::GetStatement(std::string command) {
	return sql::Statement(m_dbRaw, command);
}

void Database::AddPrototype(FieldKey key, const char* className) {
	classNames.insert({ key, className });
	protoTypes.insert({ key, pbuf::DescriptorPool::generated_pool()->FindMessageTypeByName(className) });
}

/** Makes the following assumptions:
* - statement is valid SQL
* - statement contains the following 3 unfilled vars: table name, key, and the value.
Eg valid: INSERT INTO ? (?) VALUES (?) WHERE HI=? B=2
invalid: INSERT INTO hi (?) VALUES (?) WHERE HI=? B=2
*/
void Database::SetField(sql::Statement& statement, FieldKey key, const pbuf::Message* object) {
	try {
		std::vector<uint8_t> bytes(object->ByteSizeLong() + sizeof(FieldKey));
		object->SerializeToArray(bytes.data() + sizeof(FieldKey), object->ByteSizeLong());
		memcpy(bytes.data(), &key, sizeof(FieldKey));
		sql::Statement command(m_dbRaw, "INSERT INTO ? (?) VALUES (?)");
		command.bind(1, GetTableName());
		command.bind(2, GetFieldName(key));
		command.bind(3, bytes.data(), bytes.size());
		command.exec();
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