#ifdef _WIN32

#include "MediaController.hpp"
#include "core/Logger.hpp"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

namespace YipOS {

using namespace winrt;
using namespace Windows::Media::Control;

class GSMTCMediaController : public MediaController {
public:
    ~GSMTCMediaController() override = default;

    bool Initialize() override {
        try {
            auto op = GlobalSystemMediaTransportControlsSessionManager::RequestAsync();
            session_manager_ = op.get();
            if (!session_manager_) {
                Logger::Warning("MEDIA: Failed to get GSMTC session manager");
                return false;
            }
            Logger::Info("MEDIA: GSMTC initialized");
            return true;
        } catch (const hresult_error& e) {
            Logger::Warning("MEDIA: GSMTC init failed: " + to_string(e.message()));
            return false;
        }
    }

    MediaInfo GetCurrentMedia() override {
        MediaInfo info;
        if (!session_manager_) return info;

        try {
            auto session = session_manager_.GetCurrentSession();
            if (!session) return info;

            // Playback status
            auto playback = session.GetPlaybackInfo();
            if (playback) {
                auto status = playback.PlaybackStatus();
                switch (status) {
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
                        info.status = MediaInfo::Playing; break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
                        info.status = MediaInfo::Paused; break;
                    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
                        info.status = MediaInfo::Stopped; break;
                    default:
                        info.status = MediaInfo::Unknown; break;
                }
            }

            // Media properties
            auto props_op = session.TryGetMediaPropertiesAsync();
            auto props = props_op.get();
            if (props) {
                info.title = to_string(props.Title());
                info.artist = to_string(props.Artist());
            }
        } catch (const hresult_error&) {
            // Session may have been invalidated
        }

        return info;
    }

    void Play() override { SendCommand([](auto s) { s.TryPlayAsync().get(); }); }
    void Pause() override { SendCommand([](auto s) { s.TryPauseAsync().get(); }); }
    void TogglePlayPause() override { SendCommand([](auto s) { s.TryTogglePlayPauseAsync().get(); }); }
    void Next() override { SendCommand([](auto s) { s.TrySkipNextAsync().get(); }); }
    void Previous() override { SendCommand([](auto s) { s.TrySkipPreviousAsync().get(); }); }

private:
    template<typename F>
    void SendCommand(F&& fn) {
        if (!session_manager_) return;
        try {
            auto session = session_manager_.GetCurrentSession();
            if (session) fn(session);
        } catch (const hresult_error&) {}
    }

    GlobalSystemMediaTransportControlsSessionManager session_manager_{nullptr};
};

std::unique_ptr<MediaController> MediaController::Create() {
    return std::make_unique<GSMTCMediaController>();
}

} // namespace YipOS

#endif // _WIN32
