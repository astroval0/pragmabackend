#include "PlayerDatabase.h"

PlayerDatabase PlayerDatabase::inst;

PlayerDatabase& PlayerDatabase::Get() {
	return inst;
}

PlayerDatabase::PlayerDatabase(fs::path path) : Database(path) {
	GetRaw()->exec("CREATE TABLE IF NOT EXISTS players (ID TEXT PRIMARY KEY, PlayerName BLOB);");
}

const std::string PlayerDatabase::GetTableName() {
	return "players";
}