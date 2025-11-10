#include <UpdateItemsV0Processor.h>
#include <Inventory.pb.h>
#include <PlayerDatabase.h>
#include <spdlog/spdlog.h>
#include <UpdatesItemMessage.pb.h>
#include <algorithm>
#include <google/protobuf/util/json_util.h>
#include <cctype>

namespace pbu = google::protobuf::util;

UpdateItemsV0Processor::UpdateItemsV0Processor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {
}

void UpdateItemsV0Processor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::shared_ptr<json> res = packet.GetBaseJsonResponse();
	sql::Statement invQuery = PlayerDatabase::Get().FormatStatement(
		"SELECT {col} from {table} WHERE PlayerId = ?",
		FieldKey::PLAYER_INVENTORY
	);
	invQuery.bind(1, sock.GetPlayerId());
	std::unique_ptr<Inventory> playerI = PlayerDatabase::Get().GetField<Inventory>(invQuery, FieldKey::PLAYER_INVENTORY);
	pbu::JsonPrintOptions opts;
	opts.always_print_fields_with_no_presence = true;
	FullInventory* playerInv = playerI->mutable_full();
	int invLevel = stoi(playerInv->version());
	playerInv->set_version(std::to_string(invLevel + 1));
	std::unique_ptr<UpdatesItemMessage> itemUpdates = packet.GetPayloadAsMessage<UpdatesItemMessage>();
	for (const InstancedItemUpdate& itemUpdate : itemUpdates->instanceditemupdates()) {
		std::string instanceId = itemUpdate.instanceid();
		std::transform(instanceId.begin(), instanceId.end(), instanceId.begin(),
			[](unsigned char c) {return std::tolower(c); });
		InstancedItem* curItem = nullptr;
		for (int i = 0; i < playerInv->instanced_size(); i++) {
			std::string testInstId = playerInv->instanced(i).instanceid();
			if (testInstId == instanceId) {
				curItem = playerInv->mutable_instanced(i);
				break;
			}
		}
		if (!curItem) {
			spdlog::warn("Couldn't find item with instance id {} in a item update request, skipping", instanceId);
			continue;
		}
		std::string curInstanced;
		if (!pbu::MessageToJsonString(*curItem, &curInstanced, opts).ok()) {
			spdlog::error("failed to serialize item to json");
			continue;
		}
		(*res)["payload"]["delta"]["instanced"].push_back({
			{"catalogId", curItem->instanceid()},
			{"tags", json::array()},
			{"operation", "UPDATED"},
			{"initial", json::parse(curInstanced)}
		}
		);
		if (itemUpdate.ext().setviewed()) {
			curItem->mutable_ext()->set_viewed(true);
		}
		std::string finalInstanced;
		if (!pbu::MessageToJsonString(*curItem, &finalInstanced, opts).ok()) {
			spdlog::error("failed to serialize final item to json");
		}
		(*res)["payload"]["delta"]["instanced"][(*res)["payload"]["delta"]["instanced"].size()]["final"] = json::parse(finalInstanced);
		(*res)["payload"]["segment"]["instanced"].push_back(json::parse(finalInstanced));
	}
	PlayerDatabase::Get().SetField(FieldKey::PLAYER_INVENTORY, playerI.get(), sock.GetPlayerId());
	(*res)["payload"]["segment"]["removedStackables"] = json::array();
	(*res)["payload"]["segment"]["removedInstanced"] = json::array();
	(*res)["payload"]["segment"]["previousVersion"] = std::to_string(invLevel);
	(*res)["payload"]["segment"]["version"] = playerInv->version();
	sock.SendPacket(res);
}