#include <UpdateItemsV0Processor.h>
#include <Inventory.pb.h>
#include <PlayerDatabase.h>
#include <spdlog/spdlog.h>
#include <UpdatesItemMessage.pb.h>
#include <algorithm>
#include <cctype>

UpdateItemsV0Processor::UpdateItemsV0Processor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {
}

void SendSuccessfulUpdate(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::shared_ptr<json> resjson = packet.GetBaseJsonResponse();
	(*resjson)["payload"]["success"] = true;
	sock.SendPacket(resjson);
}

void UpdateItemsV0Processor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	sql::Statement invQuery = PlayerDatabase::Get().FormatStatement(
		"SELECT {col} from {table} WHERE PlayerId = ?",
		FieldKey::PLAYER_INVENTORY
	);
	invQuery.bind(1, sock.GetPlayerId());
	std::unique_ptr<Inventory> playerI = PlayerDatabase::Get().GetField<Inventory>(invQuery, FieldKey::PLAYER_INVENTORY);
	FullInventory* playerInv = playerI->mutable_full();
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
		if (itemUpdate.ext().setviewed()) {
			curItem->mutable_ext()->set_viewed(true);
		}
	}
	sql::Statement setStatement = PlayerDatabase::Get().FormatStatement(
		"INSERT OR REPLACE INTO {table} (PlayerId, {col}) VALUES (?, ?)",
		FieldKey::PLAYER_INVENTORY
	);
	setStatement.bind(1, sock.GetPlayerId());
	PlayerDatabase::Get().SetField(setStatement, FieldKey::PLAYER_INVENTORY, playerI.get(), 2);
	SendSuccessfulUpdate(packet, sock);
}