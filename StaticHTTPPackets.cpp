#include <fstream>

#pragma warning(push) // disable msvc's complaining about us not saving the processors in vars, they'll be cleaned up when our program ends.
#pragma warning(disable: 4101)
void RegisterStaticHTTPHandlerFromFile(std::string route, std::string filename) {
    std::ifstream fileres(filename);
    if (!fileres.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    json res = json::parse(fileres);
    fileres.close();
    new StaticResponseProcessorHTTP(route, std::make_shared<json>(res));
}
#pragma warning(pop)

void RegisterStaticHTTPHandlers() {
    RegisterStaticHTTPHandlerFromFile("/v1/info", "responses/info.json");
    RegisterStaticHTTPHandlerFromFile("/v1/spectre/healthcheck-status", "responses/healthcheck-status.json");
    RegisterStaticHTTPHandlerFromFile("/v1/healthcheck", "responses/healthcheck.json");
    RegisterStaticHTTPHandlerFromFile("/v1/loginqueue/getinqueuev1", "responses/getinqueuev1.json");
    RegisterStaticHTTPHandlerFromFile("/v1/account/authenticateorcreatev2", "responses/auth.json");
    RegisterStaticHTTPHandlerFromFile("/v1/gateway", "responses/gateway.json");
    RegisterStaticHTTPHandlerFromFile("/v1/types", "responses/types.json");
}