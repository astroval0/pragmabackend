#include "Database.h"

class PlayerDatabase : public Database {
public:
	PlayerDatabase(fs::path path);
	const std::string GetTableName() override;
};