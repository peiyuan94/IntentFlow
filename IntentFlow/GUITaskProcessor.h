#pragma once
#include "framework.h"
#include "QwenAPI.h"
#include <string>
#include <vector>
#include <json/json.h>
#include <memory>

class GUITaskProcessor {
public:
    // 构造函数
    GUITaskProcessor();
    
    // 析构函数
    ~GUITaskProcessor();

    // 主处理函数
    bool processAllTasks();
    
    // 设置API密钥
    void setApiKey(const std::string& apiKey);
    
private:
    // 数据读取相关函数
    bool loadTaskData(const std::string& filePath, Json::Value& root);
    bool parseJsonLines(const std::string& content, Json::Value& root);
    
    // 任务处理相关函数
    bool processGUITasks(const std::string& taskType, 
                        const std::string& imagePath, 
                        const std::string& jsonDataPath,
                        Json::Value& tasks);
    
    std::string processGUITask(const std::string& taskType,
                              const std::string& imagePath,
                              const std::string& question,
                              const std::string& questionId);
    
    // 结果保存相关函数
    bool saveResults(const std::string& outputPath, const Json::Value& results);
    
    // 提示词构建函数
    std::string buildPromptForGrounding(const std::string& question);
    std::string buildPromptForReferring(const std::string& question);
    std::string buildPromptForVQA(const std::string& question);
    
    // 结果解析函数
    std::string parseResultForGrounding(const std::string& response);
    std::string parseResultForReferring(const std::string& response);
    std::string parseResultForVQA(const std::string& response);
    
    // Qwen API实例
    QwenAPI qwenAPI_;
};