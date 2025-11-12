#pragma once
#include "Database.h"
#include <CreatePartyRequest.pb.h>

class PartyDatabase : public Database {
private:
	static PartyDatabase inst;
public:
	static PartyDatabase& Get();
	PartyDatabase(fs::path path);
	PartyResponse GetPartyRes(const std::string& partyId);
	PartyResponse GetPartyResByInviteCode(const std::string& inviteCode);
	Party GetParty(const std::string& partyId);
	Party GetPartyByInviteCode(const std::string& inviteCode);
	void SaveParty(const Party& party);
	static std::string SerializePartyToString(const PartyResponse& partyRes);
};