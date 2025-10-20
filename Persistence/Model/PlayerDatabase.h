#include "Database.h"

class PlayerDatabase : public Database {
public:
	PlayerDatabase(fs::path path) : Database(path) {};
	void OnDbLoad() override;
	const char* GetTableName() override;
};