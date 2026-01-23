#include <Models/Core/CoreObject.hpp>

namespace FalcataIoTServer
{
    struct CoreObject::Core
    {
        UuidV7::UuidV7 uuid;

        /**
         * This is a helper object used to generate and publish
         * diagnostic message.
         */
        SystemUtils::DiagnosticsSender diagnosticsSender;

        Core(UuidV7::UuidV7 uuid) :
            diagnosticsSender(
                StringUtils::sprintf("Postgresql::CoreObject #%s", uuid.ToString().c_str())),
            uuid(std::move(uuid)) {}
        ~Core() noexcept = default;
    };

    CoreObject::CoreObject() : core_(std::make_unique<Core>(UuidV7::UuidV7::Generate())) {}
    CoreObject::CoreObject(std::string& id) :
        core_(std::make_unique<Core>(UuidV7::UuidV7::FromString(id))) {}

    CoreObject::~CoreObject() noexcept = default;

    const std::vector<std::string> CoreObject::GetInsertParams() const { return {}; }
    const std::vector<std::string> CoreObject::GetUpdateParams() const { return {}; }
    const std::vector<std::string> CoreObject::GetRemoveParams() const { return {}; }
    const std::vector<std::string> CoreObject::GetDisableParams() const { return {}; }

    const UuidV7::UuidV7& CoreObject::Uuid_r() const noexcept { return core_->uuid; }

    const std::string CoreObject::Uuid_s() const noexcept { return core_->uuid.ToString(); }

    void CoreObject::UuidFromString(const std::string& uuid) {
        core_->uuid = std::move(UuidV7::UuidV7::FromString(uuid));
    }

    SystemUtils::DiagnosticsSender::UnsubscribeDelegate CoreObject::SubscribeToDiagnostics(
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate delegate, size_t minLevel) {
        return core_->diagnosticsSender.SubscribeToDiagnostics(delegate, minLevel);
    }
}  // namespace FalcataIoTServer