#pragma once
#include <regex>
#include <string>

namespace betterrx {
	struct regex {
	public:
		std::string m_pattern;
		std::regex m_rx;

		regex(const std::string& pattern) : m_pattern(pattern), m_rx(pattern) {};
		bool operator==(const regex& other) const {
			return m_pattern == other.m_pattern;
		}
		operator std::regex() const {
			return m_rx;
		}
	};
}
using regex = betterrx::regex;

namespace std {
	template <>
	struct hash<betterrx::regex> {
		std::size_t operator()(const betterrx::regex& regex) const {
			return std::hash<std::string>{}(regex.m_pattern);
		}
	};
}