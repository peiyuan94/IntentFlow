// GUITaskProcessor.cpp: Implementation file
//

#include "pch.h"
#include "GUITaskProcessor.h"
#include "framework.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <json/json.h>
#include <regex>
#include <memory>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <locale>
#include <codecvt>

// Add log file stream
static std::ofstream logFile;
static bool logInitialized = false;

// Log function
void WriteLog(const std::wstring& message) {
    if (!logInitialized) {
        logFile.open("D:\\Git_ZPY\\IntentFlow\\gui_task_processor.log", std::ios::out | std::ios::app);
        logInitialized = true;
    }
    
    if (logFile.is_open()) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        // Format time as string using safe localtime_s
        std::ostringstream timeStream;
        std::tm localTime;
        localtime_s(&localTime, &time_t);  // Use safe localtime_s instead of localtime
        timeStream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        std::string timeStr = timeStream.str();
        
        // Convert wide string message to narrow string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string narrowMessage = converter.to_bytes(message);
        
        logFile << "[" << timeStr << "] " << narrowMessage << std::endl;
        logFile.flush();
    }
}

GUITaskProcessor::GUITaskProcessor() {
    WriteLog(L"GUITaskProcessor constructor called");
    // Initialize QwenAPI
    // API key needs to be set via setApiKey function
}

GUITaskProcessor::~GUITaskProcessor() {
    WriteLog(L"GUITaskProcessor destructor called");
    // Destructor
    if (logFile.is_open()) {
        logFile.close();
    }
}

void GUITaskProcessor::setApiKey(const std::string& apiKey) {
    WriteLog(L"setApiKey called");
    qwenAPI_.setApiKey(apiKey);
}

bool GUITaskProcessor::processAllTasks() {
    WriteLog(L"processAllTasks called");
    std::wcout << L"[GUITaskProcessor] Starting to process all GUI tasks..." << std::endl;
    WriteLog(L"[GUITaskProcessor] Starting to process all GUI tasks...");
    
    // Define task types and paths
    std::vector<std::pair<std::string, std::string>> taskConfigs = {
        {"gui_grounding", "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Grounding"},
        {"gui_referring", "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Referring"},
        {"advanced_vqa", "D:\\Git_ZPY\\IntentFlow\\test\\GUI_VQA"}
    };
    
    // Process each task type
    for (const auto& taskConfig : taskConfigs) {
        std::string taskType = taskConfig.first;
        std::string basePath = taskConfig.second;
        
        std::wcout << L"[GUITaskProcessor] Processing task type: " << 
            std::wstring(taskType.begin(), taskType.end()) << std::endl;
        WriteLog(L"[GUITaskProcessor] Processing task type: " + std::wstring(taskType.begin(), taskType.end()));
        
        std::string jsonFilePath;
        if (taskType == "gui_grounding") {
            jsonFilePath = basePath + "\\GUI_Grounding.json";
        } else if (taskType == "gui_referring") {
            jsonFilePath = basePath + "\\GUI_Referring.json";
        } else if (taskType == "advanced_vqa") {
            jsonFilePath = basePath + "\\GUI_VQA.json";
        }
        
        std::string imageBasePath = basePath + "\\image";
        
        // Load task data
        Json::Value tasks;
        if (!loadTaskData(jsonFilePath, tasks)) {
            std::wcout << L"[GUITaskProcessor] Failed to load task data from: " << 
                std::wstring(jsonFilePath.begin(), jsonFilePath.end()) << std::endl;
            WriteLog(L"[GUITaskProcessor] Failed to load task data from: " + std::wstring(jsonFilePath.begin(), jsonFilePath.end()));
            continue;
        }
        
        // Process tasks
        if (!processGUITasks(taskType, imageBasePath, jsonFilePath, tasks)) {
            std::wcout << L"[GUITaskProcessor] Failed to process tasks for type: " << 
                std::wstring(taskType.begin(), taskType.end()) << std::endl;
            WriteLog(L"[GUITaskProcessor] Failed to process tasks for type: " + std::wstring(taskType.begin(), taskType.end()));
            continue;
        }
    }
    
    std::wcout << L"[GUITaskProcessor] Finished processing all GUI tasks." << std::endl;
    WriteLog(L"[GUITaskProcessor] Finished processing all GUI tasks.");
    return true;
}

// Single task type processing function
bool GUITaskProcessor::processGUITasks(const std::string& taskType) {
    std::wcout << L"[GUITaskProcessor] Starting to process GUI tasks of type: " << 
        std::wstring(taskType.begin(), taskType.end()) << std::endl;
    
    // Define task type and path
    std::string basePath;
    if (taskType == "gui_grounding") {
        basePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Grounding";
    } else if (taskType == "gui_referring") {
        basePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Referring";
    } else if (taskType == "advanced_vqa") {
        basePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_VQA";
    } else {
        std::wcout << L"[GUITaskProcessor] Unknown task type: " << 
            std::wstring(taskType.begin(), taskType.end()) << std::endl;
        return false;
    }
    
    std::string jsonFilePath;
    if (taskType == "gui_grounding") {
        jsonFilePath = basePath + "\\GUI_Grounding.json";
    } else if (taskType == "gui_referring") {
        jsonFilePath = basePath + "\\GUI_Referring.json";
    } else if (taskType == "advanced_vqa") {
        jsonFilePath = basePath + "\\GUI_VQA.json";
    }
    
    std::string imageBasePath = basePath + "\\image";
    
    // Load task data
    Json::Value tasks;
    if (!loadTaskData(jsonFilePath, tasks)) {
        std::wcout << L"[GUITaskProcessor] Failed to load task data from: " << 
            std::wstring(jsonFilePath.begin(), jsonFilePath.end()) << std::endl;
        return false;
    }
    
    // Process tasks
    bool result = processGUITasks(taskType, imageBasePath, jsonFilePath, tasks);
    
    if (result) {
        std::wcout << L"[GUITaskProcessor] Finished processing GUI tasks of type: " << 
            std::wstring(taskType.begin(), taskType.end()) << std::endl;
    } else {
        std::wcout << L"[GUITaskProcessor] Failed to process GUI tasks of type: " << 
            std::wstring(taskType.begin(), taskType.end()) << std::endl;
    }
    
    return result;
}

bool GUITaskProcessor::loadTaskData(const std::string& filePath, Json::Value& root) {
    std::wcout << L"[GUITaskProcessor] Loading task data from: " << 
        std::wstring(filePath.begin(), filePath.end()) << std::endl;
    
    // Open file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::wcout << L"[GUITaskProcessor] Failed to open file: " << 
            std::wstring(filePath.begin(), filePath.end()) << std::endl;
        return false;
    }
    
    // Read file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // Parse JSON Lines format
    return parseJsonLines(content, root);
}

bool GUITaskProcessor::parseJsonLines(const std::string& content, Json::Value& root) {
    Json::Value tasks(Json::arrayValue);
    std::istringstream iss(content);
    std::string line;
    
    // Parse JSON line by line
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        
        Json::Value task;
        // Use CharReader and CharReaderBuilder instead of deprecated Reader
        Json::CharReaderBuilder builder;
        Json::CharReader* reader = builder.newCharReader();
        Json::Value taskValue;
        std::string errors;
        
        // Parse the JSON line
        bool parsingSuccessful = reader->parse(line.c_str(), line.c_str() + line.size(), &taskValue, &errors);
        delete reader;
        
        if (parsingSuccessful) {
            tasks.append(taskValue);
        } else {
            std::wcout << L"[GUITaskProcessor] Failed to parse JSON line: " << 
                std::wstring(line.begin(), line.end()) << std::endl;
            std::wcout << L"[GUITaskProcessor] Error: " << std::wstring(errors.begin(), errors.end()) << std::endl;
        }
    }
    
    root = tasks;
    std::wcout << L"[GUITaskProcessor] Loaded " << tasks.size() << L" tasks" << std::endl;
    return true;
}

bool GUITaskProcessor::processGUITasks(const std::string& taskType, 
                                      const std::string& imagePath,
                                      const std::string& jsonDataPath,
                                      Json::Value& tasks) {
    WriteLog(L"processGUITasks called with taskType: " + std::wstring(taskType.begin(), taskType.end()));
    std::wcout << L"[GUITaskProcessor] Processing " << tasks.size() << L" " << 
        std::wstring(taskType.begin(), taskType.end()) << L" tasks" << std::endl;
    WriteLog(L"[GUITaskProcessor] Processing " + std::to_wstring(tasks.size()) + L" " + 
        std::wstring(taskType.begin(), taskType.end()) + L" tasks");
    
    // Process each task
    for (Json::Value& task : tasks) {
        // Get task information
        std::string imageFileName = task["image"].asString();
        std::string question = task["question"].asString();
        std::string questionId = task["question_id"].asString();
        
        // Build full image path
        std::string fullImagePath = imagePath + "\\" + imageFileName;
        
        // Process single task
        std::string answer = processGUITask(taskType, fullImagePath, question, questionId);
        
        // Update task result
        task["answer"] = answer;
        
        std::wcout << L"[GUITaskProcessor] Processed task " << 
            std::wstring(questionId.begin(), questionId.end()) << 
            L", answer: " << std::wstring(answer.begin(), answer.end()) << std::endl;
        WriteLog(L"[GUITaskProcessor] Processed task " + std::wstring(questionId.begin(), questionId.end()) + 
            L", answer: " + std::wstring(answer.begin(), answer.end()));
    }
    
    // Save results
    std::string outputFileName;
    if (taskType == "gui_grounding") {
        outputFileName = "gui_grounding_result.json";
    } else if (taskType == "gui_referring") {
        outputFileName = "gui_referring_result.json";
    } else if (taskType == "advanced_vqa") {
        outputFileName = "gui_vqa_result.json";
    }
    
    std::string outputFilePath = "D:\\Git_ZPY\\IntentFlow\\" + outputFileName;
    return saveResults(outputFilePath, tasks);
}

std::string GUITaskProcessor::processGUITask(const std::string& taskType,
                                            const std::string& imagePath,
                                            const std::string& question,
                                            const std::string& questionId) {
    WriteLog(L"processGUITask called for questionId: " + std::wstring(questionId.begin(), questionId.end()));
    std::wcout << L"[GUITaskProcessor] Processing task: " << 
        std::wstring(questionId.begin(), questionId.end()) << std::endl;
    WriteLog(L"[GUITaskProcessor] Processing task: " + std::wstring(questionId.begin(), questionId.end()));
    
    // Build prompt
    std::string prompt;
    if (taskType == "gui_grounding") {
        prompt = buildPromptForGrounding(question);
    } else if (taskType == "gui_referring") {
        prompt = buildPromptForReferring(question);
    } else if (taskType == "advanced_vqa") {
        prompt = buildPromptForVQA(question);
    }
    
    WriteLog(L"[GUITaskProcessor] Prompt: " + std::wstring(prompt.begin(), prompt.end()));
    
    // Call Qwen API
    std::vector<std::string> imagePaths = {imagePath};
    QwenAPI::APIResponse response = qwenAPI_.sendImageQuery(imagePaths, prompt);
    
    WriteLog(L"[GUITaskProcessor] API Response success: " + std::wstring(response.success ? L"true" : L"false"));
    WriteLog(L"[GUITaskProcessor] API Response content: " + std::wstring(response.content.begin(), response.content.end()));
    
    if (!response.success) {
        std::wcout << L"[GUITaskProcessor] Failed to get response for task: " << 
            std::wstring(questionId.begin(), questionId.end()) << std::endl;
        WriteLog(L"[GUITaskProcessor] Failed to get response for task: " + std::wstring(questionId.begin(), questionId.end()));
        return "";
    }
    
    // Parse result
    std::string answer;
    if (taskType == "gui_grounding") {
        answer = parseResultForGrounding(response.content);
    } else if (taskType == "gui_referring") {
        answer = parseResultForReferring(response.content);
    } else if (taskType == "advanced_vqa") {
        answer = parseResultForVQA(response.content);
    }
    
    WriteLog(L"[GUITaskProcessor] Parsed answer: " + std::wstring(answer.begin(), answer.end()));
    
    return answer;
}

std::string GUITaskProcessor::buildPromptForGrounding(const std::string& question) {
    std::string prompt = "You are an expert in GUI understanding. ";
    prompt += "Please identify the coordinates of the UI component mentioned in the question. ";
    prompt += "The question is: \"" + question + "\". ";
    prompt += "Return only the coordinates in the format [x1,y1,x2,y2] or [x,y], nothing else.";
    return prompt;
}

std::string GUITaskProcessor::buildPromptForReferring(const std::string& question) {
    std::string prompt = "You are an expert in GUI understanding. ";
    prompt += "Please describe the function or content of the UI component within the specified coordinates. ";
    prompt += "The question is: \"" + question + "\". ";
    prompt += "Return only the description in text, nothing else.";
    return prompt;
}

std::string GUITaskProcessor::buildPromptForVQA(const std::string& question) {
    std::string prompt = "You are an expert in mobile app GUI understanding. ";
    prompt += "Please answer the question according to the screen information. ";
    prompt += "The question is: \"" + question + "\". ";
    prompt += "Return the answer in the format \"text [x1, y1, x2, y2]\" where the coordinates indicate relevant UI components.";
    return prompt;
}

std::string GUITaskProcessor::parseResultForGrounding(const std::string& response) {
    WriteLog(L"[parseResultForGrounding] Processing response");
    WriteLog(L"[parseResultForGrounding] Response content: " + std::wstring(response.begin(), response.end()));
    
    // First try to extract coordinates from the complete API response
    // API response format example: {"output":{"choices":[{"message":{"content":[{"text":"[247,350,478,386]"}],"role":"assistant"},"finish_reason":"stop"}]}}
    
    // Find the content field
    size_t contentPos = response.find("\"content\"");
    if (contentPos != std::string::npos) {
        WriteLog(L"[parseResultForGrounding] Found content field at position: " + std::to_wstring(contentPos));
        
        // Find the text field within content
        size_t textPos = response.find("\"text\"", contentPos);
        if (textPos != std::string::npos) {
            WriteLog(L"[parseResultForGrounding] Found text field at position: " + std::to_wstring(textPos));
            
            // Find the start of the value (skip "text":)
            size_t valueStart = response.find("\"", textPos + 7);
            if (valueStart != std::string::npos) {
                valueStart++; // Skip the first quote
                WriteLog(L"[parseResultForGrounding] Value start position: " + std::to_wstring(valueStart));
                
                // Find the end of the value
                size_t valueEnd = response.find("\"", valueStart);
                if (valueEnd != std::string::npos) {
                    WriteLog(L"[parseResultForGrounding] Value end position: " + std::to_wstring(valueEnd));
                    
                    // Extract the content text
                    std::string contentText = response.substr(valueStart, valueEnd - valueStart);
                    WriteLog(L"[parseResultForGrounding] Extracted content text: " + std::wstring(contentText.begin(), contentText.end()));
                    
                    // Check if contentText contains coordinates format
                    if (contentText.front() == '[' && contentText.back() == ']') {
                        // Validate that it contains only digits and commas
                        bool valid = true;
                        for (size_t i = 1; i < contentText.length() - 1; i++) {
                            if (!isdigit(contentText[i]) && contentText[i] != ',') {
                                valid = false;
                                break;
                            }
                        }
                        
                        if (valid) {
                            WriteLog(L"[parseResultForGrounding] Found coordinates using string parsing: " + std::wstring(contentText.begin(), contentText.end()));
                            return contentText;
                        } else {
                            WriteLog(L"[parseResultForGrounding] Content text has brackets but contains invalid characters");
                        }
                    } else {
                        WriteLog(L"[parseResultForGrounding] Content text doesn't have bracket format");
                    }
                } else {
                    WriteLog(L"[parseResultForGrounding] Could not find end quote for value");
                }
            } else {
                WriteLog(L"[parseResultForGrounding] Could not find start quote for value");
            }
        } else {
            WriteLog(L"[parseResultForGrounding] Could not find text field");
        }
    } else {
        WriteLog(L"[parseResultForGrounding] Could not find content field");
    }
    
    // If the above method fails, try to find coordinates directly in the response
    WriteLog(L"[parseResultForGrounding] Trying direct coordinate search");
    
    // Search for the last occurrence of a bracketed coordinate pattern
    size_t lastOpenBracket = response.rfind('[');
    if (lastOpenBracket != std::string::npos) {
        size_t lastCloseBracket = response.find(']', lastOpenBracket);
        if (lastCloseBracket != std::string::npos) {
            std::string potentialCoords = response.substr(lastOpenBracket, lastCloseBracket - lastOpenBracket + 1);
            WriteLog(L"[parseResultForGrounding] Potential coordinates found: " + std::wstring(potentialCoords.begin(), potentialCoords.end()));
            
            // Validate that it contains only digits and commas (and brackets)
            bool valid = true;
            for (size_t i = 1; i < potentialCoords.length() - 1; i++) {
                if (!isdigit(potentialCoords[i]) && potentialCoords[i] != ',') {
                    valid = false;
                    break;
                }
            }
            
            if (valid && potentialCoords.front() == '[' && potentialCoords.back() == ']') {
                WriteLog(L"[parseResultForGrounding] Found coordinates directly using string parsing: " + std::wstring(potentialCoords.begin(), potentialCoords.end()));
                return potentialCoords;
            } else {
                WriteLog(L"[parseResultForGrounding] Direct search found text in brackets but contains invalid characters");
            }
        } else {
            WriteLog(L"[parseResultForGrounding] Found open bracket but no close bracket");
        }
    } else {
        WriteLog(L"[parseResultForGrounding] No open bracket found in response");
    }
    
    // If still not found, return empty string
    WriteLog(L"[parseResultForGrounding] No coordinates found, returning empty string");
    return "";
}

std::string GUITaskProcessor::parseResultForReferring(const std::string& response) {
    // For Referring tasks, return the response text directly
    return response;
}

std::string GUITaskProcessor::parseResultForVQA(const std::string& response) {
    // For VQA tasks, return the response text directly
    return response;
}

bool GUITaskProcessor::saveResults(const std::string& outputPath, const Json::Value& results) {
    std::wcout << L"[GUITaskProcessor] Saving results to: " << 
        std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
    
    // Open output file
    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
        std::wcout << L"[GUITaskProcessor] Failed to open output file: " << 
            std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
        return false;
    }
    
    // Write in JSON Lines format
    for (const auto& result : results) {
        // Create a new JSON object with only the required fields
        Json::Value outputResult;
        outputResult["image"] = result["image"];
        outputResult["question_id"] = result["question_id"];
        outputResult["type"] = result["type"];
        outputResult["question"] = result["question"];
        outputResult["answer"] = result["answer"];
        
        // Write each result on a separate line
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::ostringstream oss;
        writer->write(outputResult, &oss);
        outputFile << oss.str() << std::endl;
    }
    
    outputFile.close();
    std::wcout << L"[GUITaskProcessor] Saved " << results.size() << L" results" << std::endl;
    return true;
}