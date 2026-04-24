// python_runtime.cpp - Python embedding implementation
#include "python_runtime.h"
#include <iostream>
#include <cstring>

PythonRuntime g_python;

bool PythonRuntime::init(const char* modules_path) {
    if (initialized) {
        return true;
    }
    
    std::cout << "Initializing Python runtime..." << std::endl;
    
    // Initialize Python interpreter
    Py_Initialize();
    
    if (!Py_IsInitialized()) {
        std::cerr << "Failed to initialize Python" << std::endl;
        return false;
    }
    
    // Add modules/ directory to Python path
    PyObject* sys_path = PySys_GetObject("path");
    if (sys_path) {
        PyObject* path = PyUnicode_FromString(modules_path);
        PyList_Append(sys_path, path);
        Py_DECREF(path);
        
        // Also add current directory
        path = PyUnicode_FromString(".");
        PyList_Append(sys_path, path);
        Py_DECREF(path);
    }
    
    // Get main module
    main_module = PyImport_ImportModule("__main__");
    if (!main_module) {
        std::cerr << "Failed to import __main__" << std::endl;
        Py_Finalize();
        return false;
    }
    
    initialized = true;
    std::cout << "✓ Python runtime initialized" << std::endl;
    std::cout << "  Module path: " << modules_path << "/" << std::endl;
    
    return true;
}

void PythonRuntime::shutdown() {
    if (!initialized) {
        return;
    }
    
    // Release loaded modules
    for (auto& [name, module] : loaded_modules) {
        Py_XDECREF(module);
    }
    loaded_modules.clear();
    
    Py_XDECREF(main_module);
    
    // Finalize Python
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    
    initialized = false;
    std::cout << "Python runtime shutdown" << std::endl;
}

PythonRuntime::~PythonRuntime() {
    shutdown();
}

PyObject* PythonRuntime::load_module(const std::string& module_name) {
    if (!initialized) {
        std::cerr << "Python not initialized" << std::endl;
        return nullptr;
    }
    
    // Check if already loaded
    auto it = loaded_modules.find(module_name);
    if (it != loaded_modules.end()) {
        return it->second;
    }
    
    // Import module
    PyObject* module = PyImport_ImportModule(module_name.c_str());
    if (!module) {
        std::cerr << "Failed to load module: " << module_name << std::endl;
        std::cerr << get_error() << std::endl;
        return nullptr;
    }
    
    loaded_modules[module_name] = module;
    std::cout << "✓ Loaded Python module: " << module_name << std::endl;
    
    return module;
}

PyObject* PythonRuntime::call_function(const std::string& module_name,
                                       const std::string& function_name,
                                       PyObject* args) {
    PyObject* module = load_module(module_name);
    if (!module) {
        return nullptr;
    }
    
    // Get function
    PyObject* func = PyObject_GetAttrString(module, function_name.c_str());
    if (!func || !PyCallable_Check(func)) {
        std::cerr << "Function not found or not callable: " 
                  << module_name << "." << function_name << std::endl;
        Py_XDECREF(func);
        return nullptr;
    }
    
    // Call function
    PyObject* result = nullptr;
    if (args) {
        result = PyObject_CallObject(func, args);
    } else {
        result = PyObject_CallObject(func, nullptr);
    }
    
    Py_DECREF(func);
    
    if (!result) {
        std::cerr << "Error calling " << module_name << "." << function_name << std::endl;
        std::cerr << get_error() << std::endl;
    }
    
    return result;
}

std::string PythonRuntime::call_string_function(const std::string& module_name,
                                                const std::string& function_name,
                                                const std::string& arg) {
    // Build args
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, PyUnicode_FromString(arg.c_str()));
    
    // Call function
    PyObject* result = call_function(module_name, function_name, args);
    Py_DECREF(args);
    
    if (!result) {
        return arg;  // Return original on error
    }
    
    // Convert result to string
    std::string ret_str;
    if (PyUnicode_Check(result)) {
        const char* str = PyUnicode_AsUTF8(result);
        if (str) {
            ret_str = str;
        }
    }
    
    Py_DECREF(result);
    return ret_str.empty() ? arg : ret_str;
}

void PythonRuntime::call_with_state(const std::string& module_name,
                                    const std::string& function_name,
                                    State* state) {
    // Convert State pointer to Python capsule
    PyObject* capsule = PyCapsule_New(state, "State", nullptr);
    
    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, capsule);
    
    PyObject* result = call_function(module_name, function_name, args);
    Py_DECREF(args);
    Py_XDECREF(result);
}

bool PythonRuntime::exec(const std::string& code) {
    if (!initialized) {
        return false;
    }
    
    PyObject* result = PyRun_String(code.c_str(), 
                                    Py_file_input,
                                    PyModule_GetDict(main_module),
                                    PyModule_GetDict(main_module));
    
    if (!result) {
        std::cerr << "Python exec error:" << std::endl;
        std::cerr << get_error() << std::endl;
        return false;
    }
    
    Py_DECREF(result);
    return true;
}

std::string PythonRuntime::get_error() {
    if (!PyErr_Occurred()) {
        return "";
    }
    
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    
    std::string error_msg;
    
    if (pvalue) {
        PyObject* str = PyObject_Str(pvalue);
        if (str) {
            const char* msg = PyUnicode_AsUTF8(str);
            if (msg) {
                error_msg = msg;
            }
            Py_DECREF(str);
        }
    }
    
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    
    return error_msg;
}

// Convenience functions

std::string python_enhance_text(const std::string& text) {
    if (!g_python.is_ready()) {
        return text;
    }
    
    return g_python.call_string_function("ai_enhance", "enhance_text", text);
}

void python_process_sensory(State* state) {
    if (!g_python.is_ready()) {
        return;
    }
    
    g_python.call_with_state("sensory", "process", state);
}

std::string python_generate_response(const std::string& input, State* state) {
    if (!g_python.is_ready()) {
        return input;
    }
    
    // More complex call - pass both string and state
    PyObject* module = g_python.load_module("response_gen");
    if (!module) {
        return input;
    }
    
    PyObject* func = PyObject_GetAttrString(module, "generate");
    if (!func || !PyCallable_Check(func)) {
        Py_XDECREF(func);
        return input;
    }
    
    // Build args
    PyObject* args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, PyUnicode_FromString(input.c_str()));
    PyTuple_SetItem(args, 1, PyCapsule_New(state, "State", nullptr));
    
    PyObject* result = PyObject_CallObject(func, args);
    Py_DECREF(args);
    Py_DECREF(func);
    
    if (!result) {
        return input;
    }
    
    std::string response;
    if (PyUnicode_Check(result)) {
        const char* str = PyUnicode_AsUTF8(result);
        if (str) {
            response = str;
        }
    }
    
    Py_DECREF(result);
    return response.empty() ? input : response;
}
