#include <nlohmann/json.hpp>
#include <fstream>
#include <StaticResponseProcessorWS.h>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

void RegisterStaticHandlerFromFile(std::string filename, SpectreRpcType rpcType, Site site) {
	std::ifstream resfile(filename);
	if (!resfile.is_open()) {
		throw std::runtime_error("failed to open response file");
	}
	json res = json::parse(resfile);
	resfile.close();
	new StaticResponseProcessorWS(rpcType, site, std::make_shared<json>(std::move(res)));
}

#pragma warning(push)
#pragma warning(disable: 4101)
void RegisterStaticWSHandlers() {
	RegisterStaticHandlerFromFile("responses/inventory.json", SpectreRpcType("InventoryRpc.GetInventoryV2Request"), Site::Game);
	RegisterStaticHandlerFromFile("responses/config.json", SpectreRpcType("MtnConfigServiceRpc.GetConfigForClientV1Request"), Site::Game);
	RegisterStaticHandlerFromFile("responses/beaconendpoints.json", SpectreRpcType("MtnBeaconServiceRpc.GetBeaconEndpointsV1Request"), Site::Game);
	RegisterStaticHandlerFromFile("responses/syncinventoryentitlements.json", SpectreRpcType("InventoryRpc.SyncEntitlementsV1Request"), Site::Game);
	RegisterStaticHandlerFromFile("responses/logindata.json", SpectreRpcType("GameDataRpc.GetLoginDataV3Request"), Site::Game);
	RegisterStaticHandlerFromFile("responses/recordclientanalytics.json", SpectreRpcType("MtnAnalyticsNodeServiceRpc.RecordClientAnalyticsEventV1Request"), Site::Game);
	RegisterStaticHandlerFromFile("responses/getfriendlistandregisteronline.json", SpectreRpcType("FriendRpc.GetFriendListAndRegisterOnlineV1Request"), Site::Social);
	RegisterStaticHandlerFromFile("responses/playeridentitiesbyplatform.json", SpectreRpcType("AccountRpc.GetPlayerIdentitiesByProviderAccountIdsV1Request"), Site::Social);
	RegisterStaticHandlerFromFile("responses/setsocialstatus.json", SpectreRpcType("FriendRpc.SetPresenceV1Request"), Site::Social);
	return;
}
#pragma warning(pop)