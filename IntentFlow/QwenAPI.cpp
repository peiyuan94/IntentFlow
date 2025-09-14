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

#include <opencv2/opencv.hpp>


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
    std::wcout << L"[sendImageQuery] Processing " << imagePaths.size() << L" images" << std::endl;
    
    return executeWithRetry([&]() -> APIResponse {
        // 图片转换为Base64编码
        std::vector<std::string> base64Images;
        for (const auto& imagePath : imagePaths) {
            // 使用更安全的转换函数
            std::wstring wideImagePath = ANSIToUnicodeSafe(imagePath);
            
            std::wcout << L"[sendImageQuery] Processing image: " << wideImagePath << std::endl;
            std::string base64Image = encodeImageToBase64(imagePath);
            if (base64Image.empty()) {
                std::wcout << L"[sendImageQuery] Failed to encode image: " << wideImagePath << std::endl;
                return APIResponse{ false, "", "Failed to encode image: " + imagePath, -1 };
            }
            std::wcout << L"[sendImageQuery] Successfully encoded image. Size: " << base64Image.length() << std::endl;
            base64Images.push_back(base64Image);
        }

        // 构造请求体
        std::wcout << L"[sendImageQuery] Constructing request body with " << base64Images.size() << L" images" << std::endl;
        std::string requestBody = constructRequestBody(base64Images, prompt);
        if (requestBody.empty()) {
            std::wcout << L"[sendImageQuery] Failed to construct request body" << std::endl;
            return APIResponse{ false, "", "Failed to construct request body", -1 };
        }

        // 发送HTTP请求
        std::wcout << L"[sendImageQuery] Sending HTTP request" << std::endl;
        return sendHttpRequest(requestBody);
    });
}

std::string QwenAPI::encodeImageToBase64(const std::string& imagePath) {
    // 使用更安全的转换函数
    std::wstring wideImagePath = ANSIToUnicodeSafe(imagePath);
    
    std::wcout << L"[encodeImageToBase64] Processing image: " << wideImagePath << std::endl;
    
    // Try to scale the image first
    std::wcout << L"[encodeImageToBase64] Attempting to scale image" << std::endl;
    std::string scaledImage = scaleImage(imagePath);
    if (!scaledImage.empty()) {
        std::wcout << L"[encodeImageToBase64] Successfully scaled image, returning scaled version. Size: " << scaledImage.length() << std::endl;
        return scaledImage;  // Return scaled image if successful
    }
    
    // Fall back to original method if scaling fails
    std::wcout << L"[encodeImageToBase64] Scaling failed, falling back to original method" << std::endl;
    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open()) {
        std::wcout << L"[encodeImageToBase64] Failed to open file: " << wideImagePath << std::endl;
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    const std::string& binaryData = buffer.str();
    if (binaryData.empty()) {
        std::wcout << L"[encodeImageToBase64] File is empty: " << wideImagePath << std::endl;
        return "";
    }

    // Use the new base64Encode function
    std::wcout << L"[encodeImageToBase64] Encoding original image to base64. Size: " << binaryData.length() << std::endl;
    std::string result = base64Encode(binaryData);
    std::wcout << L"[encodeImageToBase64] Returning original image base64. Size: " << result.length() << std::endl;
    return result;
}

// Add base64Encode helper function
std::string QwenAPI::base64Encode(const std::string& data) {
	// Base64 encoding table
	static const char* base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	size_t bytes_len = data.size();
	const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(data.c_str());

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

std::string QwenAPI::scaleImage(const std::string& imagePath) {
    try {
        // Convert imagePath to wide string for OpenCV
        std::wstring widePath = ANSIToUnicodeSafe(imagePath);
        std::string utf8Path = UnicodeToUTF8(widePath);
        
        std::wcout << L"[scaleImage] Processing image: " << widePath << std::endl;
        
        // Load the image
        cv::Mat image = cv::imread(utf8Path, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::wcout << L"[scaleImage] Failed to load image: " << widePath << std::endl;
            return "";
        }
        
        // Get original dimensions
        int originalWidth = image.cols;
        int originalHeight = image.rows;
        
        std::wcout << L"[scaleImage] Original image size: " << originalWidth << L"x" << originalHeight << L" for image: " << widePath << std::endl;
        
        // Target dimensions (960x960) - fixed size, not maintaining aspect ratio
        const int targetWidth = 960;
        const int targetHeight = 960;
        
        std::wcout << L"[scaleImage] Target image size: " << targetWidth << L"x" << targetHeight << L" for image: " << widePath << std::endl;
        
        // Resize the image to exact dimensions using high quality interpolation
        cv::Mat resizedImage;
        cv::resize(image, resizedImage, cv::Size(targetWidth, targetHeight), 0, 0, cv::INTER_LANCZOS4);
        
        // Encode the resized image as JPEG in memory
        std::vector<uchar> buffer;
        std::vector<int> params;
        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(90); // JPEG quality
        
        bool success = cv::imencode(".jpg", resizedImage, buffer, params);
        if (!success) {
            std::wcout << L"[scaleImage] Failed to encode image to JPEG for image: " << widePath << std::endl;
            return "";
        }
        
        // Convert the image data to Base64
        std::string imageData(buffer.begin(), buffer.end());
        std::string base64Result = base64Encode(imageData);
        
        std::wcout << L"[scaleImage] Successfully encoded image to base64, size: " << base64Result.length() << L" characters for image: " << widePath << std::endl;
        
        return base64Result;
    }
    catch (const std::exception& e) {
        std::wcout << L"[scaleImage] Exception occurred: " << ANSIToUnicodeSafe(std::string(e.what())) << std::endl;
        return "";
    }
    catch (...) {
        std::wcout << L"[scaleImage] Unknown exception occurred" << std::endl;
        return "";
    }
}

bool QwenAPI::validateApiKey(const std::string& apiKey) {
	// Ensure API key starts with sk-
	return !apiKey.empty() && apiKey.length() > 30 && apiKey.substr(0, 3) == "sk-";
}

std::string QwenAPI::constructRequestBody(const std::vector<std::string>& base64Images, const std::string& prompt) {
	// Convert prompt to wide string then to UTF-8 to ensure proper handling of Chinese characters
	std::wstring widePrompt = ANSIToUnicode(prompt);
	std::string utf8Prompt = UnicodeToUTF8(widePrompt);

	// Manually construct JSON request body without JsonCpp library
	std::string escapedPrompt = utf8Prompt;

	// Escape special characters in prompt for JSON in correct order
	// First escape backslashes
	size_t pos = 0;
	while ((pos = escapedPrompt.find("\\", pos)) != std::string::npos) {
		escapedPrompt.replace(pos, 1, "\\\\");
		pos += 2;
	}

	// Then escape double quotes
	pos = 0;
	while ((pos = escapedPrompt.find("\"", pos)) != std::string::npos) {
		escapedPrompt.replace(pos, 1, "\\\"");
		pos += 2;
	}

	// Replace newlines with \n escape sequence
	pos = 0;
	while ((pos = escapedPrompt.find("\n", pos)) != std::string::npos) {
		escapedPrompt.replace(pos, 1, "\\n");
		pos += 2;
	}

	// Replace carriage returns with \r escape sequence
	pos = 0;
	while ((pos = escapedPrompt.find("\r", pos)) != std::string::npos) {
		escapedPrompt.replace(pos, 1, "\\r");
		pos += 2;
	}

	// Replace tabs with \t escape sequence
	pos = 0;
	while ((pos = escapedPrompt.find("\t", pos)) != std::string::npos) {
		escapedPrompt.replace(pos, 1, "\\t");
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
	body += "}"; // Close input object

	// Add parameters
	body += ",\"parameters\": {";
	body += "\"max_tokens\": 1024";
	body += "}";

	body += "}"; // Close root object

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
	}
	else {
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

// 更安全的ANSI到Unicode转换函数，带有异常处理
std::wstring QwenAPI::ANSIToUnicodeSafe(const std::string& str)
{
    // 检查空字符串
    if (str.empty()) return std::wstring();
    
    try {
        // 检查字符串长度
        if (str.length() > 10000) {  // 限制字符串长度以防止异常
            return L"<path too long>";
        }
        
        // 尝试转换
        int nSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, 0, 0);
        if (nSize <= 0) return L"<conversion failed>";
        
        // 限制最大分配大小
        if (nSize > 10000) {
            return L"<path too long>";
        }
        
        std::wstring wstrResult(nSize, 0);
        int result = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstrResult[0], nSize);
        if (result <= 0) return L"<conversion failed>";
        
        // Remove trailing null character
        if (!wstrResult.empty() && wstrResult.back() == L'\0') {
            wstrResult.pop_back();
        }
        
        return wstrResult;
    }
    catch (...) {
        return L"<conversion exception>";
    }
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
