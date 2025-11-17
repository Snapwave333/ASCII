#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <filesystem>
#include <functional>

namespace NeonGlyph {
namespace Production {

enum class VersionControlStatus {
    Clean,
    Modified,
    Staged,
    Committed,
    Conflicted,
    Untracked
};

enum class ReleaseType {
    Major,
    Minor,
    Patch,
    Hotfix,
    PreRelease
};

struct VersionInfo {
    int major;
    int minor;
    int patch;
    std::string prerelease;
    std::string build_metadata;
    std::string commit_hash;
    std::chrono::system_clock::time_point timestamp;
    std::string author;
    std::string description;
};

struct ThemeAsset {
    std::string path;
    std::string hash;
    size_t size;
    std::chrono::system_clock::time_point last_modified;
    VersionControlStatus status;
};

struct BranchInfo {
    std::string name;
    std::string base_branch;
    std::string head_commit;
    std::chrono::system_clock::time_point created_at;
    bool is_active;
};

struct CommitInfo {
    std::string hash;
    std::string author;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> changed_files;
    std::vector<std::string> added_files;
    std::vector<std::string> deleted_files;
};

class VersionControlSystem {
public:
    VersionControlSystem(const std::string& repository_path);
    ~VersionControlSystem();

    // Repository management
    bool InitializeRepository();
    bool IsRepositoryValid() const;
    std::string GetRepositoryPath() const;

    // Version management
    VersionInfo GetCurrentVersion() const;
    bool SetVersion(const VersionInfo& version);
    bool IncrementVersion(ReleaseType type);
    std::vector<VersionInfo> GetVersionHistory() const;

    // Theme asset management
    bool TrackAsset(const std::string& asset_path);
    bool UntrackAsset(const std::string& asset_path);
    std::vector<ThemeAsset> GetTrackedAssets() const;
    ThemeAsset GetAssetInfo(const std::string& asset_path) const;
    bool IsAssetTracked(const std::string& asset_path) const;

    // Staging and committing
    bool StageAsset(const std::string& asset_path);
    bool UnstageAsset(const std::string& asset_path);
    std::vector<ThemeAsset> GetStagedAssets() const;
    bool HasStagedChanges() const;

    bool Commit(const std::string& message, const std::string& author);
    CommitInfo GetCommitInfo(const std::string& commit_hash) const;
    std::vector<CommitInfo> GetCommitHistory(size_t limit = 100) const;

    // Branch management
    bool CreateBranch(const std::string& branch_name, const std::string& base_branch = "main");
    bool SwitchBranch(const std::string& branch_name);
    bool MergeBranch(const std::string& source_branch, const std::string& target_branch);
    bool DeleteBranch(const std::string& branch_name);
    std::vector<BranchInfo> GetBranches() const;
    std::string GetCurrentBranch() const;

    // Diff and merge
    std::string GetAssetDiff(const std::string& asset_path, const std::string& base_commit = "", const std::string& target_commit = "HEAD") const;
    bool HasConflicts() const;
    std::vector<std::string> GetConflictingAssets() const;
    bool ResolveConflict(const std::string& asset_path, const std::string& resolution_strategy = "ours");

    // Tagging and releases
    bool CreateTag(const std::string& tag_name, const std::string& commit_hash = "HEAD", const std::string& message = "");
    bool DeleteTag(const std::string& tag_name);
    std::vector<std::string> GetTags() const;
    bool CreateRelease(const VersionInfo& version, const std::string& release_notes = "");

    // Configuration management
    bool SetConfiguration(const std::string& key, const std::string& value);
    std::string GetConfiguration(const std::string& key) const;
    std::map<std::string, std::string> GetAllConfigurations() const;

    // Hooks and automation
    using PreCommitHook = std::function<bool(const std::vector<std::string>&)>;
    using PostCommitHook = std::function<void(const CommitInfo&)>;
    using PreMergeHook = std::function<bool(const std::string&, const std::string&)>;
    using PostMergeHook = std::function<void(const std::string&, const std::string&)>;

    void SetPreCommitHook(PreCommitHook hook);
    void SetPostCommitHook(PostCommitHook hook);
    void SetPreMergeHook(PreMergeHook hook);
    void SetPostMergeHook(PostMergeHook hook);

    // Backup and restore
    bool CreateBackup(const std::string& backup_path, const std::string& backup_name = "");
    bool RestoreFromBackup(const std::string& backup_path);
    std::vector<std::string> GetAvailableBackups() const;

    // Validation and integrity
    bool ValidateRepositoryIntegrity() const;
    bool ValidateAssetIntegrity(const std::string& asset_path) const;
    std::vector<std::string> GetCorruptedAssets() const;
    bool RepairCorruptedAssets();

    // Statistics and reporting
    struct RepositoryStats {
        size_t total_assets;
        size_t tracked_assets;
        size_t commits_count;
        size_t branches_count;
        size_t tags_count;
        std::chrono::system_clock::time_point first_commit_date;
        std::chrono::system_clock::time_point last_commit_date;
        std::string most_active_author;
        std::map<std::string, size_t> authors_commit_counts;
    };

    RepositoryStats GetRepositoryStats() const;
    std::string GenerateReport() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Utility functions
std::string GenerateVersionString(const VersionInfo& version);
VersionInfo ParseVersionString(const std::string& version_string);
bool CompareVersions(const VersionInfo& v1, const VersionInfo& v2);
ReleaseType DetermineReleaseType(const VersionInfo& from_version, const VersionInfo& to_version);

} // namespace Production
} // namespace NeonGlyph