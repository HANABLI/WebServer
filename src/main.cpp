/**
 * @file main.cpp
 *
 * This module holds the main() function, which is the
 * entrypoint to the webServer program.
 *
 * © 2024 by Hatem Nabli
 */
#define _CRTDBG_MAP_ALLOC
#ifdef _WIN32
#    include <crtdbg.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <Http/Server.hpp>
#include <HttpNetworkTransport/HttpServerNetworkTransport.hpp>
#include <Json/Json.hpp>
#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/DiagnosticsStreamReporter.hpp>
#include <SystemUtils/DirectoryMonitor.hpp>
#include <SystemUtils/DynamicLibrary.hpp>
#include <SystemUtils/File.hpp>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "PluginLoader.hpp"
#include "TimeKeeper.hpp"

namespace
{
    /**
     * This flag indicates whather or not the web server
     * should shut down.
     */
    bool shutDown = false;

    /**
     * This contains variables set throuth the operating system environment
     * or the command-line arguments
     */
    struct Environment
    {
        /**
         * This is the path to the configuration file to use to configure
         * the server.
         */
        std::string configFilePath;

        /**
         * This is the path to the folder which will be monitored
         * for plug-ins to be added, changed, or removed.
         * These are the "originals" of the plug-ins, and will be
         * copied to another folder before  loading them.
         */
        std::string pluginsImagePath = SystemUtils::File::GetExeParentDirectory();

        /**
         * This is the path to the folder where copies of all
         * plug-ins to be loaded will be made.
         */
        std::string runtimePluginPath = SystemUtils::File::GetExeParentDirectory() + "/runtime";
    };
}  // namespace

/**
 * This function updates the program environement to incorporate
 * an applicable command-line arguments.
 *
 * @param[in] argc
 *      This is the number of command-line arguments to the program.
 *
 * @param[in] argv
 *      This is the array of command line arguments given to the program.
 *
 * @param[in] environement
 *      This is the environement to update.
 */
bool ProcessCommandLineArguments(int argc, char* argv[], Environment& environment) {
    size_t state = 0;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg(argv[i]);
        switch (state)
        {
        case 0: {  // next argument
            if ((arg == "-c") || (arg == "--config"))
            {
                state = 1;
            } else
            {
                fprintf(stderr, "error: unrecognized option: '%s'\n", arg.c_str());
                return false;
            }
        }
        break;

        case 1: {  // -c| --config
            if (!environment.configFilePath.empty())
            {
                fprintf(stderr, "error: multiple configuration file paths given\n");
                return false;
            }
            environment.configFilePath = arg;
            state = 0;
        }
        break;
        default:
            break;
        }
    }
    switch (state)
    {
    case 1: {
        fprintf(stderr, "error: configuration file path expected\n");
    }
        return false;
    }
    return true;
}

/**
 * This function is set up to ba called whene the SIGINT signal is
 * received by the main program, It sets the "shutDown" flag to true
 * and relies on the program to be polling the flag to detect when
 * it's been set.
 *
 * @param[in]
 *      This is the signal for which this function was called.
 */
void InterruptHandler(int sig) {
    shutDown = true;
}

/**
 * This function opens and reads the server's configuration file,
 * returning it. The configuration is formatted as a JSON object.
 *
 * @param[in] environment
 *      This contains variables set through the operating system
 *      environment or the command-line arguments.
 *
 * @return
 *      The server's configuration is returned as a JSON obejct.
 */
Json::Value ReadConfiguration(const Environment& environment) {
    // Default configuration to be used when there are any issues
    // reading the actual configuration file.
    Json::Value configuration(Json::Value::Type::Object);

    // Open the configuration file.
    std::vector<std::string> possibleConfigPaths = {
        "config.json",
        SystemUtils::File::GetExeParentDirectory() + "/config.json",

    };
    if (!environment.configFilePath.empty())
    { possibleConfigPaths.insert(possibleConfigPaths.begin(), environment.configFilePath); }
    std::shared_ptr<FILE> configFile;
    for (const auto& possibleConfigPath : possibleConfigPaths)
    {
        configFile = std::shared_ptr<FILE>(fopen(possibleConfigPath.c_str(), "rb"),
                                           [](FILE* f)
                                           {
                                               if (f != NULL)
                                               { (void)fclose(f); }
                                           });
        if (configFile != NULL)
        { break; }
    }

    if (configFile == NULL)
    {
        fprintf(stderr, "error: unable to open the configuration file\n");
        return configuration;
    }
    // Determine the size of the configuration file.
    if (fseek(configFile.get(), 0, SEEK_END) != 0)
    {
        fprintf(stderr, "error: unable to  seek to the end of the configuration file\n");
        return configuration;
    }
    const auto configSize = ftell(configFile.get());
    if (configSize == EOF)
    {
        fprintf(stderr, "error: unable to determine end of configuration file\n");
        return configuration;
    }

    if (fseek(configFile.get(), 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "error: unable to seek to the beginning of the configuration file\n");
        return configuration;
    }

    // Read the configuration file into memory.
    std::vector<char> encodedConfig(configSize + 1);
    if (fread(encodedConfig.data(), configSize, 1, configFile.get()) != 1)
    {
        fprintf(stderr, "error: unable to read the configuration file\n");
        return configuration;
    }

    // Decode the configuration file.
    configuration = Json::Value::FromEncoding(encodedConfig.data());
    return configuration;
}

/**
 * This function assembles the configuration of the server, and uses it
 * to start server with the given transport layer.
 *
 * @param[in,out] server
 *      This is the server to configure and start.
 *
 * @param[in] transport
 *      This is the transport layer to give to the server for interfacing
 *      with the network.
 *
 * @param[in] environment
 *      This contains variables set through the operating system
 *      environment or the command-line arguments.
 *
 * @return
 *      An indication of whether or not the function succeeded is returned.
 */
bool ConfigureAndStartServer(
    Http::Server& server,
    std::shared_ptr<HttpNetworkTransport::HttpServerNetworkTransport> transport,
    const Json::Value& configuration, const Environment& environment) {
    Http::Server::MobilizationDependencies deps;
    auto timeKeeper = std::make_shared<TimeKeeper>();
    deps.transport = transport;
    deps.timeKeeper = timeKeeper;
    for (const auto& key : configuration["server"].GetKeys())
    { server.SetConfigurationItem(key, configuration["server"][key]); }
    if (!server.Mobilize(deps))
    { return false; }
    return true;
}

/**
 * This is the function to call to monitor the server.
 *
 * @param[in, out] server
 *      This is the server to monitor.
 *
 * @param[in] configuration
 *      This holds all of the server's configuration items.
 *
 * @param[in] environment
 *      This contains variables set through the os environment
 *      or the command line arguments.
 *
 * @param[in] diagnosticMessageDelegate
 *      This is the function to call to publish any diagnostic
 *      messages.
 */
void MonitorServer(
    Http::Server& server, const Json::Value& configuration, const Environment& environment,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate) {
    std::string pluginsImagePath = environment.pluginsImagePath;
    if (configuration.Has("plugins-image"))
    { pluginsImagePath = static_cast<std::string>(configuration["plugins-image"]); }
    std::string pluginsRunTimePath = environment.runtimePluginPath;
    if (configuration.Has("plugins-runtime"))
    { pluginsRunTimePath = static_cast<std::string>(configuration["plugins-runtime"]); }
    std::map<std::string, std::shared_ptr<Plugin>> plugins;
    const auto pluginsEntries = configuration["plugins"];
    const auto pluginsEnabled = configuration["plugins-enabled"];
    std::string moduleExtension;
#if _WIN32
    moduleExtension = ".dll";
#elif defined(APPLE)
    moduleExtension = ".dylib";
#else
    moduleExtension = ".so";
#endif
    for (size_t i = 0; i < pluginsEnabled.GetSize(); ++i)
    {
        std::string pluginName = (pluginsEnabled)[i];
        if (pluginsEntries.Has(pluginName))
        {
            const auto pluginEntry = (pluginsEntries)[pluginName];
            const std::string pluginModule = (pluginEntry)["module"];
            auto plugin = plugins[pluginName] =
                std::make_shared<Plugin>(pluginsImagePath + "/" + pluginModule + moduleExtension,
                                         pluginsRunTimePath + "/" + pluginModule + moduleExtension);
            plugin->moduleName = pluginModule;
            plugin->lastModifiedTime = plugin->pluginImageFile.GetLastModifiedTime();
            plugin->configuration = (pluginEntry)["configuration"];
        }
    }
    PluginLoader pluginLoader(server, pluginsRunTimePath, pluginsImagePath, plugins,
                              diagnosticMessageDelegate);
    pluginLoader.Scan();
    pluginLoader.StartScanning();
    while (!shutDown)
    { std::this_thread::sleep_for(std::chrono::milliseconds(250)); }
    pluginLoader.StopScanning();
    for (auto& plugin : plugins)
    { plugin.second->Unload(diagnosticMessageDelegate); }
}

/**
 * This function is the entrypoint for the program.
 * It sts up the web server and then waits for the SIGINT
 * signal to shut down and terminate the program.
 *
 * @param[in] argc
 *      This is the number of command-line arguments given to the program.
 *
 * @param[in] argv
 *      This is the array of command-line arguments given to the program.
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    //_crtBreakAlloc = 1708;
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif /* _WIN32 */
    auto transport = std::make_shared<HttpNetworkTransport::HttpServerNetworkTransport>();
    const auto previousInterruptHandler = signal(SIGINT, InterruptHandler);
    Environment environment;
    if (!ProcessCommandLineArguments(argc, argv, environment))
    { return EXIT_FAILURE; }
    Http::Server server;
    const auto diagnosticsPublisher = SystemUtils::DiagnosticsStreamReporter(stdout, stderr);
    const auto diagnosticsSubscription = server.SubscribeToDiagnostics(diagnosticsPublisher);
    const auto configuration = ReadConfiguration(environment);
    if (!ConfigureAndStartServer(server, transport, configuration, environment))
    { return EXIT_FAILURE; }
    auto testLeak = new char[80];
    printf("Web server starting up.\n");
    MonitorServer(server, configuration, environment, diagnosticsPublisher);

    (void)signal(SIGINT, previousInterruptHandler);
    printf("Exiting ...\n");
    return EXIT_SUCCESS;
}
