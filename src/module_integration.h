// module_integration.h - Easy integration hooks for your existing code
#ifndef MODULE_INTEGRATION_H
#define MODULE_INTEGRATION_H

#include <string>

// Forward declare State to avoid circular dependency issues
struct State;

// Include other headers AFTER forward declaration
#include "language_module.h"
#include "consciousness_module.h"
#include "metacognition_module.h"

namespace module_integration {
    // Initialize all modules (call once at startup after loadEnglishDataset)
    void init_all_modules();

    // Update all modules each cycle (call in your main loop after existing updates)
    void update_all_modules(State& S);

    // Enhanced response generation (hook this into your generateResponse)
    std::string enhanced_generate_response(const std::string& input, State& S);

    // Add linguistic report to UI (call in draw_ui)
    std::string get_linguistic_report();

    // Add consciousness report to UI
    std::string get_consciousness_report();

    // Add metacognitive report to UI
    std::string get_metacognitive_report();

    // Combined sentience calculation (enhanced version of your calcSentienceRatio)
    double calc_enhanced_sentience();
}

#endif // MODULE_INTEGRATION_H