#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <memory>
#include <functional>

namespace NeonGlyph {
namespace Production {

enum class ThemeStatus {
    Concept,
    Design,
    Development,
    Testing,
    Production,
    Deprecated
};

enum class ApprovalStatus {
    Pending,
    Approved,
    Rejected,
    NeedsRevision
};

struct DevelopmentPhase {
    std::string name;
    std::string description;
    std::chrono::days estimated_duration;
    std::vector<std::string> deliverables;
    std::function<bool()> validation_criteria;
    std::string approval_required_from;
};

struct HandoffPoint {
    std::string from_phase;
    std::string to_phase;
    std::vector<std::string> required_deliverables;
    std::function<bool()> approval_criteria;
    std::chrono::system_clock::time_point scheduled_date;
    ApprovalStatus status;
};

class ThemeDevelopmentWorkflow {
private:
    std::map<std::string, DevelopmentPhase> phases_;
    std::vector<HandoffPoint> handoff_points_;
    std::string current_phase_;
    ThemeStatus current_status_;
    
public:
    ThemeDevelopmentWorkflow();
    
    void InitializeStandardWorkflow();
    bool AdvanceToNextPhase();
    std::string GetCurrentPhase() const { return current_phase_; }
    ThemeStatus GetCurrentStatus() const { return current_status_; }
    bool ValidatePhaseCompletion(const std::string& phase_name);
    std::string GeneratePhaseReport(const std::string& phase_name);
    void ScheduleHandoff(const std::string& from_phase, 
                        const std::string& to_phase,
                        std::chrono::system_clock::time_point date);
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> 
        GetWorkflowTimeline() const;
    std::chrono::system_clock::time_point EstimateCompletionDate() const;
};

class WorkflowAutomation {
private:
    ThemeDevelopmentWorkflow workflow_;
    std::map<std::string, std::function<bool()>> automated_checks_;
    
public:
    WorkflowAutomation();
    bool RunAutomatedChecks(const std::string& phase_name);
    std::string GenerateAutomatedReport(const std::string& phase_name);
    void ScheduleAutomatedTask(const std::string& task_name,
                              std::chrono::system_clock::time_point when,
                              std::function<bool()> task);
    bool ExecuteAutomatedHandoff(const HandoffPoint& handoff);
};

} // namespace Production
} // namespace NeonGlyph