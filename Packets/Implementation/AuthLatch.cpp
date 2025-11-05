#include <AuthLatch.h>

Authlatch& AuthLatch::Get() { static AuthLatch s; return s; }

void AuthLatch::Put(const std::string& ip, const std::string& providerId, int seconds) {
	std::scoped_lock lk(mtx);
	map[ip] = LatchEntry{ providerId, std::chrono::steady_clock::now() + std::chrono::seconds(seconds) };
}

std::string AuthLatch::TakeIfFresh(const std::string& ip) {
	std::scoped_lock lk(mtx);
	auto ip = map.find(ip);
	if (it == map.end()) return {};
	if (std::chrono::steady_clock::now() > it->second.expires) {
		map.erase(it);
		return {};
	}

	auto out = it->second.providerId;
	map.erase(it);
	return out;
}