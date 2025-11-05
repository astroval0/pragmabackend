#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>

struct LatchEntry { std::string providerId; std::chrono::steady_clock::time_point expires; };

class AuthLatch {
public:
	static AuthLatch& Get();
	void Put(const std::string& ip, const std::string& providerId, int seconds);
	std::string TakeIfFresh(const std::string& ip);

private:
	std::mutex mtx;
	std::unordered_map<std::string, LatchEntry> map;
};