#include <GetIdentitiesProcessor.h>
#include <GetPlayerIdentitiesRequest.pb.h>

#include <PlayerDatabase.h>
#include <ProfileData.pb.h>
#include <SteamValidator.h>
#include <AuthCfg.h>

static const AuthCfg& GetAuthCfg() {
    static AuthCfg cfg = [] {
        AuthCfg c{};
        auto try_path = [&](const char* p) {
            if (std::ifstream f(p); f.is_open()) {
                auto j = json::parse(f, nullptr, false);
                if (!j.is_discarded() && j.contains("steamApiKey") && j["steamApiKey"].is_string()) {
                    c.steamApiKey = j["steamApiKey"].get<std::string>();
                }
            }
        };
        try_path("auth.json");
        return c;
    }();
    return cfg;
}

SteamValidator GetIdentitiesProcessor::steamValidator(GetAuthCfg().steamApiKey);

GetIdentitiesProcessor::GetIdentitiesProcessor(SpectreRpcType rpcType) : WebsocketPacketProcessor(rpcType)
{

}


void GetIdentitiesProcessor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock)
{
    std::unique_ptr<GetPlayerIdentitiesRequest> req = packet.GetPayloadAsMessage<GetPlayerIdentitiesRequest>();
    GetPlayerIdentitiesResponse res;
    for (int i = 0; i < req->provideraccountids_size(); i++)
    {
        const ProviderAccountId& steamAccId = req->provideraccountids(i);
        std::string pragmaId = PlayerDatabase::Get().LookupPlayerByProvider(steamAccId.idprovidertype(), steamAccId.provideraccountid());
        PlayerIdentity* curIdentity = res.add_playeridentities();
        curIdentity->set_pragmaplayerid(pragmaId);
        curIdentity->set_pragmasocialid(pragmaId);
        std::unique_ptr<ProfileData> pd = PlayerDatabase::Get().GetField<ProfileData>(FieldKey::PROFILE_DATA, pragmaId);
        curIdentity->mutable_pragmadisplayname()->CopyFrom(pd->displayname());
        IdProviderAccount* steamAcc = curIdentity->add_idprovideraccounts();
        steamAcc->set_accountid(steamAccId.provideraccountid());
        steamAcc->set_idprovidertype(steamAccId.idprovidertype());
        DisplayName* steamDisplayName = steamAcc->mutable_providerdisplayname();
        std::optional<SteamPlayerInfo> steamInfo = steamValidator.ValidateSteamId(steamAccId.provideraccountid());
        steamDisplayName->set_displayname(steamInfo.value().personaName);
    }
    sock.SendPacket(res, packet.GetResponseType(), packet.GetRequestId());
}
