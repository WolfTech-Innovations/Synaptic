#ifndef UAC_H
#define UAC_H

#include <string>
#include <type_traits>
#include <limits>
#include <cstddef>
#include <stdexcept>

// Universal safe string-to-number conversion function (uac: universal ASCII converter).
// Supports common numeric types via implicit conversion: int, long, long long, unsigned int, 
// unsigned long, unsigned long long, float, double, long double.
// Returns a lightweight proxy object that auto-detects the desired output type from assignment context.
// Advanced sanitization (null pointers, empty strings, partial parses, out-of-range) returns 
// default-constructed value (T{}) silently.
// Locale-independent; uses std::stoi/stol/etc. family for robustness. For C++11+.

class UACProxy {
private:
    std::string str_;
    bool is_valid_;

    // Internal conversion implementation with proper error handling
    template<typename T>
    T convert() const {
        // C++11 compatible type check
        static_assert(std::is_arithmetic<T>::value, "uac: T must be an arithmetic type");

        if (!is_valid_ || str_.empty()) {
            return T{};
        }

        try {
            size_t pos = 0;
            
            // Integer types
            if (std::is_same<T, int>::value) {
                long value = std::stol(str_, &pos);
                if (pos != str_.size()) return T{};
                if (value < std::numeric_limits<int>::min() || 
                    value > std::numeric_limits<int>::max()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, long>::value) {
                long value = std::stol(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, long long>::value) {
                long long value = std::stoll(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, unsigned int>::value) {
                // Handle negative strings for unsigned types
                if (!str_.empty() && str_[0] == '-') return T{};
                unsigned long value = std::stoul(str_, &pos);
                if (pos != str_.size()) return T{};
                if (value > std::numeric_limits<unsigned int>::max()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, unsigned long>::value) {
                if (!str_.empty() && str_[0] == '-') return T{};
                unsigned long value = std::stoul(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, unsigned long long>::value) {
                if (!str_.empty() && str_[0] == '-') return T{};
                unsigned long long value = std::stoull(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            // Floating point types
            else if (std::is_same<T, float>::value) {
                float value = std::stof(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, double>::value) {
                double value = std::stod(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            else if (std::is_same<T, long double>::value) {
                long double value = std::stold(str_, &pos);
                if (pos != str_.size()) return T{};
                return static_cast<T>(value);
            }
            else {
                // Unsupported type
                return T{};
            }
        } catch (const std::invalid_argument&) {
            return T{};
        } catch (const std::out_of_range&) {
            return T{};
        } catch (...) {
            return T{};
        }
        
        return T{};
    }

public:
    // Constructor from std::string (copy to ensure lifetime)
    explicit UACProxy(const std::string& str) : str_(str), is_valid_(!str.empty()) {}

    // Constructor from const char* with null safety
    explicit UACProxy(const char* str) : is_valid_(false) {
        if (str != nullptr && *str != '\0') {
            str_ = str;
            is_valid_ = true;
        }
    }

    // Constructor from nullptr_t
    explicit UACProxy(std::nullptr_t) : is_valid_(false) {}

    // Copy constructor
    UACProxy(const UACProxy& other) : str_(other.str_), is_valid_(other.is_valid_) {}

    // Copy assignment
    UACProxy& operator=(const UACProxy& other) {
        if (this != &other) {
            str_ = other.str_;
            is_valid_ = other.is_valid_;
        }
        return *this;
    }

    // Implicit conversion operators for supported types
    operator int() const { return convert<int>(); }
    operator long() const { return convert<long>(); }
    operator long long() const { return convert<long long>(); }
    operator unsigned int() const { return convert<unsigned int>(); }
    operator unsigned long() const { return convert<unsigned long>(); }
    operator unsigned long long() const { return convert<unsigned long long>(); }
    operator float() const { return convert<float>(); }
    operator double() const { return convert<double>(); }
    operator long double() const { return convert<long double>(); }
};

// Factory functions to create the proxy
inline UACProxy uac(const std::string& str) {
    return UACProxy(str);
}

inline UACProxy uac(const char* str) {
    return UACProxy(str);
}

inline UACProxy uac(std::nullptr_t) {
    return UACProxy(nullptr);
}

// Helper function for converting with custom default
template<typename T>
T uac_convert_with_default(const std::string& str, T default_value) {
    static_assert(std::is_arithmetic<T>::value, "uac: T must be an arithmetic type");
    
    if (str.empty()) {
        return default_value;
    }

    try {
        size_t pos = 0;
        
        if (std::is_same<T, int>::value) {
            long value = std::stol(str, &pos);
            if (pos != str.size()) return default_value;
            if (value < std::numeric_limits<int>::min() || 
                value > std::numeric_limits<int>::max()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, long>::value) {
            long value = std::stol(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, long long>::value) {
            long long value = std::stoll(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, unsigned int>::value) {
            if (!str.empty() && str[0] == '-') return default_value;
            unsigned long value = std::stoul(str, &pos);
            if (pos != str.size()) return default_value;
            if (value > std::numeric_limits<unsigned int>::max()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, unsigned long>::value) {
            if (!str.empty() && str[0] == '-') return default_value;
            unsigned long value = std::stoul(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, unsigned long long>::value) {
            if (!str.empty() && str[0] == '-') return default_value;
            unsigned long long value = std::stoull(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, float>::value) {
            float value = std::stof(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, double>::value) {
            double value = std::stod(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        else if (std::is_same<T, long double>::value) {
            long double value = std::stold(str, &pos);
            if (pos != str.size()) return default_value;
            return static_cast<T>(value);
        }
        
        return default_value;
    } catch (...) {
        return default_value;
    }
}

// Explicit templated overload for custom default value
template<typename T>
T uac(const std::string& str, T default_value) {
    return uac_convert_with_default<T>(str, default_value);
}

template<typename T>
T uac(const char* str, T default_value) {
    if (str == nullptr || *str == '\0') {
        return default_value;
    }
    return uac_convert_with_default<T>(std::string(str), default_value);
}

#endif // UAC_H