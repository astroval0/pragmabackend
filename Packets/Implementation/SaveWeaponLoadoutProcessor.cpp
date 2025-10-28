#include <SaveWeaponLoadoutProcessor.h>
#include <WeaponLoadoutMessage.pb.h>
#include <WeaponLoadout.pb.h>
#include <PlayerDatabase.h>

SaveWeaponLoadoutProcessor::SaveWeaponLoadoutProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SaveWeaponLoadoutProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	const WeaponLoadout& dataToSave = packet.GetPayloadAsMessage<SaveWeaponLoadoutMessage>()->weaponloadoutdata();
	sql::Statement getLoadout = PlayerDatabase::Get().FormatStatement(
		"SELECT {col} from {table} WHERE PlayerId = ?",
		FieldKey::PLAYER_WEAPON_LOADOUT
	);
	std::string toFindLoadoutId = dataToSave.loadoutid();
	std::transform(toFindLoadoutId.begin(), toFindLoadoutId.end(), toFindLoadoutId.begin(),
		[](unsigned char c) {return std::tolower(c); }); // now getting error every request, to investigate
	std::unique_ptr<WeaponLoadouts> loadouts = PlayerDatabase::Get().GetField<WeaponLoadouts>(getLoadout, FieldKey::PLAYER_WEAPON_LOADOUT);
	bool dataWritten = false;
	for (int i = 0; i < loadouts->weaponloadoutdata_size(); i++) {
		if (loadouts->weaponloadoutdata(i).loadoutid() == toFindLoadoutId) {
			loadouts->mutable_weaponloadoutdata(i)->CopyFrom(dataToSave);
			dataWritten = true;
			break;
		}
	}
	if (!dataWritten) {
		loadouts->add_weaponloadoutdata()->CopyFrom(dataToSave);
	}
	sql::Statement setLoadout = PlayerDatabase::Get().FormatStatement(
		"INSERT OR REPLACE INTO {table} (PlayerId, {col}) VALUES (?, ?)",
		FieldKey::PLAYER_WEAPON_LOADOUT
	);
	setLoadout.bind(1, sock.GetPlayerId());
	PlayerDatabase::Get().SetField(setLoadout, FieldKey::PLAYER_WEAPON_LOADOUT, loadouts.get(), 2);
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"]["success"] = true;
	(*res)["payload"]["savedLoadoutId"] = dataToSave.loadoutid();
	sock.SendPacket(res);
}