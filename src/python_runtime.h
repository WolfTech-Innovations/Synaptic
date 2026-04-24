// python_runtime.h - Embedded Python runtime in DIGITZ
#ifndef PYTHON_RUNTIME_H
#define PYTHON_RUNTIME_H

#include <Python.h>
#include <string>
#include <vector>
#include <map>

// Forward declaration
struct State;

class PythonRuntime {
private:
    bool initialized = false;
    PyObject* sys_module = nullptr;
    PyObject* main_module = nullptr;
    
    std::map<std::string, PyObject*> loaded_modules;
    
public:
    PythonRuntime() = default;
    ~PythonRuntime();
    
    // Initialize Python interpreter with modules/ directory
    bool init(const char* modules_path = "modules");
    
    // Shutdown Python
    void shutdown();
    
    // Check if initialized
    bool is_ready() const { return initialized; }
    
    // Load a Python module from modules/ directory
    PyObject* load_module(const std::string& module_name);
    
    // Call a Python function
    PyObject* call_function(const std::string& module_name, 
                           const std::string& function_name,
                           PyObject* args = nullptr);
    
    // Helper: Call function with string argument
    std::string call_string_function(const std::string& module_name,
                                    const std::string& function_name, 
                                    const std::string& arg);
    
    // Helper: Call function with State pointer (advanced)
    void call_with_state(const std::string& module_name,
                        const std::string& function_name,
                        State* state);
    
    // Execute arbitrary Python code
    bool exec(const std::string& code);
    
    // Get Python error message
    std::string get_error();
};

// Global Python runtime
extern PythonRuntime g_python;

// Convenience functions
std::string python_enhance_text(const std::string& text);
void python_process_sensory(State* state);
std::string python_generate_response(const std::string& input, State* state);

#endif
