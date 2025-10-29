#pragma once
#include "Database.h"

class PlayerDatabase : public Database {
private:
	static PlayerDatabase inst;
public:
	static PlayerDatabase& Get();
	PlayerDatabase(fs::path path);
};