#include <nlohmann/json.hpp>
#include <fstream>
#include <StaticResponseProcessorWS.h>
#include <stdexcept>

using json = nlohmann::json;

#pragma warning(push)
#pragma warning(disable: 4101)
static void RegisterStaticWSHandlers() {
	static std::ifstream inventoryresfile("responses/inventory.json");
	if (!inventoryresfile.is_open()) {
		throw std::runtime_error("failed to open inventory response file");
	}
	static json invres;
	inventoryresfile >> invres;
	inventoryresfile.close();
	new StaticResponseProcessorWS(SpectreRpcType("InventoryRpc.GetInventoryV2Request"), invres);
	return;
}
#pragma warning(pop)