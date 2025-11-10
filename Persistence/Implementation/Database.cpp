#include "Database.h"
#include <spdlog/spdlog.h>

std::unordered_map<FieldKey, const std::string> Database::classNames;
std::unordered_map <FieldKey, std::unique_ptr<const pbuf::Message>> Database::defaultFieldValues;

Database::Database(fs::path dbPath, const std::string tableName, const std::string keyFieldName, const std::string keyFieldType) 
	: m_filename(dbPath), m_dbRaw(dbPath.string(), sql::OPEN_READWRITE | sql::OPEN_CREATE),
  m_tableName(tableName), m_keyFieldName(keyFieldName)
{
	m_dbRaw.exec("CREATE TABLE IF NOT EXISTS " + GetTableName() + " (" + GetKeyFieldName() + " " + GetKeyFieldType() + " PRIMARY KEY);");
}

sql::Database* Database::GetRaw() {
	return &m_dbRaw;
}

sql::Database& Database::GetRawRef() {
	return m_dbRaw;
}

sql::Statement Database::FormatStatement(std::string command, FieldKey key) {
	size_t tablePos = command.find("{table}");
	while (tablePos != std::string::npos) {
		command.replace(tablePos, sizeof("{table}") - 1, GetTableName());
		tablePos = command.find("{table}");
	}
	size_t colPos = command.find("{col}");
	while (colPos != std::string::npos) {
		command.replace(colPos, sizeof("{col}") - 1, GetFieldName(key));
		colPos = command.find("{col}");
	}
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

void Database::SetField(FieldKey key, const pbuf::Message* object, const std::string& dbKeyId) {
	sql::Statement setStatement = FormatStatement(
		"INSERT INTO {table} (" + GetKeyFieldName() + ", {col}) VALUES(?,?) ON CONFLICT(" + GetKeyFieldName() + ") DO UPDATE SET {col} = excluded.{col};",
		key
	);
	setStatement.bind(1, dbKeyId);
	return SetField(setStatement, key, object, 2);
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