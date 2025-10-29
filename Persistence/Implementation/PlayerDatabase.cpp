#include "PlayerDatabase.h"
#include <Inventory.pb.h>
#include <OutfitLoadout.pb.h>
#include <WeaponLoadout.pb.h>
#include <PlayerData.pb.h>

PlayerDatabase::PlayerDatabase(fs::path path) : Database(path, "players", "PlayerID", "TEXT") {
	AddPrototype<Inventory>(FieldKey::PLAYER_INVENTORY, "resources/payloads/ws/game/DefaultInventory.json");
	AddPrototype<OutfitLoadouts>(FieldKey::PLAYER_OUTFIT_LOADOUT, "resources/payloads/ws/game/DefaultOutfitLoadout.json");
	AddPrototype<WeaponLoadouts>(FieldKey::PLAYER_WEAPON_LOADOUT, "resources/payloads/ws/game/DefaultWeaponLoadout.json");
	AddPrototype<PlayerData>(FieldKey::PLAYER_DATA, "resources/payloads/ws/game/DefaultPlayerData.json");
}

PlayerDatabase PlayerDatabase::inst("playerdata.sqlite");

PlayerDatabase& PlayerDatabase::Get() {
	return inst;
}