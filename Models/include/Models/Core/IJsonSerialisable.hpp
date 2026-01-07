#pragma once

#include <Json/Json.hpp>

namespace FalcataIoTServer
{
    /**
     * @brief Contract for JSON serialization / deserialization
     *
     * Used by:
     *  - REST API
     *  - Database mapping
     *  - Event bus (MQTT / WS)
     */
    class IJsonSerializable
    {
    public:
        virtual ~IJsonSerializable() = default;

        /**
         * Serialize object to JSON
         */
        virtual Json::Value ToJson() const = 0;

        /**
         * Deserialize object from JSON
         */
        virtual void FromJson(const Json::Value& json) = 0;
    };
}  // namespace FalcataIoTServer
