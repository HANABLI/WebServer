#ifndef PLUGIN_HPP
#define PLUGIN_HPP

/**
 * @file Plugin.hpp
 *
 * This module declares the Plugin class.
 *
 * © 2024 by Hatem Nabli
 */

#include <Http/Server.hpp>
#include <Json/Json.hpp>
#include <SystemUtils/DynamicLibrary.hpp>
#include <SystemUtils/File.hpp>
#include <functional>
#include <memory>

struct Plugin
{
    /* data */
    /**
     * This is the time that the plug-in image was last modified.
     */
    time_t lastModifiedTime = 0;
    /**
     * This flag indicates whether or not the plug-in should
     * be loaded or reloaded.
     */
    bool needsToLoad = true;
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
    Json::Value configuration;

    /**
     * This is used to dynamically link with the run-time copy
     * of the plug-in image.
     */
    SystemUtils::DynamicLibrary pluginRuntimeLibrary;

    /**
     * If the plug-in is currently loaded, this is the function to
     * call in order to unload it.
     */
    std::function<void()> unloadDelegate;

    /**
     * This is the constructor for the structure.
     *
     * @param[in] imageFileName
     *      This is the plug-in image file name.
     *
     * @param[in] runtimeFileName
     *      This is the plug in runtime file name.
     */
    Plugin::Plugin(std::string& imageFileName, std::string& runtimeFileName) :
        pluginImageFile(imageFileName), pluginRuntimeFile(runtimeFileName) {}

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
    void Plugin::Load(
        Http::Server& server, bool& needsToLoad, const std::string& pluginsRunTimePath,
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate);

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
    void Unload(
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate);
};

#endif /* PLUGIN_HPP */
