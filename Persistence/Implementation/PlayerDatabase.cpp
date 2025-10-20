#include "PlayerDatabase.h"

void PlayerDatabase::OnDbLoad() {
	GetRaw()->exec("CREATE TABLE IF NOT EXISTS players (ID TEXT PRIMARY KEY, PlayerName TEXT);");
}

const char* PlayerDatabase::GetTableName() {
	return "players";
}