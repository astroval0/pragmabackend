#include <UpdateItemV4Processor.h>
#include <Inventory.pb.h>
#include <PlayerDatabase.h>
#include <spdlog/spdlog.h>
#include <UpdateSingleItemMessage.pb.h>
#include <algorithm>
#include <cctype>

UpdateItemV4Processor::UpdateItemV4Processor(SpectreRpcType rpcType) :
	WebsocketPacketProcessor(rpcType) {

}

static void SendSuccessfulUpdate(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	std::shared_ptr<json> resjson = packet.GetBaseJsonResponse();
	(*resjson)["payload"]["success"] = true;
	sock.SendPacket(resjson);
}

void UpdateItemV4Processor::Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) {
	sql::Statement invQuery = PlayerDatabase::Get().FormatStatement(
		"SELECT {col} from {table} WHERE PlayerId = ?",
		FieldKey::PLAYER_INVENTORY
	);
	invQuery.bind(1, sock.GetPlayerId());
	std::unique_ptr<Inventory> playerI = PlayerDatabase::Get().GetField<Inventory>(invQuery, FieldKey::PLAYER_INVENTORY);
	std::unique_ptr<UpdateSingleItemMessage> itemUpdate = packet.GetPayloadAsMessage<UpdateSingleItemMessage>();
	FullInventory* playerInv = playerI->mutable_full();
	if (itemUpdate->has_instanceditemupdate()) {
		InstancedItem* curItem = nullptr;
		for (int i = 0; i < playerInv->instanced_size(); i++) {
			if (playerInv->instanced(i).instanceid() == itemUpdate->instanceditemupdate().instanceid()) {
				curItem = playerInv->mutable_instanced(i);
				break;
			}
		}
		if (!curItem) {
			spdlog::warn("Couldn't find item with instance id {} in a item update request, skipping", itemUpdate->instanceditemupdate().instanceid());
			SendSuccessfulUpdate(packet, sock);
			return;
		}
		if (itemUpdate->instanceditemupdate().ext().setviewed()) {
			curItem->mutable_ext()->set_viewed(true);
		}
		if (itemUpdate->instanceditemupdate().ext().has_progressiontrackerupdate()) {
			curItem->mutable_ext()->mutable_trackedprogression()->set_activeendorsement(
				itemUpdate->instanceditemupdate().ext().progressiontrackerupdate().newactiveendorsement()
			);
		}
	}
	if (itemUpdate->has_stackeditemupdate()) {
		StackableItem* curItem = nullptr;
		for (int i = 0; i < playerInv->stackables_size(); i++) {
			if (playerInv->stackables(i).instanceid() == itemUpdate->stackeditemupdate().instanceid()) {
				curItem = playerInv->mutable_stackables(i);
				break;
			}
		}
		if (!curItem) {
			spdlog::warn("Couldn't find item with instance id {} in a item update request, skipping", itemUpdate->stackeditemupdate().instanceid());
			SendSuccessfulUpdate(packet, sock);
			return;
		}
		curItem->set_amount(itemUpdate->stackeditemupdate().newamount());
	}
	sql::Statement setStatement = PlayerDatabase::Get().FormatStatement(
		"INSERT OR REPLACE INTO {table} (PlayerId, {col}) VALUES (?, ?)",
		FieldKey::PLAYER_INVENTORY
	);
	setStatement.bind(1, sock.GetPlayerId());
	PlayerDatabase::Get().SetField(setStatement, FieldKey::PLAYER_INVENTORY, playerI.get(), 2);
	SendSuccessfulUpdate(packet, sock);
}