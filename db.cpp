#include <PlayerDatabase.h>
#include <filesystem>
#include <PlayerName.pb.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

void SetupDatabase() {
	PlayerDatabase playerData(fs::path("playerdata.sqlite"));
	playerData.AddPrototype(FieldKey::PLAYER_INGAME_NAME, "PlayerName");
	std::unique_ptr<pbuf::Message> newField = playerData.CreateObjectOfFieldType(FieldKey::PLAYER_INGAME_NAME);
	std::unique_ptr<PlayerName> newName(static_cast<PlayerName*>(newField.release()));
	spdlog::info("nameinit: {}", newName->name()); // this throws, something is wrong with CreateObjectOfFieldType function
	PlayerName name;
	name.set_name("myname");
	sql::Statement statement = playerData.FormatStatement(
		"INSERT INTO {table} ({col}) VALUES(?)", FieldKey::PLAYER_INGAME_NAME
	);
	playerData.SetField(statement, FieldKey::PLAYER_INGAME_NAME, &name);
	sql::Statement fetchStatement = playerData.FormatStatement(
		"SELECT {col} FROM {table} LIMIT 1", FieldKey::PLAYER_INGAME_NAME
	);
	std::unique_ptr<pbuf::Message> nameFetched = playerData.GetField(fetchStatement, FieldKey::PLAYER_INGAME_NAME);
	std::unique_ptr<PlayerName> converted(static_cast<PlayerName*>(nameFetched.release()));
	spdlog::info("name: {}", converted->name());
}