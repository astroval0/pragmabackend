#include <SaveWeaponLoadoutProcessor.h>
#include <SaveWeaponLoadoutMessage.pb.h>
#include <WeaponLoadout.pb.h>
#include <PlayerDatabase.h>

SaveWeaponLoadoutProcessor::SaveWeaponLoadoutProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SaveWeaponLoadoutProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<SaveWeaponLoadoutMessage> dataToSave = packet.GetPayloadAsMessage<SaveWeaponLoadoutMessage>();
	const WeaponLoadout& loadoutToSave = dataToSave->weaponloadoutdata();
	sql::Statement getLoadout = PlayerDatabase::Get().FormatStatement(
		"SELECT {col} from {table} WHERE PlayerId = ?",
		FieldKey::PLAYER_WEAPON_LOADOUT
	);
	std::string toFindLoadoutId = loadoutToSave.loadoutid();
	std::transform(toFindLoadoutId.begin(), toFindLoadoutId.end(), toFindLoadoutId.begin(),
		[](unsigned char c) {return std::tolower(c); });
	std::unique_ptr<WeaponLoadouts> loadouts = PlayerDatabase::Get().GetField<WeaponLoadouts>(getLoadout, FieldKey::PLAYER_WEAPON_LOADOUT);
	bool dataWritten = false;
	for (int i = 0; i < loadouts->weaponloadoutdata_size(); i++) {
		if (loadouts->weaponloadoutdata(i).loadoutid() == toFindLoadoutId) {
			loadouts->mutable_weaponloadoutdata(i)->CopyFrom(loadoutToSave);
			dataWritten = true;
			break;
		}
	}
	if (!dataWritten) {
		loadouts->add_weaponloadoutdata()->CopyFrom(loadoutToSave);
	}
	sql::Statement setLoadout = PlayerDatabase::Get().FormatStatement(
		"INSERT OR REPLACE INTO {table} (PlayerId, {col}) VALUES (?, ?)",
		FieldKey::PLAYER_WEAPON_LOADOUT
	);
	setLoadout.bind(1, sock.GetPlayerId());
	PlayerDatabase::Get().SetField(setLoadout, FieldKey::PLAYER_WEAPON_LOADOUT, loadouts.get(), 2);
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"]["success"] = true;
	(*res)["payload"]["savedLoadoutId"] = loadoutToSave.loadoutid();
	sock.SendPacket(res);
}