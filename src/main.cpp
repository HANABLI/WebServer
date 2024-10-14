/**
 * @file main.cpp
 * 
 * This module holds the main() function, which is the 
 * entrypoint to the webServer program.s
 */
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <signal.h>
#include <thread>
#include <stdio.h>
#include <map>
#include <string>
#include <chrono>
#include <memory>
#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/DirectoryMonitor.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/DiagnosticsStreamReporter.hpp>
#include <SystemUtils/DynamicLibrary.hpp>
#include <Http/Server.hpp>
#include <Json/Json.hpp>
#include <SystemUtils/File.hpp>
#include <HttpNetworkTransport/HttpServerNetworkTransport.hpp>

namespace {

    /**
     * This is the default port number on whish to listen for
     * connections from web client.
     */
    constexpr uint16_t DEFAULT_PORT = 8080;
    /**
     * This flag indicates whather or not the web server
     * should shut down.
     */
    bool shutDown = false;

    /**
     * This contains variables set throuth the operating system environment
     * or the command-line arguments
     */
    struct Environment {
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

    /**
     * This is the information tracked for each plug-in
     */
    struct Plugin {
        // Properties

        /**
         * This is the time that the plug-in image was last modified.
         */
        time_t lastModifiedTime = 0;

        /**
         * This is the plug-in image file.
         */
        SystemUtils::File pluginImageFile;

        /**
         * This is the plug-in runtime file.
         */
        SystemUtils::File pluginRuntimeFile;

        /**
         * This is the path to the plug-in runtime file,
         * without the file extension (if any)
         */
        std::string moduleName;

        /**
         * This is the configuration object to give to the plugin when
         * it's loaded.
         */
        std::shared_ptr< Json::Json > configuration;

        /**
         * This is used to dynamically link with the run-time copy
         * of the plug-in image.
         */
        SystemUtils::DynamicLibrary pluginRuntimeLibrary;

        /**
         * If the plug-in is currently loaded, this is the function to 
         * call in order to unload it.
         */
        std::function< void() > unloadDelegate;

        // Methods

        /**
         * This is the constructor for the structure.
         * 
         * @param[in] imageFileName
         *      This is the plug-in image file name.
         * 
         * @param[in] runtimeFileName
         *      This is the plug in runtime file name.
         */
        Plugin(const std::string& imageFileName, const std::string& runtimeFileName) : 
            pluginImageFile(imageFileName),
            pluginRuntimeFile(runtimeFileName) 
        {
        };

        /**
         * This method is used to cleanly unload the plug-in
         * folowing this eneral procedure:
         * 1. Call the "unload" delegate provided by the plug-in
         *    Typecally this delegate will turn around and revoke
         *    its registration with the web server.
         * 2. Release the unload delegate (by assigning nullptr
         *    to the variable holding onto it. Typecally this 
         *    will cause any state captured in the unload delegate
         *    to be destroyed/freed. After this it will be safe to
         *    unlink plug-in code).
         * 3. Unlik the plug-in code.
         */
        void Unload() {
            if (unloadDelegate != nullptr) {
                return;
            }
            unloadDelegate();
            unloadDelegate = nullptr;
            pluginRuntimeLibrary.Unload();
        }

        /**
         * This method cleanly load the plug-in, following the procedure
         * bellow.
         * 1.  Make a copy from the plugins-image folder to the plugins-run
         *     time folder
         * 2.  Link the plug-in code
         * 3.  Locate the plugin entrypoint function "LoadPlugin".
         * 4.  Call the entrypoint function, providing the plug-in with access
         *     to the server. Then the plugin will return a function that the
         *     server can call later to unload the plugin.
         * @note 
         *     The plugin can signal a "failure to load" or other
         *     kind of fatal error simply by leaving the "unload"
         *     delegate as a nullptr
         * 
         * @param[in] server
         *      This is the server for which to load the plugin
         * 
         * @param[in] pluginsRunTimePath
         *      This is the path to where a copy of the plugin is made
         *      in order to link the code without locking the original
         *      code image . So that the original code image can be updated
         *      even while the plugins is loaded.
         * 
         * @param[in] diagnosticMessageDelegate
         *      This is the function delegate to call to publish any diagnostic message.
         *      
         */
        void Load(
            Http::Server& server, 
            const std::string& pluginsRunTimePath, 
            SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate
        ) {
            if (
                pluginImageFile.Copy(pluginRuntimeFile.GetPath())
            ) {
                if (pluginRuntimeLibrary.Load(
                        pluginsRunTimePath,
                        moduleName
                    )
                ) {
                    const auto loadPlugin = (
                        void(*)(
                            Http::Server& server,
                            Json::Json configuration,
                            SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate,
                            std::function< void() >& unloadDelegate
                        )
                    )pluginRuntimeLibrary.GetProcedure("LoadPlugin");
                    if (loadPlugin != nullptr) {
                        loadPlugin(
                            server, 
                            *configuration,
                            [this, diagnosticMessageDelegate](
                                std::string senderName,
                                size_t level,
                                std::string message
                            ){
                                if (senderName.empty()) {
                                    diagnosticMessageDelegate(
                                        this->moduleName,
                                        level,
                                        message
                                    );
                                } else {
                                    diagnosticMessageDelegate(
                                        StringUtils::sprintf(
                                            "%s%s",
                                            this->moduleName.c_str(),
                                            senderName.c_str()
                                        ),
                                        level,
                                        message
                                    );
                                }
                            },
                            unloadDelegate
                        );
                        if (unloadDelegate == nullptr) {
                            diagnosticMessageDelegate(
                                "",
                                SystemUtils::DiagnosticsSender::Levels::WARNING,
                                StringUtils::sprintf(
                                    "unable to find plugin '%s' entrypoint",
                                    this->moduleName.c_str()
                                )
                            );
                        }
                    } else {
                        diagnosticMessageDelegate(
                            "",
                            SystemUtils::DiagnosticsSender::Levels::WARNING,
                            StringUtils::sprintf(
                                "unable to find plugin '%s' entrypoint",
                                this->moduleName.c_str()
                            )
                        );
                    }
                    if (unloadDelegate == nullptr) {
                        pluginRuntimeLibrary.Unload();
                    }
                } else {
                    diagnosticMessageDelegate(
                        "",
                        SystemUtils::DiagnosticsSender::Levels::WARNING,
                        StringUtils::sprintf(
                            "unable to link plugin '%s' library",
                            this->moduleName.c_str()
                        )
                    );
                }
                if (unloadDelegate == nullptr) {
                    pluginRuntimeFile.Destroy();
                }
            } else {
                diagnosticMessageDelegate(
                    "",
                    SystemUtils::DiagnosticsSender::Levels::WARNING,
                    StringUtils::sprintf(
                        "unable to find plugin '%s' entrypoint",
                        this->moduleName.c_str()
                    )
                );
            }
        }
    };
    
}


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
bool ProcessCommandLineArguments(
    int argc,
    char* argv[],
    Environment& environment
) {
    size_t state = 0;
    for( int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        switch (state)
        {
            case 0: { // next argument
                if ((arg == "-c") || (arg == "--config")) {
                    state = 1;
                } else {
                    fprintf(stderr, "error: unrecognized option: '%s'\n", arg.c_str());
                    return false;
                }
            } break;
            
            case 1: { // -c| --config
                if (!environment.configFilePath.empty()) {
                    fprintf(stderr, "error: multiple configuration file paths given\n");
                    return false;
                }
                environment.configFilePath = arg;
                state = 0;
            } break;
            default:
                break;
        }
    }
    switch (state)
    {
        case 1: {
            fprintf(stderr, "error: configuration file path expected\n");
        } return false;
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
Json::Json ReadConfiguration(const Environment& environment) {
    // Default configuration to be used when there are any issues
    // reading the actual configuration file.
    Json::Json configuration(Json::Json::Type::Object);
    configuration.Set("port", DEFAULT_PORT);

    // Open the configuration file.
    std::vector< std::string > possibleConfigPaths = {
        "config.json",
        SystemUtils::File::GetExeParentDirectory() + "/config.json",

    };
    if (!environment.configFilePath.empty()) {
        possibleConfigPaths.insert(
            possibleConfigPaths.begin(),
            environment.configFilePath
        );
    }
    std::shared_ptr< FILE > configFile;
    for (const auto& possibleConfigPath: possibleConfigPaths ) {
        configFile = std::shared_ptr< FILE >( 
            fopen(possibleConfigPath.c_str(), "rb"),
            [](FILE* f){
                if (f != NULL) {
                    (void)fclose(f);
                }
            }
        );
        if (configFile != NULL) {
            break;
        }
    }

    if (configFile == NULL) {
        fprintf(stderr, "error: unable to open the configuration file\n");
        return configuration;
    } 
    // Determine the size of the configuration file.
    if(fseek(configFile.get(), 0, SEEK_END) != 0) {
        fprintf(stderr, "error: unable to  seek to the end of the configuration file\n");
        return configuration;
    }
    const auto configSize = ftell(configFile.get());
    if (configSize == EOF) {
        fprintf(stderr, "error: unable to determine end of configuration file\n");
        return configuration;
    }

    if(fseek(configFile.get(), 0, SEEK_SET) != 0) {
        fprintf(stderr, "error: unable to seek to the beginning of the configuration file\n");
        return configuration; 
    }

    // Read the configuration file into memory.
    std::vector< char > encodedConfig(configSize + 1);
    if (fread(encodedConfig.data(), configSize, 1, configFile.get()) != 1) {
        fprintf(stderr, "error: unable to read the configuration file\n");
        return configuration;
    }

    // Decode the configuration file.
    configuration = Json::Json::FromEncoding(encodedConfig.data());
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
    std::shared_ptr< Http::ServerTransportLayer > transport,
    const Json::Json& configuration,
    const Environment& environment
) {
    uint16_t port = 0;
    if (configuration.Has("port")) {
        port = (int)*(configuration)["port"];
    } 
    if (port == 0) {
        port = DEFAULT_PORT;
    }
    if (!server.Mobilize(transport, port)) {
        return false;
    }
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
    Http::Server& server,
    const Json::Json& configuration,
    const Environment& environment,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate
    
) {
    SystemUtils::DirectoryMonitor directroyMonitor;
    std::string pluginsImagePath = environment.pluginsImagePath;
    if (configuration.Has("plugins-image")) {
        pluginsImagePath = *configuration["plugins-image"]; 
    }
    std::string pluginsRunTimePath = environment.runtimePluginPath;
    if (configuration.Has("plugins-runtime")) {
        pluginsRunTimePath = *configuration["plugins-runtime"];
    }
    std::map< std::string, std::shared_ptr< Plugin> > plugins;
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
    for (size_t i = 0; i < pluginsEnabled->GetSize(); ++i) {
        std::string pluginName = *(*pluginsEnabled)[i];
        if (pluginsEntries->Has(pluginName)) {
            const auto pluginEntry = (*pluginsEntries)[pluginName];
            const std::string pluginModule = *(*pluginEntry)["module"];
            const auto plugin = plugins[pluginName] = std::make_shared< Plugin>(
                pluginsImagePath + "/" + pluginModule + moduleExtension,
                pluginsRunTimePath + "/" + pluginModule + moduleExtension
            );
            plugin->moduleName = pluginModule;
            plugin->lastModifiedTime = plugin->pluginImageFile.GetLastModifiedTime();
            plugin->configuration = (*pluginEntry)["configuration"];
        }
    }
    const auto pluginScanDelegate = [&server, &plugins, configuration, diagnosticMessageDelegate, pluginsImagePath, pluginsRunTimePath]{
        for (auto& plugin: plugins) {
            if ( (plugin.second->unloadDelegate == nullptr) 
                && plugin.second->pluginImageFile.IsExisting()
            ) {
                plugin.second->Load(server, pluginsRunTimePath, diagnosticMessageDelegate);
            } else {
                diagnosticMessageDelegate(
                    "",
                    SystemUtils::DiagnosticsSender::Levels::WARNING,
                    StringUtils::sprintf(
                        "unable to find plugin image '%s' file",
                        plugin.first.c_str()
                    )
                );
            }
        }
    };
    pluginScanDelegate();
    if (
        !directroyMonitor.Start(pluginScanDelegate, pluginsImagePath)    
    ) {
        fprintf(stderr, "warning: unable to monitor plug-ins image directory (%s)\n", pluginsImagePath.c_str());
    }
    while (!shutDown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    directroyMonitor.Stop();
    for (auto& plugin: plugins) {
        plugin.second->Unload();
    }
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
    auto transport = std::make_shared< HttpNetworkTransport::HttpServerNetworkTransport >();
    const auto previousInterruptHandler = signal(SIGINT, InterruptHandler);
    Environment environment;
    if (!ProcessCommandLineArguments(argc, argv, environment)) {
        return EXIT_FAILURE;
    }
    Http::Server server;
    const auto diagnosticsPublisher = SystemUtils::DiagnosticsStreamReporter(stdout, stderr);
    const auto diagnosticsSubscription = server.SubscribeToDiagnostics(diagnosticsPublisher);
    const auto configuration = ReadConfiguration(environment);
    if (!ConfigureAndStartServer(server, transport, configuration, environment)) {
        return EXIT_FAILURE;
    }
    auto testLeak = new char[80];
    printf("Web server starting up.\n");
    MonitorServer(server, configuration, environment, diagnosticsPublisher);
    
    (void)signal(SIGINT, previousInterruptHandler);
    printf("Exiting ...\n");
    return EXIT_SUCCESS;
}
