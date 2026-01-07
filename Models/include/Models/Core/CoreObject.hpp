/**
 *
 */
#pragma once

#include <UuidV7/UuidV7.hpp>
#include <Models/Core/IJsonSerialisable.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <StringUtils/StringUtils.hpp>
#include <memory>

namespace FalcataIoTServer
{
    class CoreObject
    {
        // Life Cycle managment
    public:
        CoreObject(const CoreObject&) = delete;
        CoreObject(CoreObject&&) noexcept = default;
        CoreObject& operator=(const CoreObject&) = delete;
        CoreObject& operator=(CoreObject&&) noexcept = default;
        ~CoreObject() noexcept;

        // Methods
    public:
        /**
         * Default constructor
         */
        explicit CoreObject(/* args */);

        CoreObject(std::string& uuid);

        /**
         * This method returns the current raw id;
         */
        const UuidV7::UuidV7& Uuid_r() const noexcept;

        /**
         * This method returns the current string id;
         */
        const std::string Uuid_s() const noexcept;
        /**
         * This method create an uuid from an existing string uuid.
         */
        void UuidFromString(const std::string& uuid);

        virtual const std::vector<std::string> GetInsertParams() const;
        virtual const std::vector<std::string> GetUpdateParams() const;
        virtual const std::vector<std::string> GetRemoveParams() const;
        virtual const std::vector<std::string> GetDisableParams() const;

        SystemUtils::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
            SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate delegate,
            size_t minLevel = 0);

    public:
        /* data */
        struct Core;

        std::unique_ptr<Core> core_;
    };

}  // namespace FalcataIoTServer