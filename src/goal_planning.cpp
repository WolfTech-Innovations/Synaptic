#include "goal_planning.h"
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <cmath>

// Adding necessary using declarations for cleaner code
using std::string;
using std::vector;
using std::map;

// Correctly scope the static members
map<string, AchievementGoal> GoalPlanner::active_goals_;
vector<string> GoalPlanner::achieved_goals_;

// Reference the external random engines
extern std::random_device rd;
extern std::mt19937 rng;

AchievementGoal GoalPlanner::formulate_goal_from_context(const string& context, double current_valence) {
    AchievementGoal goal;
    goal.id = generate_goal_id();
    goal.description = "Reason about: " + context;
    goal.target_priority = 0.5 + (std::abs(current_valence) * 0.5);
    goal.current_progress = 0.0;
    goal.valence_alignment = current_valence;
    goal.action_plan = plan_goal_achievement(goal);
    
    active_goals_[goal.id] = goal;
    return goal;
}

vector<PlanStep> GoalPlanner::plan_goal_achievement(const AchievementGoal& goal) {
    (void)goal; // Silence unused parameter warning
    vector<PlanStep> plan;
    
    vector<string> actions = {
        "analyze_components",
        "synthesize_meaning",
        "evaluate_consequences"
    };
    
    for (size_t i = 0; i < actions.size(); ++i) {
        PlanStep step;
        step.action_type = actions[i];
        step.estimated_impact = 0.2;
        step.is_completed = false;
        plan.push_back(step);
    }
    
    return plan;
}

void GoalPlanner::update_goal_progress(const string& goal_id, double progress_delta) {
    if (active_goals_.count(goal_id)) {
        active_goals_[goal_id].current_progress += progress_delta;
        if (active_goals_[goal_id].current_progress >= 1.0) {
            active_goals_[goal_id].current_progress = 1.0;
            achieved_goals_.push_back(goal_id);
        }
    }
}

double GoalPlanner::evaluate_goal_relevance(const AchievementGoal& goal, double current_valence) {
    // Fixed: Use std::abs for floating point values
    double alignment = 1.0 - std::abs(goal.valence_alignment - current_valence);
    return goal.target_priority * alignment;
}

string GoalPlanner::recommend_next_action(const AchievementGoal& goal) {
    if (goal.action_plan.empty()) return "contemplate";
    
    // Fixed: Use std::min and cast correctly
    int next_step_idx = std::min((int)(goal.current_progress * goal.action_plan.size()), 
                                 (int)goal.action_plan.size() - 1);
                                 
    return goal.action_plan[next_step_idx].action_type;
}

string GoalPlanner::generate_goal_id() {
    static int counter = 0;
    // Fixed: Use std::to_string
    return "goal_" + std::to_string(counter++);
}

double GoalPlanner::calculate_valence_impact(const string& action) {
    if (action.find("analyze") != string::npos) return 0.05;
    if (action.find("synthesize") != string::npos) return 0.1;
    if (action.find("evaluate") != string::npos) return 0.15;
    return 0.01;
}