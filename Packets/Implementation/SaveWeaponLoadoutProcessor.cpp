#include <SaveWeaponLoadoutProcessor.h>
#include <SaveWeaponLoadoutMessage.pb.h>
#include <WeaponLoadout.pb.h>
#include <PlayerDatabase.h>
#include <CaseHelper.h>

SaveWeaponLoadoutProcessor::SaveWeaponLoadoutProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SaveWeaponLoadoutProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<SaveWeaponLoadoutMessage> dataToSave = packet.GetPayloadAsMessage<SaveWeaponLoadoutMessage>();
	const WeaponLoadout& loadoutToSave = dataToSave->weaponloadoutdata();

	std::unique_ptr<WeaponLoadouts> loadouts = PlayerDatabase::Get().GetField<WeaponLoadouts>(
		FieldKey::PLAYER_WEAPON_LOADOUT,
		sock.GetPlayerId()
	);

	if (!loadouts) loadouts = std::make_unique<WeaponLoadouts>();

	bool dataWritten = false;
	for (int i = 0; i < loadouts->weaponloadoutdata_size(); i++) {

		if (iequals(loadouts->weaponloadoutdata(i).loadoutid(), loadoutToSave.loadoutid())) {

			loadouts->mutable_weaponloadoutdata(i)->CopyFrom(loadoutToSave);
			loadouts->mutable_weaponloadoutdata(i)->set_playerid(sock.GetPlayerId());

			dataWritten = true;
			break;
		}
	}
	if (!dataWritten) {
		spdlog::warn("didn't find the weapon loadout the game was trying to edit, added it as a new loadout\nLoadout id: {}", loadoutToSave.loadoutid());
		WeaponLoadout* newLoadout = loadouts->add_weaponloadoutdata();
		newLoadout->CopyFrom(loadoutToSave);
		newLoadout->set_playerid(sock.GetPlayerId());
	}
	PlayerDatabase::Get().SetField(FieldKey::PLAYER_WEAPON_LOADOUT, loadouts.get(), sock.GetPlayerId());
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"]["success"] = true;
	(*res)["payload"]["savedLoadoutId"] = loadoutToSave.loadoutid();
	sock.SendPacket(res);
}