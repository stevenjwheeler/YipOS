#pragma once

#include <string>
#include <vector>
#include <set>

namespace YipOS {

struct VRCAvatarParam {
    std::string name;
    std::string input_address;
    std::string input_type;   // "Bool", "Int", "Float"
    std::string output_address;
    std::string output_type;
};

struct VRCAvatarEntry {
    std::string id;       // "avtr_xxxx"
    std::string name;
    std::vector<VRCAvatarParam> parameters;
};

class VRCAvatarData {
public:
    bool Scan(const std::string& osc_path);
    const std::vector<VRCAvatarEntry>& GetAvatars() const { return avatars_; }
    const VRCAvatarEntry* FindById(const std::string& id) const;

    // Filtered toggle params: Bool input+output, excluding known non-toggle prefixes
    std::vector<const VRCAvatarParam*> GetToggleParams(const std::string& avatar_id) const;

    void SetCurrentAvatarId(const std::string& id) { current_avatar_id_ = id; }
    const std::string& GetCurrentAvatarId() const { return current_avatar_id_; }
    const VRCAvatarEntry* GetCurrentAvatar() const { return FindById(current_avatar_id_); }

    // Detect active avatar from most recently modified LocalAvatarData file
    // vrc_root = parent of OSC dir (e.g. .../VRChat/VRChat/)
    // Also loads expression parameter names from LocalAvatarData for filtering
    bool DetectCurrentAvatar(const std::string& vrc_root);

    // Load expression params from a specific avatar's LocalAvatarData file
    bool LoadExpressionParams(const std::string& vrc_root, const std::string& avatar_id);

    // Check if a param name is in the expression params set
    bool IsExpressionParam(const std::string& name) const {
        return expression_params_.count(name) > 0;
    }

    static std::string DefaultOSCPath();

private:
    static bool IsExcludedParam(const std::string& name);
    static bool IsExpressionExcluded(const std::string& name);

    std::vector<VRCAvatarEntry> avatars_;
    std::string current_avatar_id_;
    std::set<std::string> expression_params_;  // from LocalAvatarData
};

} // namespace YipOS
