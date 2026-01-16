#pragma once
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <SQLiteCpp/SQLiteCpp.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "FieldKey.h"
#include <type_traits>
#include <spdlog/spdlog.h>
#include <optional>
#include <fstream>
#include <iostream>
#include <google/protobuf/util/json_util.h>
#include <sstream>

namespace fs = std::filesystem;
namespace pbuf = google::protobuf;
namespace sql = SQLite;

// Template implementations have to be in the headers to not get linker errors, :C it's a bit ugly
/* Sample code:
	PlayerDatabase playerData(fs::path("playerdata.sqlite"));
	playerData.AddPrototype<PlayerName>(FieldKey::PLAYER_INGAME_NAME);
	PlayerName name;
	name.set_name("myname");
	sql::Statement statement = playerData.FormatStatement(
		"INSERT INTO {table} ({col}) VALUES(?)", FieldKey::PLAYER_INGAME_NAME
	);
	playerData.SetField(statement, FieldKey::PLAYER_INGAME_NAME, &name);
	sql::Statement fetchStatement = playerData.FormatStatement(
		"SELECT {col} FROM {table} LIMIT 1", FieldKey::PLAYER_INGAME_NAME
	);
	std::unique_ptr<PlayerName> nameFetched = playerData.GetField<PlayerName>(fetchStatement, FieldKey::PLAYER_INGAME_NAME);
	spdlog::info("name: {}", nameFetched->name());
*/
class Database {
private:
	fs::path m_filename;
	sql::Database m_dbRaw;
	std::string m_tableName;
	std::string m_keyFieldName;
	std::string m_keyFieldType;
	pbuf::util::JsonParseOptions m_parseOpts;
	static std::unordered_map<FieldKey, const std::string> classNames;
	static std::unordered_map<FieldKey, std::unique_ptr<const pbuf::Message>> defaultFieldValues;
	template<typename T>
	std::unique_ptr<T> DefaultOrNullptr(FieldKey key) {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to DefaultOrNullptr must inherit from protobuf::Message");
		auto defaultValue = defaultFieldValues.find(key);
		if (defaultValue == defaultFieldValues.end()) {
			return nullptr;
		}
		spdlog::info("Returning default value for FieldKey: {}", (uint32_t)key);
		const T* typed = dynamic_cast<const T*>(defaultValue->second.get());
		if (!typed) {
			// Handle type mismatch
			spdlog::error("type mismatch in DefaultOrNullptr");
			return nullptr;
		}

		auto copy = std::make_unique<T>();
		copy->CopyFrom(*typed);
		return std::move(copy);
	}
public:
	Database(fs::path dbPath, const std::string tableName, const std::string keyFieldName, const std::string keyFieldType);

	sql::Database* GetRaw();
	sql::Database& GetRawRef();

	template<typename T>
	std::vector<std::unique_ptr<T>> GetFields(sql::Statement& query, FieldKey key) {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to GetFields must inherit from protobuf::Message");
		std::vector<std::unique_ptr<T>> output;
		while (query.executeStep()) {
			if (query.getColumnCount() != 1) {
				spdlog::warn("Multiple columns returned by query passed into GetFields w FieldKey {}, ignoring all columns except first one", classNames.at(key));
			}
			const char* blob = static_cast<const char*>(query.getColumn(0).getBlob());
			int sz = query.getColumn(0).getBytes();
			if (sz < sizeof(FieldKey)) {
				spdlog::error("sz < {} in GetFields, cell cannot contain a FieldKey", sizeof(FieldKey));
				throw;
			}
			FieldKey savedFieldKey;
			memcpy(&savedFieldKey, blob, sizeof(FieldKey));
			if (savedFieldKey != key) {
				spdlog::error("FieldKey passed to GetFields {} was not the same as FieldKey found in saved object", (uint32_t)key);
				throw;
			}
			std::unique_ptr<T> object = std::make_unique<T>();
			if (!object->ParseFromArray(blob + sizeof(FieldKey), sz - sizeof(FieldKey))) {
				spdlog::error("parse failure");
				throw;
			}
			output.push_back(std::move(object));
		}
		return output;
	}

	template<typename T>
	std::unique_ptr<T> GetField(sql::Statement& query, FieldKey key) {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to GetField must inherit from protobuf::Message");
		if (!query.executeStep()) {
			return std::move(DefaultOrNullptr<T>(key));
		}
		if (query.getColumnCount() != 1) {
			spdlog::warn("Multiple columns returned by query passed into GetField w FieldKey {}, ignoring all columns except first one", classNames.at(key));
		}
		const char* blob = static_cast<const char*>(query.getColumn(0).getBlob());
		int sz = query.getColumn(0).getBytes();
		if (sz <= 0) {
			return std::move(DefaultOrNullptr<T>(key));
		}
		if (sz < sizeof(FieldKey)) {
			spdlog::error("sz < {} but > 0 in GetField, cell cannot contain a FieldKey", sizeof(FieldKey));
			return nullptr;
		}
		FieldKey savedFieldKey;
		memcpy(&savedFieldKey, blob, sizeof(FieldKey));
		if (savedFieldKey != key) {
			spdlog::error("FieldKey passed to GetFields {} was not the same as FieldKey found in saved object", (uint32_t)key);
			return std::move(DefaultOrNullptr<T>(key));
		}
		std::unique_ptr<T> object = std::make_unique<T>();
		if (!object->ParseFromArray(blob + sizeof(FieldKey), sz - sizeof(FieldKey))) {
			spdlog::error("parse failure");
			return std::move(DefaultOrNullptr<T>(key));
		}
		return std::move(object);
	}

	template<typename T>
	std::unique_ptr<T> GetField(FieldKey key, const std::string& dbKey) {
		sql::Statement query = FormatStatement(
			"SELECT {col} FROM {table} WHERE " + GetKeyFieldName() + " = ? COLLATE NOCASE", 
			key
		);
		query.bind(1, dbKey);
		return std::move(GetField<T>(query, key));
	}

	sql::Statement FormatStatement(std::string command, FieldKey key);

	template<typename T>
	void AddPrototype(FieldKey key) {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to AddPrototype must inherit from protobuf::Message");
		classNames.insert({ key, T::descriptor()->name() });
		sql::Statement colQuery(m_dbRaw, "PRAGMA table_info(" + GetTableName() + ");");
		bool colExists = false;
		while (colQuery.executeStep()) {
			std::string colName = colQuery.getColumn(1).getText();
			if (colName == T::descriptor()->name()) {
				colExists = true;
				break;
			}
		}
		if (!colExists) {
			m_dbRaw.exec("ALTER TABLE " + GetTableName() + " ADD COLUMN " + T::descriptor()->name() + " BLOB;");
		}
	}

	template<typename T>
	void AddPrototype(FieldKey key, std::string defaultFieldValuePath) {
		AddPrototype<T>(key);
		std::ifstream defaultFile(defaultFieldValuePath);
	    if (!defaultFile.is_open())
	    {
	        spdlog::error("failed to open default message at path: {}", defaultFieldValuePath);
	        throw std::runtime_error("Failed to open default message given path");
	    }
		std::stringstream buf;
		buf << defaultFile.rdbuf();
		std::string data = buf.str();
		T defaultFieldData;
		auto status = pbuf::util::JsonStringToMessage(data, &defaultFieldData, m_parseOpts);
		if (!status.ok()) {
			spdlog::error("failed to parse default message: {}", status.message());
			throw;
		}
		defaultFieldValues[key] = std::make_unique<const T>(defaultFieldData);
	}

	bool IsFieldPopulated(FieldKey key);

	void SetField(sql::Statement& statement, FieldKey key, const pbuf::Message* object, uint32_t dataBindIndex);
	void SetField(FieldKey key, const pbuf::Message* object, const std::string& ddbKey);

	const std::string& GetFieldName(FieldKey key);

	const std::string& GetTableName();
	const std::string& GetKeyFieldName();
	const std::string& GetKeyFieldType();
};