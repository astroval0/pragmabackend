#include <CreatePartyProcessor.h>
#include <CreatePartyRequest.pb.h>
#include <stduuid/uuid.h>
#include <PartyDatabase.h>
#include <PlayerDatabase.h>
#include <ProfileData.pb.h>
#include <Inventory.pb.h>
#include <google/protobuf/util/json_util.h>

static const std::string inviteCodeChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int inviteCodeNChars = 6;
static const std::string sharedClientDataStart = "sharedClientData\":";

CreatePartyProcessor::CreatePartyProcessor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType), uuidgen(stdrandgen) {

}

std::string CreatePartyProcessor::GetRandomUUIDAsString() {
	return uuids::to_string(uuidgen());
}

std::string CreatePartyProcessor::GetNewInviteCode() {
	std::uniform_int_distribution<> dist(0, inviteCodeChars.size() - 1);
	while (true) {
		std::string inviteCode;
		for (int i = 0; i < inviteCodeNChars; i++) {
			inviteCode += inviteCodeChars[dist(stdrandgen)];
		}
		sql::Statement query(*PartyDatabase::Get().GetRaw(), "SELECT 1 FROM " + PartyDatabase::Get().GetTableName() + " WHERE PartyCode = ? LIMIT 1");
		query.bind(1, inviteCode);
		if (query.executeStep()) {
			continue;
		}
		return inviteCode;
	}
}

void CreatePartyProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::unique_ptr<CreatePartyRequest> req = packet.GetPayloadAsMessage<CreatePartyRequest>();
	PartyResponse createdPartyRes;

	Party* party = createdPartyRes.mutable_party();
	party->set_partyid(GetRandomUUIDAsString());
	party->set_invitecode(GetNewInviteCode());
	party->add_preferredgameserverzones("uscentral-1");
	party->set_version("1");

	BroadcastPartyExtraInfo* pExtra = party->mutable_extbroadcastparty();
	(*pExtra->mutable_standard())["mode"] = "Standard";
	pExtra->set_lobbymode("standard_casual");
	pExtra->set_version("171268");
	pExtra->set_hasacceptableregion(true);
	pExtra->mutable_crossplaypreference()->set_platform("CROSS_PLAY_PLATFORM_PC");
	PartyMember* creatingPlayer = party->add_partymembers();
	creatingPlayer->set_isleader(true);
	creatingPlayer->set_isready(false);

	std::unique_ptr<ProfileData> playerProfile = PlayerDatabase::Get().GetField<ProfileData>(FieldKey::PROFILE_DATA, sock.GetPlayerId());
	creatingPlayer->mutable_displayname()->CopyFrom(playerProfile->displayname());
	creatingPlayer->set_playerid(sock.GetPlayerId());
	creatingPlayer->set_socialid(sock.GetPlayerId());

	PartyMemberExtraInfo* creatingPlayerExtra = creatingPlayer->mutable_ext();
	creatingPlayerExtra->set_version("171268");
	creatingPlayerExtra->set_preferredteam("TEAM0");
	creatingPlayerExtra->set_rankedmodeunlocked(true);

	PartyMemberPlayerData* partyPlayerDat = creatingPlayerExtra->mutable_playerdata();
	std::unique_ptr<PlayerData> playerDat = PlayerDatabase::Get().GetField<PlayerData>(FieldKey::PLAYER_DATA, sock.GetPlayerId());
	partyPlayerDat->mutable_defenderweaponloadout()->set_playerid(sock.GetPlayerId());
	partyPlayerDat->mutable_defenderweaponloadout()->set_loadoutid(playerDat->defenderweaponloadoutid());
	partyPlayerDat->mutable_attackerweaponloadout()->set_playerid(sock.GetPlayerId());
	partyPlayerDat->mutable_attackerweaponloadout()->set_loadoutid(playerDat->attackerweaponloadoutid());
	partyPlayerDat->mutable_matchmakingdata()->CopyFrom(playerDat->matchmakingdata());
	partyPlayerDat->mutable_banner()->CopyFrom(playerDat->banner());

	std::unique_ptr<OutfitLoadouts> outfitLoadouts = PlayerDatabase::Get().GetField<OutfitLoadouts>(FieldKey::PLAYER_OUTFIT_LOADOUT, sock.GetPlayerId());
	OutfitLoadout* selectedAttackerOutfit = nullptr;
	OutfitLoadout* selectedDefenderOutfit = nullptr;
	for(int i = 0; i < outfitLoadouts->loadouts_size(); i++) {
		OutfitLoadout* loadout = outfitLoadouts->mutable_loadouts(i);
		if (loadout->loadoutid() == playerDat->attackeroutfitloadoutid()) {
			selectedAttackerOutfit = loadout;
		}
		if (loadout->loadoutid() == playerDat->defenderoutfitloadoutid()) {
			selectedDefenderOutfit = loadout;
		}
	}
	if (selectedAttackerOutfit == nullptr || selectedDefenderOutfit == nullptr) {
		spdlog::error("Could not find selected outfit loadouts for player {}", sock.GetPlayerId());
		throw;
	}
	partyPlayerDat->mutable_attackeroutfitloadout()->CopyFrom(*selectedAttackerOutfit);
	partyPlayerDat->mutable_defenderoutfitloadout()->CopyFrom(*selectedDefenderOutfit);

	std::unique_ptr<Inventory> invstruct = PlayerDatabase::Get().GetField<Inventory>(FieldKey::PLAYER_INVENTORY, sock.GetPlayerId());
	const FullInventory& inv = invstruct->full();
	for (int i = 0; i < inv.instanced_size(); i++) {
		// TODO make this only actually return the items needed for performance, but for MVP this should be fine
		creatingPlayerExtra->add_limitedinstancedinventory()->CopyFrom(inv.instanced(i));
	}

	PartySharedClientData* sharedData = creatingPlayerExtra->mutable_sharedclientdata();
	sharedData->set_accountidprovider("STEAM");
	sharedData->set_platformname("STEAM");
	sharedData->set_crossplayplatformkind("CROSS_PLAY_PLATFORM_PC");
	// TODO use actual steam id
	sharedData->set_currentprovideraccountid("76561199041068696");

	std::string jsoninit;
	pbuf::util::JsonPrintOptions popts;
	popts.always_print_fields_with_no_presence = true;
	auto status = pbuf::util::MessageToJsonString(createdPartyRes, &jsoninit, popts);
	if (!status.ok()) {
		spdlog::error("Failed to serialize CreatePartyProcessor response: {}", status.message());
		throw;
	}
	std::string jsonfinal;
	size_t pos = jsoninit.find(sharedClientDataStart);
	if (pos == std::string::npos) {
		spdlog::error("did not find sharedClientData property in CreatePartyProcessor res json, something weird has happened");
		throw;
	}
	pos += sharedClientDataStart.size();
	jsonfinal = std::string(jsoninit.begin(), jsoninit.begin() + pos);
	jsonfinal += '\"';
	char curChar = jsoninit[pos];
	while (curChar != '}') {
		if (curChar == '\"') {
			jsonfinal += "\\\"";
		}
		else {
			jsonfinal += curChar;
		}
		pos++;
		curChar = jsoninit[pos];
	}
	jsonfinal += "}\"";
	jsonfinal += std::string(jsoninit.begin() + pos + 1, jsoninit.end());
	sock.SendPacket(jsonfinal, packet.GetRequestId(), packet.GetResponseType());
}