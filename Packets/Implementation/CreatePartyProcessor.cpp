#include <CreatePartyProcessor.h>
#include <CreatePartyRequest.pb.h>
#include <uuid.h>
#include <PartyDatabase.h>
#include <PlayerDatabase.h>
#include <ProfileData.pb.h>
#include <Inventory.pb.h>
#include <google/protobuf/util/json_util.h>

static const std::string inviteCodeChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int inviteCodeNChars = 6;

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
	for (int i = 0; i < req->preferredgameserverzones_size(); i++) {
		party->add_preferredgameserverzones();
		party->set_preferredgameserverzones(i, req->preferredgameserverzones(i));
	}
	party->set_version("1");
	BroadcastPartyExtraInfo* pExtra = party->mutable_extbroadcastparty();
	(*pExtra->mutable_standard())["mode"] = "Standard";
	pExtra->set_lobbymode("standard_casual");
	pExtra->set_version("171268");
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
	std::unique_ptr<Inventory> invstruct = PlayerDatabase::Get().GetField<Inventory>(FieldKey::PLAYER_INVENTORY, sock.GetPlayerId());
	std::unique_ptr<WeaponLoadouts> wpnLoadouts = PlayerDatabase::Get().GetField<WeaponLoadouts>(FieldKey::PLAYER_WEAPON_LOADOUT, sock.GetPlayerId());
	const WeaponLoadout* wpnLoadout;
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
	sharedData->set_currentprovideraccountid(sock.GetPlayerId());
	std::string jsoninit;
	pbuf::util::MessageToJsonString(createdPartyRes, &jsoninit);
	sock.SendPacket(createdPartyRes, packet.GetResponseType(), packet.GetRequestId());
}