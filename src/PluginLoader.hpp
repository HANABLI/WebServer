#ifndef PLUGIN_LOADER_HPP
#define PLUGIN_LOADER_HPP

/**
 * @file PluginLoader.hpp
 * 
 * This module declares the plug-in loader
 * class.
 * 
 * © 2014 by Hatem Nabli
 */

#include "Plugin.hpp"
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <SystemUtils/DirectoryMonitor.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <Http/Server.hpp>
#include <StringUtils/StringUtils.hpp>

/**
 * This class monitors the directory containing the image files
 * for the plug-ins of the web server. It maintains a worker thread 
 * which is responsible for loading any image files that are found to match
 * a known plug-in, or reload it if the image file changes.
 */
class PluginLoader {

public:
    // Lifecycle Methods
    ~PluginLoader() noexcept;
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader(PluginLoader&&) noexcept = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;
    PluginLoader& operator=(PluginLoader&&) noexcept = delete;

    // Public methods
public:
    /**
     * This is the Plug-in loader cunstructor.
     */
    PluginLoader(
        Http::Server& server,
        std::string pluginsRunTimePath,
        std::string pluginsImagePath,
        std::map< std::string, std::shared_ptr< Plugin> >& plugins,
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate
    );

    /**
     * This method is called to perform manual scanning of the plugin image
     * folder, looking for plugins to load.
     */
    void Scan();

    /**
     * This method start a worker thread which will monitor the image folder
     * for plugins, loading, reloading, or unloading plug-ins as necessary.
     */
    void StartScanning();

    /**
     * This method stops the thread worker which monitors the image folder.
     */
    void StopScanning();


    // Private properties
private:
    /* data */

    /**
     * This is the type of structure that contains the private
     * properties of the instance. It is defined in the implementation
     * and declared here to ensure that iwt is scoped inside the class.
    */
    struct Impl;

    /**
    * This contains the private properties of the instance.
    */       
    std::unique_ptr<struct Impl> impl_;
    

};


#endif /* PLUGIN_LOADER_HPP */
