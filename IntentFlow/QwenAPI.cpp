#include "pch.h"
#include "QwenAPI.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>
#include <codecvt>
#include <locale>

QwenAPI::QwenAPI(const APIConfig& config) : config_(config) {
    // Validate API key
    if (!validateApiKey(config_.apiKey)) {
        throw std::invalid_argument("Invalid API key format");
    }
}

QwenAPI::APIResponse QwenAPI::sendImageQuery(const std::string& imagePath, const std::string& prompt) {
    std::vector<std::string> imagePaths = { imagePath };
    return sendImageQuery(imagePaths, prompt);
}

QwenAPI::APIResponse QwenAPI::sendImageQuery(const std::vector<std::string>& imagePaths, const std::string& prompt) {
    return executeWithRetry([&]() -> APIResponse {
        // 图片转换为Base64编码
        std::vector<std::string> base64Images;
        for (const auto& imagePath : imagePaths) {
            std::string base64Image = encodeImageToBase64(imagePath);
            if (base64Image.empty()) {
                return APIResponse{ false, "", "Failed to encode image: " + imagePath, -1 };
            }
            base64Images.push_back(base64Image);
        }

        // 构造请求体
        std::string requestBody = constructRequestBody(base64Images, prompt);
        if (requestBody.empty()) {
            return APIResponse{ false, "", "Failed to construct request body", -1 };
        }

        // 发送HTTP请求
        return sendHttpRequest(requestBody);
        });
}

std::string QwenAPI::encodeImageToBase64(const std::string& imagePath) {
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    const std::string& binaryData = buffer.str();
    if (binaryData.empty()) {
        return "";
    }

    // 转换成Base64编码
    static const char* base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    size_t bytes_len = binaryData.size();
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(binaryData.c_str());

    while (bytes_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

bool QwenAPI::validateApiKey(const std::string& apiKey) {
    // Ensure API key starts with sk-
    return !apiKey.empty() && apiKey.length() > 30 && apiKey.substr(0, 3) == "sk-";
}

std::string QwenAPI::constructRequestBody(const std::vector<std::string>& base64Images, const std::string& prompt) {
    // Manually construct JSON request body without JsonCpp library
    std::string escapedPrompt = prompt;
    
    // 转义特殊字符以确保生成有效的JSON格式
    // 注意转义顺序非常重要：
    // 1. 首先转义反斜杠，因为它是转义字符
    // 2. 然后转义双引号，因为它们用于字符串界定
    // 3. 最后处理其他特殊字符
    
    // 转义反斜杠
    size_t pos = 0;
    while ((pos = escapedPrompt.find("\\", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    // 转义双引号
    pos = 0;
    while ((pos = escapedPrompt.find("\"", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // 转义换行符
    pos = 0;
    while ((pos = escapedPrompt.find("\n", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    // 转义回车符
    pos = 0;
    while ((pos = escapedPrompt.find("\r", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\r");
        pos += 2;
    }
    
    // 转义制表符
    pos = 0;
    while ((pos = escapedPrompt.find("\t", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\t");
        pos += 2;
    }
    
    // 构建JSON请求体
    std::string body;
    
    // 开始根对象
    body += "{";
    
    // 添加模型信息
    body += "\"model\": \"qwen-vl-max\",";
    
    // 开始输入对象
    body += "\"input\": {";
    
    // 开始消息数组
    body += "\"messages\": [";
    
    // 添加用户消息
    body += "{";
    body += "\"role\": \"user\",";
    body += "\"content\": [";
    
    // 添加每个图像内容
    for (size_t i = 0; i < base64Images.size(); ++i) {
        if (i > 0) body += ",";
        body += "{";
        body += "\"image\": \"data:image/jpeg;base64," + base64Images[i] + "\"";
        body += "}";
    }
    
    // 添加文本内容
    if (!base64Images.empty()) body += ",";
    body += "{";
    body += "\"text\": \"" + escapedPrompt + "\"";
    body += "}";
    
    // 结束content数组和消息对象
    body += "]}";
    
    // 结束messages数组
    body += "]";
    
    // 结束input对象
    body += "}";
    
    // 添加参数
    body += ",\"parameters\": {";
    body += "\"max_tokens\": 1024";
    body += "}";
    
    // 结束根对象
    body += "}";
    
    return body;
}

QwenAPI::APIResponse QwenAPI::sendHttpRequest(const std::string& requestBody) {
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    APIResponse result;

    do {
        // Create session
        hSession = WinHttpOpen(L"QwenAPI Client/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (!hSession) {
            result.errorMessage = "Failed to create WinHTTP session";
            break;
        }

        // Connect to server
        hConnect = WinHttpConnect(hSession, L"dashscope.aliyuncs.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            result.errorMessage = "Failed to connect to server";
            break;
        }

        // Create request
        hRequest = WinHttpOpenRequest(hConnect, L"POST",
            L"/api/v1/services/aigc/multimodal-generation/generation",
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            result.errorMessage = "Failed to create HTTP request";
            break;
        }

        // Set request headers
        std::wstring headers = L"Authorization: Bearer " + std::wstring(config_.apiKey.begin(), config_.apiKey.end()) +
            L"\r\nContent-Type: application/json\r\nAccept: application/json\r\n";

        // Convert headers to LPCWSTR
        LPCWSTR pwszHeaders = headers.c_str();
        DWORD dwHeadersLength = static_cast<DWORD>(headers.length());

        // Send request with proper headers
        if (!WinHttpSendRequest(hRequest, pwszHeaders, dwHeadersLength,
            (LPVOID)requestBody.c_str(), static_cast<DWORD>(requestBody.length()),
            static_cast<DWORD>(requestBody.length()), 0)) {
            result.errorMessage = "Failed to send HTTP request: " + std::to_string(GetLastError());
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            return result;
        }

        // Receive response
        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            result.errorMessage = "Failed to receive HTTP response: " + std::to_string(GetLastError());
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            return result;
        }

        // Get response status code
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

        result.statusCode = static_cast<int>(dwStatusCode);

        // Read response content
        std::string responseBuffer;
        DWORD dwDownloaded = 0;
        std::vector<char> buffer(10240); // 10KB buffer

        do {
            if (!WinHttpReadData(hRequest, (LPVOID)buffer.data(), static_cast<DWORD>(buffer.size()), &dwDownloaded)) {
                result.errorMessage = "Error reading HTTP data: " + std::to_string(GetLastError());
                break;
            }
            
            if (dwDownloaded > 0) {
                responseBuffer.append(buffer.data(), dwDownloaded);
            }
        } while (dwDownloaded > 0);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);

        // Process response
        return processResponse(responseBuffer, result.statusCode);
    } while (false);

    // Clean up handles in case of error
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return result;
}

QwenAPI::APIResponse QwenAPI::processResponse(const std::string& response, int statusCode) {
    APIResponse result;
    result.statusCode = statusCode;
    
    if (statusCode == 200) {
        result.success = true;
        // Ensure response content correctly handles UTF-8 encoding
        result.content = response;
    } else {
        result.success = false;
        result.errorMessage = response;
    }
    
    return result;
}

QwenAPI::APIResponse QwenAPI::executeWithRetry(const std::function<APIResponse()>& operation) {
    APIResponse lastResponse;

    for (int attempt = 0; attempt <= config_.maxRetries; ++attempt) {
        lastResponse = operation();

        // 如果成功或者达到最大重试次数，返回结果
        if (lastResponse.success || attempt == config_.maxRetries) {
            return lastResponse;
        }

        // 如果是某些错误，直接返回结果
        if (lastResponse.statusCode == 401 || lastResponse.statusCode == 403) {
            // 认证错误，直接返回结果
            return lastResponse;
        }

        // 等待一段时间，然后重试，指数退避
        if (attempt < config_.maxRetries) {
            int delayMs = (1 << attempt) * 1000; // 1秒, 2秒, 4秒...
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    return lastResponse;
}

std::string QwenAPI::UnicodeToUTF8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int nSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    if (nSize <= 0) return std::string();
    std::string strResult(nSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &strResult[0], nSize, 0, 0);
    // Remove trailing null character
    if (!strResult.empty() && strResult.back() == '\0') {
        strResult.pop_back();
    }
    return strResult;
}

std::wstring QwenAPI::UTF8ToUnicode(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int nSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    if (nSize <= 0) return std::wstring();
    std::wstring wstrResult(nSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstrResult[0], nSize);
    // Remove trailing null character
    if (!wstrResult.empty() && wstrResult.back() == L'\0') {
        wstrResult.pop_back();
    }
    return wstrResult;
}

std::string QwenAPI::UnicodeToANSI(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int nSize = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    if (nSize <= 0) return std::string();
    std::string strResult(nSize, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &strResult[0], nSize, 0, 0);
    // Remove trailing null character
    if (!strResult.empty() && strResult.back() == '\0') {
        strResult.pop_back();
    }
    return strResult;
}

std::wstring QwenAPI::ANSIToUnicode(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int nSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
    if (nSize <= 0) return std::wstring();
    std::wstring wstrResult(nSize, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstrResult[0], nSize);
    // Remove trailing null character
    if (!wstrResult.empty() && wstrResult.back() == L'\0') {
        wstrResult.pop_back();
    }
    return wstrResult;
}
