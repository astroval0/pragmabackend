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
	return;
}
#pragma warning(pop)