#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace fs = std::filesystem;

#pragma warning(push) // disable msvc's complaining about us not saving the processors in vars, they'll be cleaned up when our program ends.
#pragma warning(disable: 4101)
void RegisterStaticHTTPHandlerFromFile(std::string route, std::string filename) {
    std::ifstream fileres(filename);
    if (!fileres.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    json res = json::parse(fileres);
    fileres.close();
    spdlog::info("registered static HTTP route {} from json file at {}", route, filename);
    new StaticResponseProcessorHTTP(route, std::make_shared<json>(std::move(res)));
}
#pragma warning(pop)

void RegisterStaticHTTPHandlers() {
    for (const auto& file : fs::recursive_directory_iterator("resources/payloads/static/game")) {
        if (!fs::is_regular_file(file)) continue;
        std::string route = (fs::absolute(file.path().parent_path()) / file.path().stem()).string();
        std::string prefixString = fs::absolute("resources/payloads/static/game").string();
        route.erase(route.find(prefixString), prefixString.size());
        std::replace(route.begin(), route.end(), '\\', '/');
        RegisterStaticHTTPHandlerFromFile(route, file.path().string());
    }
    for (const auto& file : fs::recursive_directory_iterator("resources/payloads/static/social")) {
        if (!fs::is_regular_file(file)) continue;
        std::string route = (fs::absolute(file.path().parent_path()) / file.path().stem()).string();
        std::string prefixString = fs::absolute("resources/payloads/static/social").string();
        route.erase(route.find(prefixString), prefixString.size());
        std::replace(route.begin(), route.end(), '\\', '/');
        RegisterStaticHTTPHandlerFromFile(route, file.path().string());
    }
}