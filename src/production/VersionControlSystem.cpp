#include "production/VersionControlSystem.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <random>
#include <regex>
#include <set>

namespace NeonGlyph {
namespace Production {

// Utility functions
std::string GenerateVersionString(const VersionInfo& version) {
    std::string version_str = std::to_string(version.major) + "." + 
                             std::to_string(version.minor) + "." + 
                             std::to_string(version.patch);
    
    if (!version.prerelease.empty()) {
        version_str += "-" + version.prerelease;
    }
    
    if (!version.build_metadata.empty()) {
        version_str += "+" + version.build_metadata;
    }
    
    return version_str;
}

VersionInfo ParseVersionString(const std::string& version_string) {
    VersionInfo version{};
    std::regex version_regex(R"(^v?(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z\-\.]+))?(?:\+([0-9A-Za-z\-\.]+))?$)");
    std::smatch match;
    
    if (std::regex_match(version_string, match, version_regex)) {
        version.major = std::stoi(match[1].str());
        version.minor = std::stoi(match[2].str());
        version.patch = std::stoi(match[3].str());
        
        if (match[4].matched) {
            version.prerelease = match[4].str();
        }
        
        if (match[5].matched) {
            version.build_metadata = match[5].str();
        }
    }
    
    return version;
}

bool CompareVersions(const VersionInfo& v1, const VersionInfo& v2) {
    if (v1.major != v2.major) return v1.major < v2.major;
    if (v1.minor != v2.minor) return v1.minor < v2.minor;
    if (v1.patch != v2.patch) return v1.patch < v2.patch;
    return v1.prerelease < v2.prerelease;
}

ReleaseType DetermineReleaseType(const VersionInfo& from_version, const VersionInfo& to_version) {
    if (to_version.major > from_version.major) return ReleaseType::Major;
    if (to_version.minor > from_version.minor) return ReleaseType::Minor;
    if (to_version.patch > from_version.patch) return ReleaseType::Patch;
    if (!to_version.prerelease.empty() && from_version.prerelease.empty()) return ReleaseType::PreRelease;
    return ReleaseType::Hotfix;
}

// Helper functions
std::string GenerateHash(const std::string& content) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(content));
}

std::string GenerateCommitHash() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 40; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm time_info;
    localtime_s(&time_info, &time_t);
    ss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Implementation class
class VersionControlSystem::Impl {
public:
    Impl(const std::string& repository_path) : repository_path_(repository_path) {
        InitializeRepository();
    }

    bool InitializeRepository() {
        std::filesystem::path repo_path(repository_path_);
        std::filesystem::path vcs_path = repo_path / ".ngvcs";
        
        if (!std::filesystem::exists(vcs_path)) {
            std::filesystem::create_directories(vcs_path);
            std::filesystem::create_directories(vcs_path / "objects");
            std::filesystem::create_directories(vcs_path / "refs");
            std::filesystem::create_directories(vcs_path / "refs/heads");
            std::filesystem::create_directories(vcs_path / "refs/tags");
            
            // Initialize HEAD
            std::ofstream head_file(vcs_path / "HEAD");
            head_file << "ref: refs/heads/main\n";
            head_file.close();
            
            // Initialize config
            std::ofstream config_file(vcs_path / "config");
            config_file << "[core]\n";
            config_file << "    repositoryformatversion = 0\n";
            config_file << "    filemode = true\n";
            config_file << "    bare = false\n";
            config_file.close();
            
            // Create initial commit
            CreateInitialCommit();
        }
        
        return true;
    }

    bool IsRepositoryValid() const {
        std::filesystem::path repo_path(repository_path_);
        std::filesystem::path vcs_path = repo_path / ".ngvcs";
        return std::filesystem::exists(vcs_path) && std::filesystem::exists(vcs_path / "HEAD");
    }

    std::string GetRepositoryPath() const {
        return repository_path_;
    }

    VersionInfo GetCurrentVersion() const {
        return current_version_;
    }

    bool SetVersion(const VersionInfo& version) {
        current_version_ = version;
        SaveVersionInfo();
        return true;
    }

    bool IncrementVersion(ReleaseType type) {
        switch (type) {
            case ReleaseType::Major:
                current_version_.major++;
                current_version_.minor = 0;
                current_version_.patch = 0;
                break;
            case ReleaseType::Minor:
                current_version_.minor++;
                current_version_.patch = 0;
                break;
            case ReleaseType::Patch:
            case ReleaseType::Hotfix:
                current_version_.patch++;
                break;
            case ReleaseType::PreRelease:
                current_version_.prerelease = "alpha";
                break;
        }
        
        SaveVersionInfo();
        return true;
    }

    std::vector<VersionInfo> GetVersionHistory() const {
        return version_history_;
    }

    bool TrackAsset(const std::string& asset_path) {
        std::filesystem::path full_path(repository_path_);
        full_path /= asset_path;
        
        if (!std::filesystem::exists(full_path)) {
            return false;
        }
        
        ThemeAsset asset;
        asset.path = asset_path;
        asset.hash = GenerateFileHash(full_path.string());
        asset.size = std::filesystem::file_size(full_path);
        // Convert file_time_type to system_clock::time_point
        auto file_time = std::filesystem::last_write_time(full_path);
        auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
        asset.last_modified = system_time;
        asset.status = VersionControlStatus::Clean;
        
        tracked_assets_[asset_path] = asset;
        SaveTrackedAssets();
        return true;
    }

    bool UntrackAsset(const std::string& asset_path) {
        auto it = tracked_assets_.find(asset_path);
        if (it != tracked_assets_.end()) {
            tracked_assets_.erase(it);
            SaveTrackedAssets();
            return true;
        }
        return false;
    }

    std::vector<ThemeAsset> GetTrackedAssets() const {
        std::vector<ThemeAsset> assets;
        for (const auto& pair : tracked_assets_) {
            assets.push_back(pair.second);
        }
        return assets;
    }

    ThemeAsset GetAssetInfo(const std::string& asset_path) const {
        auto it = tracked_assets_.find(asset_path);
        if (it != tracked_assets_.end()) {
            return it->second;
        }
        return ThemeAsset{};
    }

    bool IsAssetTracked(const std::string& asset_path) const {
        return tracked_assets_.find(asset_path) != tracked_assets_.end();
    }

    bool StageAsset(const std::string& asset_path) {
        auto it = tracked_assets_.find(asset_path);
        if (it != tracked_assets_.end()) {
            it->second.status = VersionControlStatus::Staged;
            staged_assets_.insert(asset_path);
            SaveStagedAssets();
            return true;
        }
        return false;
    }

    bool UnstageAsset(const std::string& asset_path) {
        auto it = tracked_assets_.find(asset_path);
        if (it != tracked_assets_.end()) {
            it->second.status = VersionControlStatus::Modified;
            staged_assets_.erase(asset_path);
            SaveStagedAssets();
            return true;
        }
        return false;
    }

    std::vector<ThemeAsset> GetStagedAssets() const {
        std::vector<ThemeAsset> assets;
        for (const auto& asset_path : staged_assets_) {
            auto it = tracked_assets_.find(asset_path);
            if (it != tracked_assets_.end()) {
                assets.push_back(it->second);
            }
        }
        return assets;
    }

    bool HasStagedChanges() const {
        return !staged_assets_.empty();
    }

    bool Commit(const std::string& message, const std::string& author) {
        if (!HasStagedChanges()) {
            return false;
        }

        CommitInfo commit;
        commit.hash = GenerateCommitHash();
        commit.author = author;
        commit.message = message;
        commit.timestamp = std::chrono::system_clock::now();
        
        for (std::set<std::string>::const_iterator it_staged = staged_assets_.begin(); it_staged != staged_assets_.end(); ++it_staged) {
            const std::string& asset_path = *it_staged;
            std::map<std::string, ThemeAsset>::iterator it_tracked = tracked_assets_.find(asset_path);
            if (it_tracked != tracked_assets_.end()) {
                commit.changed_files.push_back(asset_path);
                it_tracked->second.status = VersionControlStatus::Committed;
            }
        }
        
        commits_.push_back(commit);
        staged_assets_.clear();
        
        SaveCommit(commit);
        SaveTrackedAssets();
        SaveStagedAssets();
        
        return true;
    }

    CommitInfo GetCommitInfo(const std::string& commit_hash) const {
        for (const auto& commit : commits_) {
            if (commit.hash == commit_hash) {
                return commit;
            }
        }
        return CommitInfo{};
    }

    std::vector<CommitInfo> GetCommitHistory(size_t limit = 100) const {
        std::vector<CommitInfo> history = commits_;
        if (history.size() > limit) {
            history.resize(limit);
        }
        return history;
    }

    bool CreateBranch(const std::string& branch_name, const std::string& base_branch = "main") {
        BranchInfo branch;
        branch.name = branch_name;
        branch.base_branch = base_branch;
        branch.head_commit = GetCurrentBranchHead(base_branch);
        branch.created_at = std::chrono::system_clock::now();
        branch.is_active = false;
        
        branches_[branch_name] = branch;
        SaveBranches();
        return true;
    }

    bool SwitchBranch(const std::string& branch_name) {
        auto it = branches_.find(branch_name);
        if (it != branches_.end()) {
            for (auto& branch : branches_) {
                branch.second.is_active = false;
            }
            it->second.is_active = true;
            current_branch_ = branch_name;
            SaveBranches();
            SaveCurrentBranch();
            return true;
        }
        return false;
    }

    std::vector<BranchInfo> GetBranches() const {
        std::vector<BranchInfo> branch_list;
        for (const auto& pair : branches_) {
            branch_list.push_back(pair.second);
        }
        return branch_list;
    }

    std::string GetCurrentBranch() const {
        return current_branch_;
    }

    bool CreateTag(const std::string& tag_name, const std::string& commit_hash = "HEAD", const std::string& message = "") {
        TagInfo tag;
        tag.name = tag_name;
        tag.commit_hash = commit_hash;
        tag.message = message;
        tag.timestamp = std::chrono::system_clock::now();
        
        tags_[tag_name] = tag;
        SaveTags();
        return true;
    }

    RepositoryStats GetRepositoryStats() const {
        RepositoryStats stats{};
        stats.total_assets = tracked_assets_.size();
        stats.tracked_assets = tracked_assets_.size();
        stats.commits_count = commits_.size();
        stats.branches_count = branches_.size();
        stats.tags_count = tags_.size();
        
        if (!commits_.empty()) {
            stats.first_commit_date = commits_.front().timestamp;
            stats.last_commit_date = commits_.back().timestamp;
            
            std::map<std::string, size_t> author_counts;
            for (const auto& commit : commits_) {
                author_counts[commit.author]++;
            }
            
            size_t max_commits = 0;
            for (const auto& pair : author_counts) {
                stats.authors_commit_counts[pair.first] = pair.second;
                if (pair.second > max_commits) {
                    max_commits = pair.second;
                    stats.most_active_author = pair.first;
                }
            }
        }
        
        return stats;
    }

private:
    void CreateInitialCommit() {
        CommitInfo commit;
        commit.hash = GenerateCommitHash();
        commit.author = "System";
        commit.message = "Initial commit";
        commit.timestamp = std::chrono::system_clock::now();
        
        commits_.push_back(commit);
        SaveCommit(commit);
    }

    std::string GenerateFileHash(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return GenerateHash(buffer.str());
    }

    std::string GetCurrentBranchHead(const std::string& branch_name) {
        auto it = branches_.find(branch_name);
        if (it != branches_.end()) {
            return it->second.head_commit;
        }
        return "";
    }

    void SaveVersionInfo() const {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        
        std::ofstream version_file(vcs_path / "version");
        version_file << GenerateVersionString(current_version_) << "\n";
        version_file.close();
    }

    void SaveTrackedAssets() const {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        
        std::ofstream assets_file(vcs_path / "tracked_assets");
        for (const auto& pair : tracked_assets_) {
            const auto& asset = pair.second;
            assets_file << asset.path << "|" << asset.hash << "|" << asset.size << "|" 
                       << asset.last_modified.time_since_epoch().count() << "|" 
                       << static_cast<int>(asset.status) << "\n";
        }
        assets_file.close();
    }

    void SaveStagedAssets() const {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        
        std::ofstream staged_file(vcs_path / "staged_assets");
        for (std::set<std::string>::const_iterator it = staged_assets_.begin(); it != staged_assets_.end(); ++it) {
            staged_file << *it << "\n";
        }
        staged_file.close();
    }

    void SaveCommit(const CommitInfo& commit) {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        vcs_path /= "objects";
        
        std::ofstream commit_file(vcs_path / commit.hash);
        commit_file << commit.hash << "\n";
        commit_file << commit.author << "\n";
        commit_file << commit.message << "\n";
        commit_file << commit.timestamp.time_since_epoch().count() << "\n";
        
        commit_file << commit.changed_files.size() << "\n";
        for (const auto& file : commit.changed_files) {
            commit_file << file << "\n";
        }
        
        commit_file << commit.added_files.size() << "\n";
        for (const auto& file : commit.added_files) {
            commit_file << file << "\n";
        }
        
        commit_file << commit.deleted_files.size() << "\n";
        for (const auto& file : commit.deleted_files) {
            commit_file << file << "\n";
        }
        
        commit_file.close();
    }

    void SaveBranches() const {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        
        std::ofstream branches_file(vcs_path / "branches");
        for (const auto& pair : branches_) {
            const auto& branch = pair.second;
            branches_file << branch.name << "|" << branch.base_branch << "|" 
                         << branch.head_commit << "|" << branch.created_at.time_since_epoch().count() 
                         << "|" << branch.is_active << "\n";
        }
        branches_file.close();
    }

    void SaveCurrentBranch() const {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        
        std::ofstream branch_file(vcs_path / "current_branch");
        branch_file << current_branch_ << "\n";
        branch_file.close();
    }

    void SaveTags() const {
        std::filesystem::path vcs_path(repository_path_);
        vcs_path /= ".ngvcs";
        
        std::ofstream tags_file(vcs_path / "tags");
        for (const auto& pair : tags_) {
            const auto& tag = pair.second;
            tags_file << tag.name << "|" << tag.commit_hash << "|" 
                     << tag.message << "|" << tag.timestamp.time_since_epoch().count() << "\n";
        }
        tags_file.close();
    }

    std::string repository_path_;
    VersionInfo current_version_{1, 0, 0, "", "", "", std::chrono::system_clock::now(), "System", "Initial version"};
    std::vector<VersionInfo> version_history_;
    std::map<std::string, ThemeAsset> tracked_assets_;
    std::set<std::string> staged_assets_;
    std::vector<CommitInfo> commits_;
    std::map<std::string, BranchInfo> branches_;
    std::string current_branch_ = "main";
    
    struct TagInfo {
        std::string name;
        std::string commit_hash;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::map<std::string, TagInfo> tags_;
};

// VersionControlSystem implementation
VersionControlSystem::VersionControlSystem(const std::string& repository_path) 
    : impl_(std::make_unique<Impl>(repository_path)) {}

VersionControlSystem::~VersionControlSystem() = default;

bool VersionControlSystem::InitializeRepository() {
    return impl_->InitializeRepository();
}

bool VersionControlSystem::IsRepositoryValid() const {
    return impl_->IsRepositoryValid();
}

std::string VersionControlSystem::GetRepositoryPath() const {
    return impl_->GetRepositoryPath();
}

VersionInfo VersionControlSystem::GetCurrentVersion() const {
    return impl_->GetCurrentVersion();
}

bool VersionControlSystem::SetVersion(const VersionInfo& version) {
    return impl_->SetVersion(version);
}

bool VersionControlSystem::IncrementVersion(ReleaseType type) {
    return impl_->IncrementVersion(type);
}

std::vector<VersionInfo> VersionControlSystem::GetVersionHistory() const {
    return impl_->GetVersionHistory();
}

bool VersionControlSystem::TrackAsset(const std::string& asset_path) {
    return impl_->TrackAsset(asset_path);
}

bool VersionControlSystem::UntrackAsset(const std::string& asset_path) {
    return impl_->UntrackAsset(asset_path);
}

std::vector<ThemeAsset> VersionControlSystem::GetTrackedAssets() const {
    return impl_->GetTrackedAssets();
}

ThemeAsset VersionControlSystem::GetAssetInfo(const std::string& asset_path) const {
    return impl_->GetAssetInfo(asset_path);
}

bool VersionControlSystem::IsAssetTracked(const std::string& asset_path) const {
    return impl_->IsAssetTracked(asset_path);
}

bool VersionControlSystem::StageAsset(const std::string& asset_path) {
    return impl_->StageAsset(asset_path);
}

bool VersionControlSystem::UnstageAsset(const std::string& asset_path) {
    return impl_->UnstageAsset(asset_path);
}

std::vector<ThemeAsset> VersionControlSystem::GetStagedAssets() const {
    return impl_->GetStagedAssets();
}

bool VersionControlSystem::HasStagedChanges() const {
    return impl_->HasStagedChanges();
}

bool VersionControlSystem::Commit(const std::string& message, const std::string& author) {
    return impl_->Commit(message, author);
}

CommitInfo VersionControlSystem::GetCommitInfo(const std::string& commit_hash) const {
    return impl_->GetCommitInfo(commit_hash);
}

std::vector<CommitInfo> VersionControlSystem::GetCommitHistory(size_t limit) const {
    return impl_->GetCommitHistory(limit);
}

bool VersionControlSystem::CreateBranch(const std::string& branch_name, const std::string& base_branch) {
    return impl_->CreateBranch(branch_name, base_branch);
}

bool VersionControlSystem::SwitchBranch(const std::string& branch_name) {
    return impl_->SwitchBranch(branch_name);
}

std::vector<BranchInfo> VersionControlSystem::GetBranches() const {
    return impl_->GetBranches();
}

std::string VersionControlSystem::GetCurrentBranch() const {
    return impl_->GetCurrentBranch();
}

bool VersionControlSystem::CreateTag(const std::string& tag_name, const std::string& commit_hash, const std::string& message) {
    return impl_->CreateTag(tag_name, commit_hash, message);
}

VersionControlSystem::RepositoryStats VersionControlSystem::GetRepositoryStats() const {
    return impl_->GetRepositoryStats();
}

} // namespace Production
} // namespace NeonGlyph