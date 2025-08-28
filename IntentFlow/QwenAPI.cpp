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
    // ��֤API��Կ
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
        // ��ͼ��ת��ΪBase64����
        std::vector<std::string> base64Images;
        for (const auto& imagePath : imagePaths) {
            std::string base64Image = encodeImageToBase64(imagePath);
            if (base64Image.empty()) {
                return APIResponse{ false, "", "Failed to encode image: " + imagePath, -1 };
            }
            base64Images.push_back(base64Image);
        }

        // ����������
        std::string requestBody = constructRequestBody(base64Images, prompt);
        if (requestBody.empty()) {
            return APIResponse{ false, "", "Failed to construct request body", -1 };
        }

        // ����HTTP����
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

    // �򻯵�Base64����
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
    // ������API��Կͨ����sk-��ͷ
    return !apiKey.empty() && apiKey.length() > 30 && apiKey.substr(0, 3) == "sk-";
}

std::string QwenAPI::constructRequestBody(const std::vector<std::string>& base64Images, const std::string& prompt) {
    // Manually construct JSON request body without JsonCpp library
    std::string escapedPrompt = prompt;
    
    // Escape special characters in prompt for JSON
    size_t pos = 0;
    while ((pos = escapedPrompt.find("\\", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escapedPrompt.find("\"", pos)) != std::string::npos) {
        escapedPrompt.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    std::string body = "{";
    
    // Add model
    body += "\"model\": \"qwen-vl-max\",";
    
    // Add input object
    body += "\"input\": {";
    
    // Add messages array
    body += "\"messages\": [";
    
    // Add user message
    body += "{";
    body += "\"role\": \"user\",";
    body += "\"content\": [";
    
    // Add image content for each image
    for (size_t i = 0; i < base64Images.size(); ++i) {
        if (i > 0) body += ",";
        body += "{";
        body += "\"image\": \"data:image/jpeg;base64," + base64Images[i] + "\"";
        body += "}";
    }
    
    // Add text content
    if (!base64Images.empty()) body += ",";
    body += "{";
    body += "\"text\": \"" + escapedPrompt + "\"";
    body += "}";
    
    body += "]"; // Close content array
    body += "}"; // Close message object
    
    body += "]"; // Close messages array
    body += "},"; // Close input object
    
    // Add parameters
    body += "\"parameters\": {";
    body += "\"max_tokens\": 1024";
    body += "}";
    
    body += "}"; // Close root object
    
    return body;
}

QwenAPI::APIResponse QwenAPI::sendHttpRequest(const std::string& requestBody) {
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    APIResponse response;

    do {
        // �����Ự
        hSession = WinHttpOpen(L"QwenAPI Client/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (!hSession) {
            response.errorMessage = "Failed to create WinHTTP session";
            break;
        }

        // ���ӵ�������
        hConnect = WinHttpConnect(hSession, L"dashscope.aliyuncs.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            response.errorMessage = "Failed to connect to server";
            break;
        }

        // ��������
        hRequest = WinHttpOpenRequest(hConnect, L"POST",
            L"/api/v1/services/aigc/multimodal-generation/generation",
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            response.errorMessage = "Failed to create HTTP request";
            break;
        }

        // ��������ͷ
        std::wstring headers = L"Authorization: Bearer " + std::wstring(config_.apiKey.begin(), config_.apiKey.end()) +
            L"\r\nContent-Type: application/json\r\nAccept: application/json\r\n";

        // ��������
        if (!WinHttpSendRequest(hRequest,
            headers.c_str(), static_cast<DWORD>(headers.length()),
            const_cast<LPVOID>(static_cast<const void*>(requestBody.c_str())),
            static_cast<DWORD>(requestBody.length()),
            static_cast<DWORD>(requestBody.length()),
            0)) {
            response.errorMessage = "Failed to send HTTP request";
            break;
        }

        // ������Ӧ
        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            response.errorMessage = "Failed to receive HTTP response";
            break;
        }

        // ��ȡ״̬��
        DWORD statusCode = 0;
        DWORD dwSize = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

        response.statusCode = static_cast<int>(statusCode);

        // ��ȡ��Ӧ����
        std::string responseContent;
        DWORD dwDownloaded;
        char buffer[4096];
        do {
            if (WinHttpReadData(hRequest, buffer, sizeof(buffer), &dwDownloaded)) {
                responseContent.append(buffer, dwDownloaded);
            }
            else {
                response.errorMessage = "Failed to read response data";
                break;
            }
        } while (dwDownloaded > 0);

        // ������Ӧ
        response = processResponse(responseContent, response.statusCode);

        // �ɹ����
        response.success = true;

    } while (false);

    // ������Դ
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return response;
}

QwenAPI::APIResponse QwenAPI::processResponse(const std::string& responseStr, int statusCode) {
    APIResponse response;
    response.statusCode = statusCode;

    if (statusCode >= 200 && statusCode < 300) {
        // Simple response processing without JSON parsing library
        response.success = true;
        response.content = responseStr;
    }
    else {
        response.success = false;
        response.errorMessage = "HTTP error " + std::to_string(statusCode) + ": " + responseStr;
    }

    return response;
}

QwenAPI::APIResponse QwenAPI::executeWithRetry(const std::function<APIResponse()>& operation) {
    APIResponse lastResponse;

    for (int attempt = 0; attempt <= config_.maxRetries; ++attempt) {
        lastResponse = operation();

        // ����ɹ����ߴﵽ������Դ������򷵻ؽ��
        if (lastResponse.success || attempt == config_.maxRetries) {
            return lastResponse;
        }

        // ����ĳЩ���󣬿���ѡ������
        if (lastResponse.statusCode == 401 || lastResponse.statusCode == 403) {
            // ��֤��������������
            return lastResponse;
        }

        // �ȴ�һ��ʱ������ԣ�ָ���˱ܣ�
        if (attempt < config_.maxRetries) {
            int delayMs = (1 << attempt) * 1000; // 1��, 2��, 4��...
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    return lastResponse;
}