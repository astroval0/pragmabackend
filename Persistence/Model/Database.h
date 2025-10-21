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
	static std::unordered_map<FieldKey, const std::string> classNames;
public:
	Database(fs::path dbPath);

	sql::Database* GetRaw();

	template<typename T>
	std::vector<std::unique_ptr<T>> GetFields(sql::Statement& statement, FieldKey key) {
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
	std::unique_ptr<T> GetField(sql::Statement& statement, FieldKey key) {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to GetField must inherit from protobuf::Message");
		if (!query.executeStep()) {
			return nullptr;
		}
		if (query.getColumnCount() != 1) {
			spdlog::warn("Multiple columns returned by query passed into GetField w FieldKey {}, ignoring all columns except first one", classNames.at(key));
		}
		const char* blob = static_cast<const char*>(query.getColumn(0).getBlob());
		int sz = query.getColumn(0).getBytes();
		if (sz < sizeof(FieldKey)) {
			spdlog::error("sz < {} in GetField, cell cannot contain a FieldKey", sizeof(FieldKey));
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
		return std::move(object);
	}

	sql::Statement FormatStatement(std::string command, FieldKey key);

	template<typename T>
	void AddPrototype(FieldKey key) {
		static_assert(std::is_base_of<pbuf::Message, T>::value, "Type provided to AddPrototype must inherit from protobuf::Message");
		classNames.insert({ key, T::descriptor()->name() });
	}

	bool IsFieldPopulated(FieldKey key);

	void SetField(sql::Statement& statement, FieldKey key, const pbuf::Message* object);

	const std::string& GetFieldName(FieldKey key);

	virtual const std::string GetTableName() = 0;
};