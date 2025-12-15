#include <nlohmann/json.hpp>
#include <fstream>
#include <StaticResponseProcessorWS.h>
#include <RegexPayloadProcessorWS.h>
#include <spdlog/spdlog.h>
#include <string>

void RegisterStaticHandlerFromFile(std::string filename, SpectreRpcType rpcType) {
	std::ifstream resfile(filename);
	if (!resfile.is_open()) {
		throw std::runtime_error("failed to open response file");
	}
	json res = json::parse(resfile);
	resfile.close();
	spdlog::info("registered static WS {} from json file at {}", rpcType.GetName(), filename);
	new StaticResponseProcessorWS(rpcType, std::make_shared<json>(std::move(res)));
}

void RegisterRegexHandlerFromFiles(SpectreRpcType rpcType, std::unordered_map<regex, std::string> map) {
	std::unordered_map<regex, std::shared_ptr<json>> map2;
	for (const auto& [regex, filename] : map) {
		std::ifstream resfile(filename);
		if (!resfile.is_open()) {
			throw std::runtime_error("failed to open res file");
		}
		json res = json::parse(resfile);
		resfile.close();
		map2.insert({ regex, std::make_shared<json>(std::move(res)) });
	}
	spdlog::info("registered static WS {} using regex handler", rpcType.GetName());
	new RegexPayloadProcessorWS(rpcType, map2);
}

#pragma warning(push)
#pragma warning(disable: 4101)
void RegisterStaticWSHandlers() {
	for (const auto& file : fs::recursive_directory_iterator("resources/payloads/static/ws/game")) {
		if (!fs::is_regular_file(file)) continue;
		std::string rpcType = file.path().filename().stem().string();
		RegisterStaticHandlerFromFile(fs::absolute(file.path()).string(), SpectreRpcType(rpcType));
	}
	/*for (const auto& file : fs::recursive_directory_iterator("resources/payloads/static/ws/social")) {
		if (!fs::is_regular_file(file)) continue;
		std::string rpcType = file.path().filename().stem().string();
		RegisterStaticHandlerFromFile(fs::absolute(file.path()).string(), SpectreRpcType(rpcType));
	} dont have any of these rn and git doesn't sync empty dirs */
	RegisterRegexHandlerFromFiles(SpectreRpcType("MtnBeaconServiceRpc.GetBeaconEndpointsV1Request"), {
		{regex("hathora-udp\""), "resources/payloads/ws/game/beacon/hathora-udp.json"},
		{regex("hathora\""), "resources/payloads/ws/game/beacon/hathora.json"}
	});
}
#pragma warning(pop)