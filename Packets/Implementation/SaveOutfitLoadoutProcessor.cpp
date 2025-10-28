#include <SaveOutfitLoadoutProcessor.h>
#include <OutfitLoadout.pb.h>
#include <PlayerDatabase.h>

SaveOutfitLoadoutProcessor::SaveOutfitLoadoutProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

void SaveOutfitLoadoutProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<OutfitLoadout> loadoutToSave = packet.GetPayloadAsMessage<OutfitLoadout>();
	sql::Statement getLoadout = PlayerDatabase::Get().FormatStatement(
		"SELECT {col} from {table} WHERE PlayerId = ?",
		FieldKey::PLAYER_OUTFIT_LOADOUT
	);
	std::string toFindLoadoutId = loadoutToSave->loadoutid();
	std::transform(toFindLoadoutId.begin(), toFindLoadoutId.end(), toFindLoadoutId.begin(),
		[](unsigned char c) {return std::tolower(c); });
	std::unique_ptr<OutfitLoadouts> loadouts = PlayerDatabase::Get().GetField<OutfitLoadouts>(getLoadout, FieldKey::PLAYER_OUTFIT_LOADOUT);
	bool dataWritten = false;
	for (int i = 0; i < loadouts->loadouts_size(); i++) {
		if (loadouts->loadouts(i).loadoutid() == toFindLoadoutId) {
			loadouts->mutable_loadouts(i)->CopyFrom(*loadoutToSave);
			dataWritten = true;
			break;
		}
	}
	if (!dataWritten) {
		loadouts->add_loadouts()->CopyFrom(*loadoutToSave);
	}
	sql::Statement setLoadout = PlayerDatabase::Get().FormatStatement(
		"INSERT OR REPLACE INTO {table} (PlayerId, {col}) VALUES (?, ?)",
		FieldKey::PLAYER_OUTFIT_LOADOUT
	);
	setLoadout.bind(1, sock.GetPlayerId());
	PlayerDatabase::Get().SetField(setLoadout, FieldKey::PLAYER_OUTFIT_LOADOUT, loadouts.get(), 2);
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	(*res)["payload"]["success"] = true;
	(*res)["payload"]["savedLoadoutId"] = loadoutToSave->loadoutid();
	sock.SendPacket(res);
}