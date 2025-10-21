#include <PlayerDatabase.h>
#include <filesystem>
#include <PlayerName.pb.h>
#include <memory>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

void SetupDatabase() {
	PlayerDatabase playerData(fs::path("playerdata.sqlite"));
	playerData.AddPrototype<PlayerName>(FieldKey::PLAYER_INGAME_NAME);
}