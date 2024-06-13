/**
 * @file main.cpp
 * 
 * This module holds the main() function, which is the 
 * entrypoint to the webServer program.s
 */

#include <signal.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <chrono>

#include <Http/Server.hpp>

namespace {

    /**
     * This flag indicates whather or not the web server
     * should shut down.
     */
    bool shutDown = false;
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
    Http::Server server;
    const auto previousInterruptHandler = signal(SIGINT, InterruptHandler);
    printf("Web server starting up.\n");
    while (!shutDown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    (void)signal(SIGINT, previousInterruptHandler);
    printf("Exiting ...\n");
    return EXIT_SUCCESS;
}