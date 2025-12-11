#include <SaveOutfitLoadoutProcessor.h>
#include <OutfitLoadout.pb.h>
#include <PlayerDatabase.h>
#include <CaseHelper.h>

SaveOutfitLoadoutProcessor::SaveOutfitLoadoutProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SaveOutfitLoadoutProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<OutfitLoadout> loadoutToSave = packet.GetPayloadAsMessage<OutfitLoadout>();
	std::unique_ptr<OutfitLoadouts> loadouts = PlayerDatabase::Get().GetField<OutfitLoadouts>(FieldKey::PLAYER_OUTFIT_LOADOUT, sock.GetPlayerId());
	if (!loadouts) loadouts = std::make_unique<OutfitLoadouts>();

	bool dataWritten = false;
	for (int i = 0; i < loadouts->loadouts_size(); i++) {
		if (iequals(loadouts->loadouts(i).loadoutid(), loadoutToSave->loadoutid())) {

			loadouts->mutable_loadouts(i)->CopyFrom(*loadoutToSave);
			loadouts->mutable_loadouts(i)->set_playerid(sock.GetPlayerId());

			dataWritten = true;
			break;
		}
	}
	if (!dataWritten) {
		spdlog::warn("didn't find the outfit loadout the game was trying to edit, added it as a new loadout\nLoadout id: {}", loadoutToSave->loadoutid());
		OutfitLoadout* newLoadout = loadouts->add_loadouts();
		newLoadout->CopyFrom(*loadoutToSave);
		newLoadout->set_playerid(sock.GetPlayerId());
	}
	PlayerDatabase::Get().SetField(FieldKey::PLAYER_OUTFIT_LOADOUT, loadouts.get(), sock.GetPlayerId());
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"]["success"] = true;
	(*res)["payload"]["savedLoadoutId"] = loadoutToSave->loadoutid();
	sock.SendPacket(res);
}