#pragma once

#include "Screen.hpp"
#include <string>

namespace YipOS {

struct VRCXWorldEntry;

class VRCXWorldDetailScreen : public Screen {
public:
    VRCXWorldDetailScreen(PDAController& pda);

    void Render() override;
    void RenderDynamic() override;
    bool OnInput(const std::string& key) override;

private:
    void RenderContent();

    static std::string ParseInstanceType(const std::string& location);
    static std::string ParseRegion(const std::string& location);
    static void LaunchWorld(const std::string& location);

    const VRCXWorldEntry* world_ = nullptr;
};

} // namespace YipOS
