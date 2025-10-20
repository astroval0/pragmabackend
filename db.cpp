#include <PlayerDatabase.h>
#include <filesystem>
#include <PlayerName.pb.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

void SetupDatabase() {
	PlayerDatabase playerData(fs::path("playerdata.sqlite"));
	playerData.AddPrototype(FieldKey::PLAYER_INGAME_NAME, "PlayerName");
	PlayerName name;
	name.set_name("myname");
	sql::Statement statement = playerData.GetStatement(
		"INSERT INTO ? (?) VALUES (?)"
	);
	playerData.SetField(statement, FieldKey::PLAYER_INGAME_NAME, &name);
	sql::Statement fetchVal = playerData.GetStatement(
		"SELECT ? FROM ? LIMIT 1"
	);
	std::shared_ptr<PlayerName> nameFetched = std::dynamic_pointer_cast<PlayerName>(
		playerData.GetField(fetchVal, FieldKey::PLAYER_INGAME_NAME));
	spdlog::info(nameFetched->name());
}