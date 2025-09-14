#include "GUITaskProcessor.h"
#include "framework.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <json/json.h>
#include <regex>
#include <memory>

GUITaskProcessor::GUITaskProcessor() {
    // 初始化QwenAPI
    // API密钥需要通过setApiKey函数设置
}

GUITaskProcessor::~GUITaskProcessor() {
    // 析构函数
}

void GUITaskProcessor::setApiKey(const std::string& apiKey) {
    qwenAPI_.setApiKey(apiKey);
}

bool GUITaskProcessor::processAllTasks() {
    std::wcout << L"[GUITaskProcessor] Starting to process all GUI tasks..." << std::endl;
    
    // 定义任务类型和路径
    std::vector<std::pair<std::string, std::string>> taskConfigs = {
        {"gui_grounding", "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Grounding"},
        {"gui_referring", "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Referring"},
        {"advanced_vqa", "D:\\Git_ZPY\\IntentFlow\\test\\GUI_VQA"}
    };
    
    // 处理每种任务类型
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
        
        // 读取任务数据
        Json::Value tasks;
        if (!loadTaskData(jsonFilePath, tasks)) {
            std::wcout << L"[GUITaskProcessor] Failed to load task data from: " << 
                std::wstring(jsonFilePath.begin(), jsonFilePath.end()) << std::endl;
            continue;
        }
        
        // 处理任务
        if (!processGUITasks(taskType, imageBasePath, jsonFilePath, tasks)) {
            std::wcout << L"[GUITaskProcessor] Failed to process tasks for type: " << 
                std::wstring(taskType.begin(), taskType.end()) << std::endl;
            continue;
        }
    }
    
    std::wcout << L"[GUITaskProcessor] Finished processing all GUI tasks." << std::endl;
    return true;
}

bool GUITaskProcessor::loadTaskData(const std::string& filePath, Json::Value& root) {
    std::wcout << L"[GUITaskProcessor] Loading task data from: " << 
        std::wstring(filePath.begin(), filePath.end()) << std::endl;
    
    // 打开文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::wcout << L"[GUITaskProcessor] Failed to open file: " << 
            std::wstring(filePath.begin(), filePath.end()) << std::endl;
        return false;
    }
    
    // 读取文件内容
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    // 解析JSON Lines格式
    return parseJsonLines(content, root);
}

bool GUITaskProcessor::parseJsonLines(const std::string& content, Json::Value& root) {
    Json::Value tasks(Json::arrayValue);
    std::istringstream iss(content);
    std::string line;
    
    // 逐行解析JSON
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        
        Json::Value task;
        Json::Reader reader;
        if (reader.parse(line, task)) {
            tasks.append(task);
        } else {
            std::wcout << L"[GUITaskProcessor] Failed to parse JSON line: " << 
                std::wstring(line.begin(), line.end()) << std::endl;
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
    
    // 处理每个任务
    for (Json::Value& task : tasks) {
        // 获取任务信息
        std::string imageFileName = task["image"].asString();
        std::string question = task["question"].asString();
        std::string questionId = task["question_id"].asString();
        
        // 构建完整图像路径
        std::string fullImagePath = imagePath + "\\" + imageFileName;
        
        // 处理单个任务
        std::string answer = processGUITask(taskType, fullImagePath, question, questionId);
        
        // 更新任务结果
        task["answer"] = answer;
        
        std::wcout << L"[GUITaskProcessor] Processed task " << 
            std::wstring(questionId.begin(), questionId.end()) << 
            L", answer: " << std::wstring(answer.begin(), answer.end()) << std::endl;
    }
    
    // 保存结果
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
    
    // 构建提示词
    std::string prompt;
    if (taskType == "gui_grounding") {
        prompt = buildPromptForGrounding(question);
    } else if (taskType == "gui_referring") {
        prompt = buildPromptForReferring(question);
    } else if (taskType == "advanced_vqa") {
        prompt = buildPromptForVQA(question);
    }
    
    // 调用Qwen API
    std::vector<std::string> imagePaths = {imagePath};
    APIResponse response = qwenAPI_.sendImageQuery(imagePaths, prompt);
    
    if (!response.success) {
        std::wcout << L"[GUITaskProcessor] Failed to get response for task: " << 
            std::wstring(questionId.begin(), questionId.end()) << std::endl;
        return "";
    }
    
    // 解析结果
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
    // 使用正则表达式提取坐标
    std::regex coordRegex(R"($$[^$$]*\d+[^$$]*$$)");
    std::smatch match;
    
    if (std::regex_search(response, match, coordRegex)) {
        return match.str();
    }
    
    // 如果没有找到标准格式，返回原始响应
    return response;
}

std::string GUITaskProcessor::parseResultForReferring(const std::string& response) {
    // 对于Referring任务，直接返回响应文本
    return response;
}

std::string GUITaskProcessor::parseResultForVQA(const std::string& response) {
    // 对于VQA任务，直接返回响应文本
    return response;
}

bool GUITaskProcessor::saveResults(const std::string& outputPath, const Json::Value& results) {
    std::wcout << L"[GUITaskProcessor] Saving results to: " << 
        std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
    
    // 打开输出文件
    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
        std::wcout << L"[GUITaskProcessor] Failed to open output file: " << 
            std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
        return false;
    }
    
    // 写入JSON Lines格式
    for (const auto& result : results) {
        // 将每个结果写入单独的一行
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