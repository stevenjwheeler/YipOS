#ifndef _WIN32

#include "MediaController.hpp"
#include "core/Logger.hpp"

namespace YipOS {

// Linux stub — media control not yet implemented
// TODO: MPRIS/D-Bus integration
class LinuxMediaController : public MediaController {
public:
    bool Initialize() override {
        Logger::Warning("MEDIA: Media control not available on Linux yet");
        return false;
    }

    MediaInfo GetCurrentMedia() override { return {}; }
    void Play() override {}
    void Pause() override {}
    void TogglePlayPause() override {}
    void Next() override {}
    void Previous() override {}
};

std::unique_ptr<MediaController> MediaController::Create() {
    return std::make_unique<LinuxMediaController>();
}

} // namespace YipOS

#endif // !_WIN32
