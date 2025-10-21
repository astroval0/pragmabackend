#include "Database.h"
#include <spdlog/spdlog.h>

std::unordered_map<FieldKey, const std::string> Database::classNames;

Database::Database(fs::path dbPath) : m_filename(dbPath), m_dbRaw(dbPath.string(), sql::OPEN_READWRITE | sql::OPEN_CREATE) {}

sql::Database* Database::GetRaw() {
	return &m_dbRaw;
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

const std::string& Database::GetFieldName(FieldKey key) {
	return classNames.at(key);
}