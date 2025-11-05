#pragma once
#include "Database.h"

class PartyDatabase : public Database {
private:
	static PartyDatabase inst;
public:
	static PartyDatabase& Get();
	PartyDatabase(fs::path path);
};