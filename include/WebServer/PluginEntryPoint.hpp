#ifndef WEB_SERVER_PLUGIN_ENTRY_POINT_HPP
#define WEB_SERVER_PLUGIN_ENTRY_POINT_HPP

/**
 * @file PluginEntryPoint.hpp
 * 
 * This module declares the PluginEntryPoint type
 * 
 * © 2024 by Hatem Nabli
 */

#include <Http/Server.hpp>
#include <Json/Json.hpp>
#include <functional>
#include <SystemUtils/DiagnosticsSender.hpp>

/**
 * This is the type expected for the entry point function
 * for all server plug-ins
 * 
 * @param[in, out] server
 *      This is the server to which to add the plugin.
 * 
 * @param[in] configuration
 *      This holds the configuration items of the plugin.
 * 
 * @param[in] diagnosticMessageDelegate
 *      This is the function to call to delliver à diagnostic message.
 * 
 * @param[in, out] unloadDelegate
 *      This is the funcntion to call in order to unloade the plugin.
 *      if this is set to nullptr on return, it means the plug-in was 
 *      unable to load successfully. 
 */
typedef void (*PluginEntryPoint)(
    Http::Server& server,
    Json::Json configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate,
    std::function< void() >& unloadDelegate
);




#endif /* WEB_SERVER_PLUGIN_ENTRY_POINT_HPP */