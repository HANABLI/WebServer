/**
 * @file PluginLoader.cpp
 * 
 * This module  
 */

#include "PluginLoader.hpp"


    struct PluginLoader::Impl {
        // Properties
        
        /**
         * This is the server for which to handle the plug-in loader.
         */
        Http::Server &server;
        /**
         * This is the plug-in loader worker thread.
         */
        std::thread worker;
        /** 
         * This is the function to call if the scan flag is set
         */
        std::function < void() > scanDelegate;
        /** 
         * This is used to wait for either the scan or stop flag to be set.
         */
        std::condition_variable pluginLoaderWakeCondition;
        /** 
         * This is used to synchronize access to the scan and stop flags.
         */
        std::mutex pluginLoaderMutex;
        /** 
         * This flag is set to signal when the scanDelegate should
         * be called. this function clears the flag. 
         */
        bool pluginLoaderScan;
        /**  
         * This flag is set to signal when the thread should exit.
         */
        bool pluginLoaderStop;
        /**
         * This is the path to the plugins runtime folder.
         */
        std::string pluginsRunTimePath;
        /**
         * This is the path to the plugin image folder.
         */
        std::string pluginsImagePath;
        /**
         * 
         */
        SystemUtils::DirectoryMonitor directroyMonitor;
        /**
         * 
         */
        std::map< std::string, std::shared_ptr< Plugin> >& plugins;

        /**
         * Message Delegate
         */
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate;
        
        // Methods

        /**
         * This is the constructor of the implementation structure
         * 
         * @param[in, out] server
         *      This is the server for which to handle plug-in loading.
         */

        Impl(Http::Server& server, std::map< std::string, std::shared_ptr< Plugin> >& plugins): server(server), plugins(plugins) {

        }

        /**
         * This method is called to perform manual scanning of the plugin image
         * folder, looking for plugins to load.
         */
        void Scan() {
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
        }


        /**
         * This function is called in its own worker thread. It will call
         * the given scanDelegate whenever the given scan flag remains clear
         * after a short wait period after being set.
         * 
         */
        void Launch() {
            std::unique_lock< std::mutex > lock(pluginLoaderMutex);
            diagnosticMessageDelegate("PluginLoader", 0, "starting");
            while (!pluginLoaderStop) {
                diagnosticMessageDelegate("PluginLoader", 0, "sleeping");
                (void)pluginLoaderWakeCondition.wait(
                    lock,
                    [this] { return pluginLoaderScan || pluginLoaderStop; }
                );
                diagnosticMessageDelegate("PluginLoader", 0, "Waking");
                if (pluginLoaderStop) {
                    break;
                }
                if (pluginLoaderScan) {
                    diagnosticMessageDelegate("PluginLoader", 0, "need to scan ... waiting");
                    pluginLoaderScan = false;
                    if (
                        pluginLoaderWakeCondition.wait_for(
                            lock,
                            std::chrono::microseconds(100),
                            [this]{ return pluginLoaderScan || pluginLoaderStop; }
                        )
                    ) {
                        diagnosticMessageDelegate("PluginLoader", 0, "Need to scan ... still updating; backing off");
                    } else {
                        diagnosticMessageDelegate("PluginLoader", 0, "Scanning");
                        Scan();
                    }
                }
            }
            diagnosticMessageDelegate("PluginLoader", 0, "stopping");
        }
    };
    
    // Methods
    PluginLoader::~PluginLoader() noexcept {
        StopScanning();
    }

    PluginLoader::PluginLoader(
        Http::Server& server,
        std::string pluginRuntimePath,
        std::string pluginsImagePath,
        std::map< std::string, std::shared_ptr< Plugin> >& plugins,
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate
    ): impl_(new Impl(server, plugins))
    {
        impl_->pluginsImagePath = pluginsImagePath;
        impl_->pluginsRunTimePath = pluginRuntimePath;
        impl_->diagnosticMessageDelegate = diagnosticMessageDelegate;
    }

    void PluginLoader::Scan() {
        std::lock_guard< decltype(impl_->pluginLoaderMutex) > lock(impl_->pluginLoaderMutex);
        impl_->Scan();
    }
    
    void PluginLoader::StartScanning() {
        if (impl_->worker.joinable()) {
            return;
        }
        const auto imagePathChangedDelegate = [this]{
            std::lock_guard< decltype(impl_->pluginLoaderMutex) > lock(impl_->pluginLoaderMutex);
            impl_->pluginLoaderScan = true;
            impl_->pluginLoaderWakeCondition.notify_all();
        };
        if (
            !impl_->directroyMonitor.Start(imagePathChangedDelegate, impl_->pluginsImagePath)    
        ) {
            fprintf(stderr, "warning: unable to monitor plug-ins image directory (%s)\n", impl_->pluginsImagePath.c_str());
        }
        impl_->worker = std::thread(&Impl::Launch, impl_.get());
    }

    void PluginLoader::StopScanning() {
        if(!impl_->worker.joinable()) {
            return;
        }
        impl_->directroyMonitor.Stop();
        {
            std::lock_guard< decltype(impl_->pluginLoaderMutex) > lock(impl_->pluginLoaderMutex);
            impl_->pluginLoaderStop = true;
            impl_->pluginLoaderWakeCondition.notify_all();
        }
        impl_->worker.join();
    }
    
