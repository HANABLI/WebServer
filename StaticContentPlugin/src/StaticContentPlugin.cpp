/**
 * @file StaticContentPlugin.cpp
 * 
 * This is a plug-in for the Excalibur web server, designed
 * to serve static content (e.g. files) in response to resource requests.
 * 
 * @ 2024 by Hatem Nabli
 */
#include <memory>
#include <functional>
#include <StringUtils/StringUtils.hpp>
#include <Http/Server.hpp>
#include <Json/Json.hpp>

#ifdef _WIN32
#define API __declspec(dllexport)
#else /* POSIX */
#define API
#endif /* _WIN32 / POSIX */

extern "C" API void LoadPlugin(
    Http::Server& server,
    Json::Json configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate,
    std::function< void() >& unloadDelegate
) {
    Uri::Uri uri;
    if (!configuration.Has("space")) {
        diagnosticMessageDelegate(
            "",
            SystemUtils::DiagnosticsSender::Levels::ERROR,
            "no 'space' Uri in the configuration" 
        );
        return;
    }
    if (!uri.ParseFromString(*configuration["space"])) {
        diagnosticMessageDelegate(
            "",
            SystemUtils::DiagnosticsSender::Levels::ERROR,
            "unable to parse 'space' uri"
        );
        return;
    }
    auto space = uri.GetPath();
    (void)space.erase(space.begin());
    const auto unregistrationDelegate = server.RegisterResource(
        space,
        [](
            std::shared_ptr< Http::Server::Request > request
        ){
            const auto response = std::make_shared< Http::Client::Response >();
            response->statusCode = 200;
            response->status = "OK";
            response->headers.AddHeader("Content-Type", "text/plain");
            response->body = "Coming soon...!";
            response->headers.AddHeader("Content-Length", StringUtils::sprintf("%zu", response->body.size()));  
            return response;
        }
    );
    
    unloadDelegate = [unregistrationDelegate]{
        unregistrationDelegate();
    };
}
