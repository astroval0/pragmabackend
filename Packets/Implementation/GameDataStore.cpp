#include <GameDataStore.h>
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace pbu = google::protobuf::util;

GameDataStore GameDataStore::inst("resources/payloads/ws/game/DefaultInventoryStore.json");

static std::string InventoryStoreToPayload(InventoryContent* invStore) {
	std::string jsonstr;
	std::string jsonstr2;
	pbu::JsonPrintOptions popts;
	popts.always_print_fields_with_no_presence = true;
	auto status = pbu::MessageToJsonString(*invStore, &jsonstr, popts);
	if (!status.ok()) {
		spdlog::error("Failed to serialize InventoryContent to json string: {}", status.message());
		throw;
	}
	size_t curPos = 0;
	curPos = jsonstr.find("\"contentId\":", curPos);
	size_t lastPos = 0;

	while (curPos != std::string::npos) {
		curPos--;
		char curChar = jsonstr[curPos];
		jsonstr2 += std::string(jsonstr.begin() + lastPos, jsonstr.begin() + curPos);
		jsonstr2 += '\"';
		while (curChar != '}') {
			if (curChar == '\"') {
				jsonstr2 += "\\\"";
			}
			else {
				jsonstr2 += curChar;
			}
			curPos++;
			curChar = jsonstr[curPos];
		}
		jsonstr2 += curChar;
		jsonstr2 += '\"';
		curPos++;
		lastPos = curPos;
		curPos = jsonstr.find("\"contentId\":", curPos);
	}
	jsonstr2 += std::string(jsonstr.begin() + lastPos, jsonstr.end());
	curPos = jsonstr2.find("\"gameData\":{}");
	while (curPos != std::string::npos) {
		curPos += 11;
		jsonstr2[curPos] = '\"';
		jsonstr2[curPos + 1] = '\"';
		curPos = jsonstr2.find("\"gameData\":{}");
	}
	curPos = jsonstr2.find("\"gameData\":\"{\\\"contentId\\\":\\\"\\\"");
	while (curPos != std::string::npos) {
		jsonstr2.replace(curPos, 170, "\"{}\"");
		curPos = jsonstr2.find("\"gameData\":\"{\\\"contentId\\\":\\\"\\\"");
	}
	return jsonstr2;
}

void GameDataStore::RefreshInventoryStoreCache(InventoryContent* invStore) {
	inventoryStore_bufCache = InventoryStoreToPayload(invStore);
	inventoryStoreLock.unlock();
}

GameDataStore::GameDataStore(std::string inventoryStorePath) {
	std::ifstream invStoreFile(inventoryStorePath);
	if (!invStoreFile.is_open()) {
		spdlog::error("failed to open InventoryStore file");
		throw;
	}
	std::stringstream ss;
	ss << invStoreFile.rdbuf();
	auto status = pbu::JsonStringToMessage(ss.str(), &inventoryStore);
	if (!status.ok()) {
		spdlog::error("failed to initialize InventoryStore: {}", status.message());
		throw;
	}
	inventoryStore_bufCache = InventoryStoreToPayload(&inventoryStore);
}

std::unique_ptr<InventoryContent, std::function<void(InventoryContent*)>> GameDataStore::InventoryStore_mut() {
	// When the returned pointer goes out of scope, call the RefreshInventoryStoreCache method in the class
	while(!inventoryStoreLock.try_lock()){}
	return std::unique_ptr<InventoryContent, std::function<void(InventoryContent*)>>(
		&inventoryStore,
		// Turns class method into static lambda
		std::bind(&GameDataStore::RefreshInventoryStoreCache, this, std::placeholders::_1)
	);
}

std::unique_ptr<InventoryContent, std::function<void(InventoryContent*)>> GameDataStore::InventoryStore() {
	while (!inventoryStoreLock.try_lock()) {}
	return std::unique_ptr<InventoryContent, std::function<void(InventoryContent*)>>(
		&inventoryStore,
		std::bind(&GameDataStore::UnlockInventoryStore, this, std::placeholders::_1)
	);
}

std::unique_ptr<std::string, std::function<void(std::string*)>> GameDataStore::InventoryStore_buf() {
	while (!inventoryStoreLock.try_lock());
	return std::unique_ptr<std::string, std::function<void(std::string*)>>(
		&inventoryStore_bufCache,
		std::bind(&GameDataStore::UnlockInventoryStore2, this, std::placeholders::_1)
	);
}

GameDataStore& GameDataStore::Get() {
	return inst;
}

void GameDataStore::UnlockInventoryStore2(std::string* unused) {
	inventoryStoreLock.unlock();
}

void GameDataStore::UnlockInventoryStore(InventoryContent* unused) {
	inventoryStoreLock.unlock();
}