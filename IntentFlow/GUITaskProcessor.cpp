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

GUITaskProcessor::GUITaskProcessor() {
    // Initialize QwenAPI
    // API key needs to be set via setApiKey function
}

GUITaskProcessor::~GUITaskProcessor() {
    // Destructor
}

void GUITaskProcessor::setApiKey(const std::string& apiKey) {
    qwenAPI_.setApiKey(apiKey);
}

bool GUITaskProcessor::processAllTasks() {
    std::wcout << L"[GUITaskProcessor] Starting to process all GUI tasks..." << std::endl;
    
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
            continue;
        }
        
        // Process tasks
        if (!processGUITasks(taskType, imageBasePath, jsonFilePath, tasks)) {
            std::wcout << L"[GUITaskProcessor] Failed to process tasks for type: " << 
                std::wstring(taskType.begin(), taskType.end()) << std::endl;
            continue;
        }
    }
    
    std::wcout << L"[GUITaskProcessor] Finished processing all GUI tasks." << std::endl;
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
    std::wcout << L"[GUITaskProcessor] Processing " << tasks.size() << L" " << 
        std::wstring(taskType.begin(), taskType.end()) << L" tasks" << std::endl;
    
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
    }
    
    // Save results
    std::string outputFileName;
    if (taskType == "gui_grounding") {
        outputFileName = "gui_grounding.json";
    } else if (taskType == "gui_referring") {
        outputFileName = "gui_referring.json";
    } else if (taskType == "advanced_vqa") {
        outputFileName = "gui_vqa.json";
    }
    
    std::string outputFilePath = "D:\\Git_ZPY\\IntentFlow\\" + outputFileName;
    return saveResults(outputFilePath, tasks);
}

std::string GUITaskProcessor::processGUITask(const std::string& taskType,
                                            const std::string& imagePath,
                                            const std::string& question,
                                            const std::string& questionId) {
    std::wcout << L"[GUITaskProcessor] Processing task: " << 
        std::wstring(questionId.begin(), questionId.end()) << std::endl;
    
    // Build prompt
    std::string prompt;
    if (taskType == "gui_grounding") {
        prompt = buildPromptForGrounding(question);
    } else if (taskType == "gui_referring") {
        prompt = buildPromptForReferring(question);
    } else if (taskType == "advanced_vqa") {
        prompt = buildPromptForVQA(question);
    }
    
    // Call Qwen API
    std::vector<std::string> imagePaths = {imagePath};
    QwenAPI::APIResponse response = qwenAPI_.sendImageQuery(imagePaths, prompt);
    
    if (!response.success) {
        std::wcout << L"[GUITaskProcessor] Failed to get response for task: " << 
            std::wstring(questionId.begin(), questionId.end()) << std::endl;
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
    // Use regular expression to extract coordinates
    std::regex coordRegex(R"($$[^$$]*\d+[^$$]*$$)");
    std::smatch match;
    
    if (std::regex_search(response, match, coordRegex)) {
        return match.str();
    }
    
    // If no standard format is found, return the original response
    return response;
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
        // Write each result on a separate line
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::ostringstream oss;
        writer->write(result, &oss);
        outputFile << oss.str() << std::endl;
    }
    
    outputFile.close();
    std::wcout << L"[GUITaskProcessor] Saved " << results.size() << L" results" << std::endl;
    return true;
}