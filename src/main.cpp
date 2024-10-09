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
#include <string>
#include <chrono>
#include <memory>
#include <SystemUtils/DiagnosticsStreamReporter.hpp>
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
    std::vector< char > encodedConfig(configSize);
    if (fread(encodedConfig.data(), encodedConfig.size(), 1, configFile.get()) != 1) {
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
    const Environment& environment
) {
    const auto configuration = ReadConfiguration(environment);
    uint16_t port = 0;
    if (configuration.Has("port")) {
        port = (int)*configuration["port"];
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
        const auto diagnosticsSubscription = server.SubscribeToDiagnostics(
        SystemUtils::DiagnosticsStreamReporter(stdout, stderr)
    );
    if (!ConfigureAndStartServer(server, transport, environment)) {
        return EXIT_FAILURE;
    }
    auto testLeak = new char[80];
    printf("Web server starting up.\n");
    while (!shutDown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    (void)signal(SIGINT, previousInterruptHandler);
    printf("Exiting ...\n");
    return EXIT_SUCCESS;
}
