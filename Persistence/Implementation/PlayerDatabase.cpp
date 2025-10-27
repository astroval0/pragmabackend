#include "PlayerDatabase.h"
#include <Inventory.pb.h>
#include <OutfitLoadout.pb.h>

PlayerDatabase::PlayerDatabase(fs::path path) : Database(path, "players", "PlayerID", "TEXT") {
	AddPrototype<Inventory>(FieldKey::PLAYER_INVENTORY, "resources/payloads/ws/game/DefaultInventory.json");
	AddPrototype<OutfitLoadouts>(FieldKey::PLAYER_OUTFIT_LOADOUT, "resources/payloads/ws/game/DefaultOutfitLoadout.json");
}

PlayerDatabase PlayerDatabase::inst("playerdata.sqlite");

PlayerDatabase& PlayerDatabase::Get() {
	return inst;
}