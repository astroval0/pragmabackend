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
    SQLite::Statement query(*raw, "SELECT player_id FROM providers WHERE provider=? AND provider_id=?;");
    query.bind(1, provider);
    query.bind(2, providerId);
    if (query.executeStep()) return query.getColumn(0).getString();
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

static void CheckBansSchema(SQLite::Database& db) {
    db.exec("CREATE TABLE IF NOT EXISTS bans("
        "player_id TEXT PRIMARY KEY,"
        "reason TEXT,"
        "banned_until INTEGER DEFAULT 0)"); 
    SQLite::Statement probe(db, "SELECT 1 FROM pragma_table_info('bans') WHERE name='banned_until';");
    if (!probe.executeStep()) {
        db.exec("ALTER TABLE bans ADD COLUMN banned_until INTEGER DEFAULT 0;");
        db.exec("UPDATE bans SET banned_until=0 WHERE banned_until IS NULL;");
    }
}

bool PlayerDatabase::IsBanned(const std::string& playerId) {
    auto* raw = GetRaw();
    CheckBansSchema(*raw);
    SQLite::Statement query(*raw, "SELECT banned_until FROM bans WHERE player_id=?;");
    query.bind(1, playerId);
    if (!query.executeStep()) return false; 
    long long until_ms = 0;
    if (!query.getColumn(0).isNull()) {
        until_ms = query.getColumn(0).getInt64();
    }

    if (until_ms == 0) return true; 
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return now_ms < until_ms; 
}