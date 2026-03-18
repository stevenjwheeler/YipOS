#pragma once

#include <string>
#include <memory>

namespace YipOS {

struct MediaInfo {
    std::string title;
    std::string artist;
    enum Status { Stopped, Playing, Paused, Unknown } status = Unknown;
};

class MediaController {
public:
    virtual ~MediaController() = default;
    virtual bool Initialize() = 0;
    virtual MediaInfo GetCurrentMedia() = 0;
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void TogglePlayPause() = 0;
    virtual void Next() = 0;
    virtual void Previous() = 0;
    static std::unique_ptr<MediaController> Create();
};

} // namespace YipOS
