#pragma once
#include "Database.h"

class PlayerDatabase : public Database {
private:
	static PlayerDatabase inst;
public:
	static PlayerDatabase& Get();
	PlayerDatabase(fs::path path);
	std::string LookupPlayerByProvider(const std::string& provider, const std::string& providerId);
	void UpsertProviderMap(const std::string& provider, const std::string& providerId, const std::string& playerId);
	bool IsBanned(const std::string& playerId);
};