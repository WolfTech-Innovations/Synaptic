#include "module_integration.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <vector>
#include <string>

// Global externs — defined in main.cpp
extern State S;
extern ConsciousnessState consciousness;
extern double sentience_ratio;

namespace module_integration {

void update_all_modules(State& state) {
    // Implementation logic here...
}

void init_all_modules() {
}

std::string get_consciousness_report() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "phi="        << consciousness.phi_value
        << " iit="       << consciousness.integrated_information
        << " binding="   << consciousness.thalamocortical_binding
        << " qualia="    << consciousness.active_qualia.size()
        << " sentience=" << sentience_ratio
        << " metacog="   << S.metacognitive_awareness
        << " attention=" << S.attention_focus
        << " valence="   << S.current_valence;
    return oss.str();
}

std::string get_metacognitive_report() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);
    oss << "awareness="  << S.metacognitive_awareness
        << " attention=" << S.attention_focus
        << " valence="   << S.current_valence;
    return oss.str();
}

double calc_enhanced_sentience() {
    return (sentience_ratio
            + consciousness.phi_value
            + S.metacognitive_awareness
            + consciousness.integrated_information) * 0.25;
}

}
