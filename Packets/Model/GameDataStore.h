#pragma once
#include <LoginDataMessage.pb.h>
#include <mutex>

class GameDataStore {
private:
	InventoryContent inventoryStore;
	std::string inventoryStore_bufCache;
	static GameDataStore inst;
	void RefreshInventoryStoreCache(InventoryContent* invStore);
	void UnlockInventoryStore2(std::string* unused);
	void UnlockInventoryStore(InventoryContent* unused);
	std::mutex inventoryStoreLock;
public:
	GameDataStore(std::string inventoryStoreFilePath);
	static GameDataStore& Get();
	/**
	* Gets the list of all items, crafting entries, .etc that are available in the game
	* Thread-safe
	* Do not store this reference, as it will lock any other writes / reads until it is complete.
	*/
	std::unique_ptr<InventoryContent, std::function<void(InventoryContent*)>> InventoryStore();
	/**
	* Do not use this method unless you intend to make changes to the InventoryStore because the cache
	* for it will be recalculated after the returned pointer goes out of scope. Instead use InventoryStore()
	* for simply reading the data.
	* If InventoryStore_buf or InventoryStore is called before the returned pointer of this method goes out of scope in the same thread, will hang forever :D
	* Do not store this pointer
	*/
	std::unique_ptr<InventoryContent, std::function<void(InventoryContent*)>> InventoryStore_mut();
	/**
	* Get a reference to the InventoryStore as a string payload, this cached and should be used
	* when sending requests like GetLoginDataV3 as it may be expensive to serialize a 60k line json file over and over.
	* Should make a COPY of the buffer from the reference, do not use it with something that could call memcpy on it and overwrite it
	* Do not store this pointer for longer than you absolutely need it to avoid unnecessary blocking of other threads.
	* Thread-safe
	*/
	std::unique_ptr<std::string, std::function<void(std::string*)>> InventoryStore_buf();
};