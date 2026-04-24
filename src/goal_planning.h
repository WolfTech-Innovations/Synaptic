#ifndef GOAL_PLANNING_H
#define GOAL_PLANNING_H

#include <string>
#include <vector>
#include <map>

struct PlanStep {
    std::string action_type;    // Missing in your current .h
    double estimated_impact;    // Missing in your current .h
    bool is_completed;          // Missing in your current .h
};

struct AchievementGoal {
    std::string id;             // Missing in your current .h
    std::string description;
    double target_priority;     // Missing in your current .h
    double current_progress;
    double valence_alignment;
    std::vector<PlanStep> action_plan;
};

class GoalPlanner {
public:
    static AchievementGoal formulate_goal_from_context(const std::string& context, double current_valence);
    static std::vector<PlanStep> plan_goal_achievement(const AchievementGoal& goal);
    static void update_goal_progress(const std::string& goal_id, double progress_delta);
    
    // Add this declaration - it was missing
    static double evaluate_goal_relevance(const AchievementGoal& goal, double current_valence);
    
    static std::string recommend_next_action(const AchievementGoal& goal);
    static double calculate_valence_impact(const std::string& action);

private:
    static std::string generate_goal_id();
    static std::map<std::string, AchievementGoal> active_goals_;
    static std::vector<std::string> achieved_goals_;
};

#endif