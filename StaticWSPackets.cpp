#include <nlohmann/json.hpp>
#include <fstream>
#include <StaticResponseProcessorWS.h>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

void RegisterStaticHandlerFromFile(std::string filename, SpectreRpcType rpcType) {
	std::ifstream resfile(filename);
	if (!resfile.is_open()) {
		throw std::runtime_error("failed to open response file");
	}
	json res = json::parse(resfile);
	resfile.close();
	new StaticResponseProcessorWS(rpcType, std::make_shared<json>(res));
}

#pragma warning(push)
#pragma warning(disable: 4101)
void RegisterStaticWSHandlers() {
	RegisterStaticHandlerFromFile("responses/inventory.json", SpectreRpcType("InventoryRpc.GetInventoryV2Request"));
	RegisterStaticHandlerFromFile("responses/config.json", SpectreRpcType("MtnConfigServiceRpc.GetConfigForClientV1Request"));
	RegisterStaticHandlerFromFile("responses/beaconendpoints.json", SpectreRpcType("MtnBeaconServiceRpc.GetBeaconEndpointsV1Request"));
	RegisterStaticHandlerFromFile("responses/syncinventoryentitlements.json", SpectreRpcType("InventoryRpc.SyncEntitlementsV1Request"));
	RegisterStaticHandlerFromFile("responses/logindata.json", SpectreRpcType("GameDataRpc.GetLoginDataV3Request"));
	RegisterStaticHandlerFromFile("responses/recordclientanalytics.json", SpectreRpcType("MtnAnalyticsNodeServiceRpc.RecordClientAnalyticsEventV1Request"));
	RegisterStaticHandlerFromFile("responses/getfriendlistandregisteronline.json", SpectreRpcType("FriendRpc.GetFriendListAndRegisterOnlineV1Request"));
	RegisterStaticHandlerFromFile("responses/playerindentitiesbyplatform.json", SpectreRpcType("AccountRpc.GetPlayerIdentitiesByProviderAccountIdsV1Request"));
	RegisterStaticHandlerFromFile("responses/setsocialstatus.json", SpectreRpcType("FriendRpc.SetPresenceV1Request"));
	return;
}
#pragma warning(pop)