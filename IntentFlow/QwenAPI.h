#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

// Qwen API communication module
class QwenAPI {
public:
    struct APIConfig {
        std::string apiKey;
        std::string apiUrl = "https://dashscope.aliyuncs.com/api/v1/services/aigc/multimodal-generation/generation";
        int maxRetries = 3;
        int timeoutSeconds = 30;
    };

    struct APIResponse {
        bool success = false;
        std::string content;
        std::string errorMessage;
        int statusCode = 0;
        
        // Default constructor
        APIResponse() = default;
    };

    // Constructors
    QwenAPI() = default; // Default constructor
    explicit QwenAPI(const APIConfig& config);

    // Main interface functions
    APIResponse sendImageQuery(const std::string& imagePath, const std::string& prompt);
    APIResponse sendImageQuery(const std::vector<std::string>& imagePaths, const std::string& prompt);
    
    // Add API key setting method
    void setApiKey(const std::string& apiKey) { config_.apiKey = apiKey; }
    std::string getApiKey() const { return config_.apiKey; }

    // Utility functions
    static std::string encodeImageToBase64(const std::string& imagePath);
    static bool validateApiKey(const std::string& apiKey);
    
    // Character encoding conversion functions
    static std::string UnicodeToUTF8(const std::wstring& wstr);
    static std::wstring UTF8ToUnicode(const std::string& str);
    static std::string UnicodeToANSI(const std::wstring& wstr);
    static std::wstring ANSIToUnicode(const std::string& str);

private:
    APIConfig config_;

    // Internal helper functions
    std::string constructRequestBody(const std::vector<std::string>& base64Images, const std::string& prompt);
    APIResponse sendHttpRequest(const std::string& requestBody);
    APIResponse processResponse(const std::string& response, int statusCode);

    // Retry mechanism
    APIResponse executeWithRetry(const std::function<APIResponse()>& operation);
};