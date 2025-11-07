#include <PartyDatabase.h>
#include <google/protobuf/util/json_util.h>

PartyDatabase::PartyDatabase(fs::path path) : Database(path, "parties", "PartyID", "TEXT") {
	sql::Statement colQuery(GetRawRef(), "PRAGMA table_info(" + GetTableName() + ");");
	bool colExists = false;
	while (colQuery.executeStep()) {
		std::string colName = colQuery.getColumn(1).getText();
		if (colName == "PartyCode") {
			colExists = true;
			break;
		}
	}
	if (!colExists) {
		GetRaw()->exec("ALTER TABLE " + GetTableName() + " ADD COLUMN PartyCode TEXT;");
	}
	AddPrototype<BroadcastPartyExtraInfo>(FieldKey::PARTY_EXTRA_BROADCAST_INFO);
	AddPrototype<BroadcastPrivatePartyExtraInfo>(FieldKey::PARTY_PRIVATE_EXTRA_BROADCAST_INFO);
	AddPrototype<PartyMembers>(FieldKey::PARTY_MEMBERS);
}

PartyDatabase PartyDatabase::inst("playerdata.sqlite");

PartyDatabase& PartyDatabase::Get() {
	return inst;
}

void PartyDatabase::SaveParty(const Party& party) {
	PartyMembers members;
	for (int i = 0; i < party.partymembers_size(); i++) {
		members.add_members()->CopyFrom(party.partymembers(i));
	}
	SetField(FieldKey::PARTY_MEMBERS, &members, party.partyid());
	SetField(FieldKey::PARTY_EXTRA_BROADCAST_INFO, &party.extbroadcastparty(), party.partyid());
	SetField(FieldKey::PARTY_PRIVATE_EXTRA_BROADCAST_INFO, &party.extprivateplayer(), party.partyid());
}

Party PartyDatabase::GetParty(const std::string& partyId) {
	Party party;
	std::unique_ptr<PartyMembers> members = GetField<PartyMembers>(FieldKey::PARTY_MEMBERS, partyId);
	for (int i = 0; i < members->members_size(); i++) {
		party.add_partymembers()->CopyFrom(members->members(i));
	}
	party.set_partyid(partyId);
	sql::Statement getInviteCode(
		GetRawRef(),
		"SELECT PartyCode FROM " + GetTableName() + " WHERE PartyID = ?"
	);
	getInviteCode.bind(1, partyId);
	if (!getInviteCode.executeStep()) {
		spdlog::error("failed to find invite code for party: {}", partyId);
		throw;
	}
	party.set_invitecode(getInviteCode.getColumn("PartyCode").getString());
	party.add_preferredgameserverzones("uscentral-1");
	party.set_version("1");
	return party;
}

PartyResponse PartyDatabase::GetPartyRes(const std::string& partyId) {
	PartyResponse res;
	Party party = GetParty(partyId);
	res.mutable_party()->CopyFrom(party);
	return res;
}

Party PartyDatabase::GetPartyByInviteCode(const std::string& inviteCode) {
	Party party;
	sql::Statement getPartyId(
		GetRawRef(),
		"SELECT PartyID FROM " + GetTableName() + " WHERE PartyCode = ?"
	);
	getPartyId.bind(1, inviteCode);
	if (!getPartyId.executeStep()) {
		spdlog::error("failed to find party id for party from invite code: {}\n are you sure that this invite code hasn't expired?", inviteCode);
		throw;
	}
	std::string partyId = getPartyId.getColumn("PartyID");
	std::unique_ptr<PartyMembers> members = GetField<PartyMembers>(FieldKey::PARTY_MEMBERS, partyId);
	for (int i = 0; i < members->members_size(); i++) {
		party.add_partymembers()->CopyFrom(members->members(i));
	}
	party.set_partyid(partyId);

	party.set_invitecode(inviteCode);
	party.add_preferredgameserverzones("uscentral-1");
	party.set_version("1");
	return party;
}

PartyResponse PartyDatabase::GetPartyResByInviteCode(const std::string& inviteCode) {
	PartyResponse res;
	Party party = GetPartyByInviteCode(inviteCode);
	res.mutable_party()->CopyFrom(party);
	return res;
}

static const std::string sharedClientDataStart = "sharedClientData\":";

std::string PartyDatabase::SerializePartyToString(const PartyResponse& partyRes){
	std::string jsoninit;
	pbuf::util::JsonPrintOptions popts;
	popts.always_print_fields_with_no_presence = true;
	auto status = pbuf::util::MessageToJsonString(partyRes, &jsoninit, popts);
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
	return jsonfinal;
}