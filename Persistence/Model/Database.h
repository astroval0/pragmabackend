#include <filesystem>
#include <unordered_map>
#include <vector>
#include <SQLiteCpp/SQLiteCpp.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "FieldKey.h"

namespace fs = std::filesystem;
namespace pbuf = google::protobuf;
namespace sql = SQLite;

class Database {
private:
	fs::path m_filename;
	sql::Database m_dbRaw;
	static std::unordered_map<FieldKey, const pbuf::Descriptor*> protoTypes;
	static std::unordered_map<FieldKey, const char*> classNames;
public:
	Database(fs::path dbPath);
	sql::Database* GetRaw();
	std::unique_ptr<pbuf::Message> CreateObjectOfFieldType(FieldKey key);
	std::vector<std::unique_ptr<pbuf::Message>> GetFields(sql::Statement& statement, FieldKey key);
	std::unique_ptr<pbuf::Message> GetField(sql::Statement& statement, FieldKey key);
	sql::Statement FormatStatement(std::string command, FieldKey key);
	void AddPrototype(FieldKey key, const char* className);
	bool IsFieldPopulated(FieldKey key);
	void SetField(sql::Statement& statement, FieldKey key, const pbuf::Message* object);
	const char* GetFieldName(FieldKey key);
	virtual const std::string GetTableName() = 0;
};