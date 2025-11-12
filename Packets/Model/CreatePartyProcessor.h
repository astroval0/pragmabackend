#pragma once
#include <PacketProcessor.h>
#include <stduuid/uuid.h>

class CreatePartyProcessor : public WebsocketPacketProcessor {
private:
	uuids::uuid_random_generator uuidgen;
	std::mt19937 stdrandgen;
	std::string GetRandomUUIDAsString();
	std::string GetNewInviteCode();
public:
	CreatePartyProcessor(SpectreRpcType rpcType);
	void Process(SpectreWebsocketRequest& packet, SpectreWebsocket& sock) override;
};