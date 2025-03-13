#include "Plugin.hpp"
#include <Http/Server.hpp>
#include <Json/Json.hpp>
#include <StringUtils/StringUtils.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/DynamicLibrary.hpp>
#include <WebServer/PluginEntryPoint.hpp>

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
void Plugin::Unload(
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate) {
    diagnosticMessageDelegate("", 0,
                              StringUtils::sprintf("Unloading '%s' plugin", moduleName.c_str()));
    if (unloadDelegate == nullptr)
    { return; }
    unloadDelegate();
    unloadDelegate = nullptr;
    pluginRuntimeLibrary.Unload();
    diagnosticMessageDelegate("", 0,
                              StringUtils::sprintf("Plugin '%s' unloaded", moduleName.c_str()));
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
void Plugin::Load(
    Http::Server& server, bool needsToLoad, const std::string& pluginsRunTimePath,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate) {
    diagnosticMessageDelegate("", 0,
                              StringUtils::sprintf("Copying plugin '%s'", moduleName.c_str()));
    if (pluginImageFile.Copy(pluginRuntimeFile.GetPath()))
    {
        diagnosticMessageDelegate("", 0,
                                  StringUtils::sprintf("Linking plugin '%s'", moduleName.c_str()));
        if (pluginRuntimeLibrary.Load(pluginsRunTimePath, moduleName))
        {
            diagnosticMessageDelegate(
                "", 0,
                StringUtils::sprintf("Looking for plugin '%s' entrypoint", moduleName.c_str()));
            const auto loadPlugin =
                (PluginEntryPoint)pluginRuntimeLibrary.GetProcedure("LoadPlugin");
            if (loadPlugin != nullptr)
            {
                diagnosticMessageDelegate(
                    "", 0, StringUtils::sprintf("Loading plugin entrypoint", moduleName.c_str()));
                loadPlugin(
                    &server, configuration,
                    [this, diagnosticMessageDelegate](std::string senderName, size_t level,
                                                      std::string message)
                    {
                        if (senderName.empty())
                        {
                            diagnosticMessageDelegate(moduleName, level, message);
                        } else
                        {
                            diagnosticMessageDelegate(
                                StringUtils::sprintf("%s%s", moduleName.c_str(),
                                                     senderName.c_str()),
                                level, message);
                        }
                    },
                    unloadDelegate);
                if (unloadDelegate == nullptr)
                {
                    diagnosticMessageDelegate(
                        "", SystemUtils::DiagnosticsSender::Levels::WARNING,
                        StringUtils::sprintf("unable to find plugin '%s' entrypoint",
                                             moduleName.c_str()));
                    needsToLoad = false;
                } else
                {
                    diagnosticMessageDelegate(
                        "", 1, StringUtils::sprintf("Plugin '%s' Loaded", moduleName.c_str()));
                }
            } else
            {
                diagnosticMessageDelegate(
                    "", SystemUtils::DiagnosticsSender::Levels::WARNING,
                    StringUtils::sprintf("unable to find plugin '%s' entrypoint",
                                         moduleName.c_str()));
                needsToLoad = false;
            }
            if (unloadDelegate == nullptr)
            { pluginRuntimeLibrary.Unload(); }
        } else
        {
            diagnosticMessageDelegate(
                "", SystemUtils::DiagnosticsSender::Levels::WARNING,
                StringUtils::sprintf("unable to link plugin '%s' library", moduleName.c_str()));
            needsToLoad = false;
        }
        if (unloadDelegate == nullptr)
        { pluginRuntimeFile.Destroy(); }
    } else
    {
        diagnosticMessageDelegate(
            "", SystemUtils::DiagnosticsSender::Levels::WARNING,
            StringUtils::sprintf("unable to copy plugin '%s' entrypoint", moduleName.c_str()));
        needsToLoad = false;
    }
}
