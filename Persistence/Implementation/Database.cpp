#include "Database.h"
#include <spdlog/spdlog.h>

std::unordered_map<FieldKey, const std::string> Database::classNames;
std::unordered_map <FieldKey, std::unique_ptr<const pbuf::Message>> Database::defaultFieldValues;

Database::Database(fs::path dbPath, const std::string tableName, const std::string keyFieldName, const std::string keyFieldType) 
	: m_filename(dbPath), m_dbRaw(dbPath.string(), sql::OPEN_READWRITE | sql::OPEN_CREATE),
  m_tableName(tableName), m_keyFieldName(keyFieldName), m_keyFieldType(m_keyFieldType)
{
	m_dbRaw.exec("CREATE TABLE IF NOT EXISTS " + GetTableName() + " (" + GetKeyFieldName() + " " + GetKeyFieldType() + " PRIMARY KEY);");
}

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
* - statement has 1 questionmark, and it's index(1-indexed) is passed for dataBindIndex
Eg valid: INSERT INTO players (PlayerName) VALUES (?) WHERE HI=? B=2
*/
void Database::SetField(sql::Statement& statement, FieldKey key, const pbuf::Message* object, uint32_t dataBindIndex) {
	if (dataBindIndex == 0) {
		spdlog::error("passed 0 for data bind index which is not valid");
		throw;
	}
	try {
		std::vector<uint8_t> bytes(object->ByteSizeLong() + sizeof(FieldKey));
		object->SerializeToArray(bytes.data() + sizeof(FieldKey), object->ByteSizeLong());
		memcpy(bytes.data(), &key, sizeof(FieldKey));
		statement.bind(dataBindIndex, bytes.data(), bytes.size());
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

const std::string& Database::GetTableName() {
	return m_tableName;
}

const std::string& Database::GetKeyFieldName() {
	return m_keyFieldName;
}

const std::string& Database::GetKeyFieldType() {
	return m_keyFieldType;
}