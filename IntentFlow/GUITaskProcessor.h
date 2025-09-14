#pragma once
#include "framework.h"
#include "QwenAPI.h"
#include <string>
#include <vector>
#include <json/json.h>
#include <memory>

class GUITaskProcessor {
public:
    // Constructor
    GUITaskProcessor();
    
    // Destructor
    ~GUITaskProcessor();

    // Main processing function
    bool processAllTasks();
    
    // Single task type processing function
    bool processGUITasks(const std::string& taskType);
    
    // Set API key
    void setApiKey(const std::string& apiKey);
    
private:
    // Data loading functions
    bool loadTaskData(const std::string& filePath, Json::Value& root);
    bool parseJsonLines(const std::string& content, Json::Value& root);
    
    // Task processing functions
    bool processGUITasks(const std::string& taskType, 
                        const std::string& imagePath, 
                        const std::string& jsonDataPath,
                        Json::Value& tasks);
    
    std::string processGUITask(const std::string& taskType,
                              const std::string& imagePath,
                              const std::string& question,
                              const std::string& questionId);
    
    // Result saving functions
    bool saveResults(const std::string& outputPath, const Json::Value& results);
    
    // Prompt building functions
    std::string buildPromptForGrounding(const std::string& question);
    std::string buildPromptForReferring(const std::string& question);
    std::string buildPromptForVQA(const std::string& question);
    
    // Result parsing functions
    std::string parseResultForGrounding(const std::string& response);
    std::string parseResultForReferring(const std::string& response);
    std::string parseResultForVQA(const std::string& response);
    
    // Qwen API instance
    QwenAPI qwenAPI_;
};