#include "PlayerDatabase.h"
#include <Inventory.pb.h>

PlayerDatabase::PlayerDatabase(fs::path path) : Database(path, "players", "PlayerID", "TEXT") {
	AddPrototype<Inventory>(FieldKey::PLAYER_INVENTORY);
}

PlayerDatabase PlayerDatabase::inst("playerdata.sqlite");

PlayerDatabase& PlayerDatabase::Get() {
	return inst;
}