#include <PartyDatabase.h>

PartyDatabase::PartyDatabase(fs::path path) : Database(path, "parties", "PartyID", "TEXT") {
	sql::Statement colQuery(GetRawRef(), "PRAGMA table_info(" + GetTableName() + ");");
	bool colExists = false;
	while (colQuery.executeStep()) {
		std::string colName = colQuery.getColumn(1).getText();
		if (colName == "PartyCode") {
			colExists = true;
			break;
		}
	}
	if (!colExists) {
		GetRaw()->exec("ALTER TABLE " + GetTableName() + " ADD COLUMN " + "PartyCode" + " BLOB;");
	}
}

PartyDatabase PartyDatabase::inst("playerdata.sqlite");

PartyDatabase& PartyDatabase::Get() {
	return inst;
}