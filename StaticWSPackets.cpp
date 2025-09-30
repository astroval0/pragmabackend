#include <nlohmann/json.hpp>
#include <fstream>
#include <StaticResponseProcessorWS.h>
#include <stdexcept>

using json = nlohmann::json;

void RegisterStaticHandlerFromFile(std::string filename, SpectreRpcType rpcType) {
	std::ifstream resfile(filename);
	if (!resfile.is_open()) {
		throw std::runtime_error("failed to open response file");
	}
	json res;
	resfile >> res;
	resfile.close();
	new StaticResponseProcessorWS(rpcType, res);
}

#pragma warning(push)
#pragma warning(disable: 4101)
void RegisterStaticWSHandlers() {
	RegisterStaticHandlerFromFile("responses/inventory.json", SpectreRpcType("InventoryRpc.GetInventoryV2Request"));
	RegisterStaticHandlerFromFile("responses/config.json", SpectreRpcType("MtnConfigServiceRpc.GetConfigForClientV1Request"));
	return;
}
#pragma warning(pop)