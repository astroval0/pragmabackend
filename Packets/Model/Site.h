#pragma once
enum class Site { Game, Social, Unknown };
inline const char* SiteKey(Site s) { return s == Site::Game ? "game" : (s == Site::Social ? "social" : "unknown"); }