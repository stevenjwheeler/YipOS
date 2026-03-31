#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

struct DMMessage;

class DMMessageScreen : public Screen {
public:
    DMMessageScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;

private:
    void RenderContent();
    static std::string FormatTimestamp(int64_t date);

    const DMMessage* msg_ = nullptr;
    std::string peer_name_;
};

} // namespace YipOS
