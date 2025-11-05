#include <PlayerDatabase.h>
#include <Database.h>

static void CheckProviderTable(SQLite::Database& db) {
	db.exec(
		"CREATE TABLE IF NOT EXISTS providers ("
		"provider TEXT NOT NULL,"
		"provider_id TEXT NOT NULL,"
		"player_id TEXT NOT NULL,"
		"PRIMARY KEY(provider, provider_id));"
	);
	db.exec("CREATE INDEX IF NOT EXISTS providers_player_idx ON providers(player_id);");
}

std::string PlayerDatabase::LookupPlayerByProvider(const std::string& provider, const std::string& providerId) {
	auto* raw = GetRaw();
	CheckProviderTable(*raw);
	SQLite::Statement query(*raw, "SELECT player_id FROM providers WHERE provider = ? AND provider_id = ?;");
	query.bind(1, provider);
	query.bind(2, providerId);
	if (query.executeStep()) {
		return query.getColumn(0).getString();
	}
	return {};
}

void PlayerDatabase::UpsertProviderMap(const std::string& provider, const std::string& providerId, const std::string& playerId) {
	auto* raw = GetRaw();
	CheckProviderTable(*raw);
	SQLite::Statement query(*raw, "INSERT OR REPLACE INTO providers(provider, provider_id, player_id) VALUES(?,?,?);");
	query.bind(1, provider);
	query.bind(2, providerId);
	query.bind(3, playerId);
	query.exec();
}

bool PlayerDatabase::IsBanned(const std::string& playerId) {
	auto* raw = GetRaw();
	raw->exec("CREATE TABLE IF NOT EXISTS bans(player_id TEXT PRIMARY KEY);");
	SQLite::Statement query(*raw, "SELECT banned_until FROM bans WHERE player_id=?;");
	query.bind(1, playerId);
	if (!query.executeStep()) return false;
	const auto until = query.getColumn(0).getInt64();
	if (until == 0) return true;
	const auto now = static_cast<long long>(time(nullptr));
	return now < (until / 1000);
}