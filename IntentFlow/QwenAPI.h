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

    // Utility functions
    static std::string encodeImageToBase64(const std::string& imagePath);
    static bool validateApiKey(const std::string& apiKey);

private:
    APIConfig config_;

    // Internal helper functions
    std::string constructRequestBody(const std::vector<std::string>& base64Images, const std::string& prompt);
    APIResponse sendHttpRequest(const std::string& requestBody);
    APIResponse processResponse(const std::string& response, int statusCode);

    // Retry mechanism
    APIResponse executeWithRetry(const std::function<APIResponse()>& operation);
};