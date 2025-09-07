#include "pch.h"
#include "TestInterface.h"
#include "QwenAPI.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>
#include <functional>

using std::min;

TestInterface::TestInterface(std::shared_ptr<QwenAPI> qwenAPI) 
    : qwenAPI_(qwenAPI) {
    statistics_ = {0, 0, 0, 0.0, 0.0};
}

void TestInterface::Initialize() {
    // Initialization operations (if needed)
    statistics_ = {0, 0, 0, 0.0, 0.0};
}

TestInterface::TestResult TestInterface::executeTest(const std::string& imagePath, 
                                                    const std::string& prompt, 
                                                    TaskType taskType, 
                                                    const std::string& input) {
    TestResult result;
    result.taskType = taskType;
    result.input = input;
    result.imagePath = imagePath;
    result.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Increase total test count
    statistics_.totalTests++;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Call QwenAPI to execute test
    QwenAPI::APIResponse response = qwenAPI_->sendImageQuery(imagePath, prompt);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Update statistics
    statistics_.averageResponseTime = (statistics_.averageResponseTime * (statistics_.totalTests - 1) + duration.count()) / statistics_.totalTests;
    
    if (response.success) {
        result.success = true;
        result.output = response.content;
        statistics_.successfulTests++;
    } else {
        result.success = false;
        result.errorMessage = response.errorMessage;
        statistics_.failedTests++;
    }
    
    // Calculate success rate
    statistics_.successRate = static_cast<double>(statistics_.successfulTests) / statistics_.totalTests * 100;
    
    return result;
}

std::string TestInterface::buildGroundingPrompt(const std::string& question) {
    // Ensure Chinese questions are handled correctly
    std::wstring unicodeQuestion = QwenAPI::UTF8ToUnicode(question);
    std::string utf8Prompt = R"({
        "role": "user",
        "content": [
            {
                "image": "IMAGE_PLACEHOLDER"
            },
            {
                "text": "Please provide the bounding box coordinates for the UI element mentioned in the question: )" + question + R"("
            }
        ]
    })";
    return utf8Prompt;
}

std::string TestInterface::buildReferringPrompt(int x, int y) {
    std::string utf8Prompt = R"({
        "role": "user",
        "content": [
            {
                "image": "IMAGE_PLACEHOLDER"
            },
            {
                "text": "What UI element is located at coordinates ()" + std::to_string(x) + ", " + std::to_string(y) + R"()? Please describe its function."
            }
        ]
    })";
    return utf8Prompt;
}

std::string TestInterface::buildVQAPrompt(const std::string& question) {
    // Ensure Chinese questions are handled correctly
    std::wstring unicodeQuestion = QwenAPI::UTF8ToUnicode(question);
    std::string utf8Prompt = R"({
        "role": "user",
        "content": [
            {
                "image": "IMAGE_PLACEHOLDER"
            },
            {
                "text": "Answer the following question about the UI: )" + question + R"("
            }
        ]
    })";
    return utf8Prompt;
}

TestInterface::TestResult TestInterface::executeGroundingTest(const std::string& imagePath, const std::string& question) {
    std::string prompt = buildGroundingPrompt(question);
    return executeTest(imagePath, prompt, TaskType::GUI_GROUNDING, question);
}

TestInterface::TestResult TestInterface::executeReferringTest(const std::string& imagePath, int x, int y) {
    std::string prompt = buildReferringPrompt(x, y);
    return executeTest(imagePath, prompt, TaskType::GUI_REFERRING, std::to_string(x) + "," + std::to_string(y));
}

TestInterface::TestResult TestInterface::executeVQATest(const std::string& imagePath, const std::string& question) {
    std::string prompt = buildVQAPrompt(question);
    return executeTest(imagePath, prompt, TaskType::GUI_VQA, question);
}

std::vector<TestInterface::TestResult> TestInterface::executeBatchGroundingTest(
    const std::vector<std::string>& imagePaths, 
    const std::vector<std::string>& questions) {
    
    std::vector<TestResult> results;
    size_t count = min(imagePaths.size(), questions.size());
    
    for (size_t i = 0; i < count; ++i) {
        results.push_back(executeGroundingTest(imagePaths[i], questions[i]));
    }
    
    return results;
}

std::vector<TestInterface::TestResult> TestInterface::executeBatchReferringTest(
    const std::vector<std::string>& imagePaths,
    const std::vector<std::pair<int, int>>& coordinates) {
    
    std::vector<TestResult> results;
    size_t count = min(imagePaths.size(), coordinates.size());
    
    for (size_t i = 0; i < count; ++i) {
        results.push_back(executeReferringTest(imagePaths[i], coordinates[i].first, coordinates[i].second));
    }
    
    return results;
}

std::vector<TestInterface::TestResult> TestInterface::executeBatchVQATest(
    const std::vector<std::string>& imagePaths,
    const std::vector<std::string>& questions) {
    
    std::vector<TestResult> results;
    size_t count = min(imagePaths.size(), questions.size());
    
    for (size_t i = 0; i < count; ++i) {
        results.push_back(executeVQATest(imagePaths[i], questions[i]));
    }
    
    return results;
}

TestInterface::GroundingResult TestInterface::parseGroundingResult(const std::string& resultText) {
    GroundingResult result = {0, 0, 0, 0, ""};
    
    // Use regular expressions to extract coordinates
    std::regex coordRegex(R"(Coordinates.*?[：:]\s*$$\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*$$)");
    std::smatch matches;
    
    if (std::regex_search(resultText, matches, coordRegex) && matches.size() == 5) {
        result.x1 = std::stoi(matches[1].str());
        result.y1 = std::stoi(matches[2].str());
        result.x2 = std::stoi(matches[3].str());
        result.y2 = std::stoi(matches[4].str());
    }
    
    // Extract description information
    size_t descPos = resultText.find("Description:");
    if (descPos != std::string::npos) {
        result.description = resultText.substr(descPos + 12);
        // Remove leading spaces
        result.description.erase(0, result.description.find_first_not_of(" \t\n\r"));
    }
    
    return result;
}

TestInterface::VQAResult TestInterface::parseVQAResult(const std::string& resultText) {
    VQAResult result = {"", 0, 0, 0, 0, false};
    result.answer = resultText;
    
    // Try to extract coordinate information
    std::regex coordRegex(R"(Coordinates.*?[：:]\s*$$\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*$$)");
    std::smatch matches;
    
    if (std::regex_search(resultText, matches, coordRegex) && matches.size() == 5) {
        result.x1 = std::stoi(matches[1].str());
        result.y1 = std::stoi(matches[2].str());
        result.x2 = std::stoi(matches[3].str());
        result.y2 = std::stoi(matches[4].str());
        result.hasCoordinates = true;
    }
    
    return result;
}

TestInterface::Statistics TestInterface::getStatistics() const {
    return statistics_;
}