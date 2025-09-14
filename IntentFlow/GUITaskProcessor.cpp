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
#include <opencv2/opencv.hpp>
#include "QwenAPI.h"

static thread_local std::string currentImagePath;

// Function declarations
std::wstring ANSIToUnicode(const std::string& str);
std::string UnicodeToUTF8(const std::wstring& wstr);
std::wstring UTF8ToUnicode(const std::string& str);
std::pair<int, int> getImageDimensions(const std::string& imagePath);
void WriteLog(const std::wstring& message);

// Helper functions for string conversion
std::wstring ANSIToUnicode(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string UnicodeToUTF8(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::wstring UTF8ToUnicode(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

// Function to get image dimensions
std::pair<int, int> getImageDimensions(const std::string& imagePath) {
	// Convert imagePath to wide string for OpenCV
	std::wstring widePath = ANSIToUnicode(imagePath);

	// Load the image
	cv::Mat image = cv::imread(UnicodeToUTF8(widePath), cv::IMREAD_COLOR);
	if (image.empty()) {
		// Cannot use WriteLog here as it's a member function
		return std::make_pair(0, 0);
	}

	// Return width and height
	return std::make_pair(image.cols, image.rows);
}

// Function to scale coordinates based on image resizing
std::string scaleCoordinatesInQuestion(const std::string& question, const std::string& imagePath);

// Function to scale coordinates from 960x960 back to original image size
std::string GUITaskProcessor::scaleCoordinatesInAnswer(const std::string& coordinates) {
	WriteLog(L"[scaleCoordinatesInAnswer] Processing coordinates: " + std::wstring(coordinates.begin(), coordinates.end()));
	WriteLog(L"[scaleCoordinatesInAnswer] Current image path: " + std::wstring(currentImagePath.begin(), currentImagePath.end()));

	// Get image dimensions
	std::pair<int, int> dimensions = getImageDimensions(currentImagePath);
	int originalWidth = dimensions.first;
	int originalHeight = dimensions.second;

	if (originalWidth <= 0 || originalHeight <= 0) {
		WriteLog(L"[scaleCoordinatesInAnswer] Failed to get image dimensions");
		return coordinates;
	}

	WriteLog(L"[scaleCoordinatesInAnswer] Original image size: " +
		std::to_wstring(originalWidth) + L"x" + std::to_wstring(originalHeight));

	// Target dimensions (960x960) - same as in QwenAPI::scaleImage
	const int targetWidth = 960;
	const int targetHeight = 960;

	// Calculate scale factors for each dimension separately (not maintaining aspect ratio)
	// 修复：正确的缩放因子应该是原始尺寸/目标尺寸，而不是目标尺寸/原始尺寸
	float scaleX = (float)originalWidth / targetWidth;
	float scaleY = (float)originalHeight / targetHeight;

	WriteLog(L"[scaleCoordinatesInAnswer] Scale factors - X: " + std::to_wstring(scaleX) +
		L", Y: " + std::to_wstring(scaleY));

	// Parse coordinate values
	std::vector<std::string> coords;
	size_t start = 1; // Skip the opening bracket
	size_t commaPos = 0;

	// Split by comma
	while ((commaPos = coordinates.find(',', start)) != std::string::npos) {
		coords.push_back(coordinates.substr(start, commaPos - start));
		start = commaPos + 1;
	}
	coords.push_back(coordinates.substr(start, coordinates.length() - start - 1)); // Add the last value, skip closing bracket

	// Make sure we have valid coordinates (either 2 or 4 values)
	if (coords.size() == 2 || coords.size() == 4) {
		WriteLog(L"[scaleCoordinatesInAnswer] Found " + std::to_wstring(coords.size()) + L" coordinates");

		// Convert and scale coordinates
		std::vector<int> scaledCoords;
		for (size_t i = 0; i < coords.size(); ++i) {
			// Trim whitespace
			std::string trimmedCoord = coords[i];
			trimmedCoord.erase(0, trimmedCoord.find_first_not_of(" \t"));
			trimmedCoord.erase(trimmedCoord.find_last_not_of(" \t") + 1);

			try {
				int value = std::stoi(trimmedCoord);
				// Scale X coordinates with scaleX and Y coordinates with scaleY
				int scaledValue;
				if (i % 2 == 0) { // X coordinate (0, 2, ...)
					scaledValue = static_cast<int>(value * scaleX);
				}
				else { // Y coordinate (1, 3, ...)
					scaledValue = static_cast<int>(value * scaleY);
				}
				scaledCoords.push_back(scaledValue);
				WriteLog(L"[scaleCoordinatesInAnswer] Original: " + std::wstring(trimmedCoord.begin(), trimmedCoord.end()) +
					L", Scaled: " + std::to_wstring(scaledValue));
			}
			catch (const std::exception&) {
				WriteLog(L"[scaleCoordinatesInAnswer] Failed to parse coordinate: " + std::wstring(coords[i].begin(), coords[i].end()));
				return coordinates; // Return original if parsing fails
			}
		}

		// Build scaled coordinate string
		std::string scaledCoordStr = "[";
		for (size_t i = 0; i < scaledCoords.size(); ++i) {
			if (i > 0) scaledCoordStr += ",";
			scaledCoordStr += std::to_string(scaledCoords[i]);
		}
		scaledCoordStr += "]";

		WriteLog(L"[scaleCoordinatesInAnswer] Scaled coordinates string: " + std::wstring(scaledCoordStr.begin(), scaledCoordStr.end()));
		return scaledCoordStr;
	}
	else {
		WriteLog(L"[scaleCoordinatesInAnswer] Invalid number of coordinates: " + std::to_wstring(coords.size()));
	}

	return coordinates;
}

// Overloaded function that can be called without imagePath (for use in parseResultForGrounding)
std::string scaleCoordinatesInAnswer(const std::string& coordinates) {
	// For now, just return the coordinates as is if we don't have the image path
	// In a future update, we might pass the image path to this function
	WriteLog(L"[scaleCoordinatesInAnswer] No image path provided, returning coordinates as is: " + std::wstring(coordinates.begin(), coordinates.end()));
	return coordinates;
}

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
		}
		else if (taskType == "gui_referring") {
			jsonFilePath = basePath + "\\GUI_Referring.json";
		}
		else if (taskType == "advanced_vqa") {
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
	}
	else if (taskType == "gui_referring") {
		basePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Referring";
	}
	else if (taskType == "advanced_vqa") {
		basePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_VQA";
	}
	else {
		std::wcout << L"[GUITaskProcessor] Unknown task type: " <<
			std::wstring(taskType.begin(), taskType.end()) << std::endl;
		return false;
	}

	std::string jsonFilePath;
	if (taskType == "gui_grounding") {
		jsonFilePath = basePath + "\\GUI_Grounding.json";
	}
	else if (taskType == "gui_referring") {
		jsonFilePath = basePath + "\\GUI_Referring.json";
	}
	else if (taskType == "advanced_vqa") {
		jsonFilePath = basePath + "\\GUI_VQA.json";
	}

	std::string imageBasePath = basePath + "\\image";

	// Load task data
	Json::Value tasks;
	if (!loadTaskData(jsonFilePath, tasks)) {
		std::wcout << L"[GUITaskProcessor] Failed to load task data from: " <<
			std::wstring(imageBasePath.begin(), imageBasePath.end()) << std::endl;
		return false;
	}

	// Process tasks
	bool result = processGUITasks(taskType, imageBasePath, jsonFilePath, tasks);

	if (result) {
		std::wcout << L"[GUITaskProcessor] Finished processing GUI tasks of type: " <<
			std::wstring(taskType.begin(), taskType.end()) << std::endl;
	}
	else {
		std::wcout << L"[GUITaskProcessor] Failed to process GUI tasks of type: " <<
			std::wstring(taskType.begin(), taskType.end()) << std::endl;
	}

	return result;
}

bool GUITaskProcessor::loadTaskData(const std::string& filePath, Json::Value& root) {
	std::wcout << L"[GUITaskProcessor] Loading task data from: " <<
		std::wstring(filePath.begin(), filePath.end()) << std::endl;

	// Open file with UTF-8 support
	std::ifstream file(filePath, std::ios::binary);
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

	// Handle UTF-8 BOM if present
	if (content.size() >= 3 && content[0] == '\xEF' && content[1] == '\xBB' && content[2] == '\xBF') {
		content = content.substr(3);  // Remove BOM
	}

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

		// Convert line from UTF-8 to handle Chinese characters properly
		std::wstring wideLine = UTF8ToUnicode(line);
		std::string utf8Line = UnicodeToUTF8(wideLine);

		Json::Value task;
		// Use CharReader and CharReaderBuilder instead of deprecated Reader
		Json::CharReaderBuilder builder;
		Json::CharReader* reader = builder.newCharReader();
		Json::Value taskValue;
		std::string errors;

		// Parse the JSON line
		bool parsingSuccessful = reader->parse(utf8Line.c_str(), utf8Line.c_str() + utf8Line.size(), &taskValue, &errors);
		delete reader;

		if (parsingSuccessful) {
			tasks.append(taskValue);
		}
		else {
			std::wcout << L"[GUITaskProcessor] Failed to parse JSON line: " <<
				std::wstring(utf8Line.begin(), utf8Line.end()) << std::endl;
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

		// Ensure question is properly UTF-8 encoded
		std::wstring wideQuestion = UTF8ToUnicode(question);
		//std::string utf8Question = UnicodeToUTF8(wideQuestion);
		std::string utf8Question = QwenAPI::UnicodeToANSI(wideQuestion);
		WriteLog(L"Question: " + std::wstring(utf8Question.begin(), utf8Question.end()));

		// Build full image path
		std::string fullImagePath = imagePath + "\\" + imageFileName;

		// Process single task
		std::string answer = processGUITask(taskType, fullImagePath, utf8Question, questionId);

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
	}
	else if (taskType == "gui_referring") {
		outputFileName = "gui_referring_result.json";
	}
	else if (taskType == "advanced_vqa") {
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

	// Store the current image path for use in parseResultForGrounding
	currentImagePath = imagePath;

	// Build prompt
	std::string prompt;
	if (taskType == "gui_grounding") {
		prompt = buildPromptForGrounding(question);
	}
	else if (taskType == "gui_referring") {
		// For GUI Referring, we need to scale the coordinates in the question
		std::string scaledQuestion = scaleCoordinatesInQuestion(question, imagePath);
		prompt = buildPromptForReferring(scaledQuestion);
	}
	else if (taskType == "advanced_vqa") {
		prompt = buildPromptForVQA(question);
	}

	WriteLog(L"[GUITaskProcessor] Prompt: " + std::wstring(prompt.begin(), prompt.end()));

	// Call Qwen API
	std::vector<std::string> imagePaths = { imagePath };
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
	}
	else if (taskType == "gui_referring") {
		answer = parseResultForReferring(response.content);
	}
	else if (taskType == "advanced_vqa") {
		answer = parseResultForVQA(response.content);
	}

	WriteLog(L"[GUITaskProcessor] Parsed answer: " + std::wstring(answer.begin(), answer.end()));

	return answer;
}

std::string GUITaskProcessor::buildPromptForGrounding(const std::string& question) {
	std::string prompt = "You are an expert in GUI understanding. ";
	prompt += "The input image has been resized to 960x960 pixels for processing. ";
	prompt += "Please identify the coordinates of the UI component mentioned in the question. ";
	prompt += "The question is: \"" + question + "\". ";
	prompt += "Return only the coordinates in the format [x1,y1,x2,y2] or [x,y], nothing else. ";
	prompt += "Note that the coordinates should be based on the 960x960 pixel image, not the original image size.";
	return prompt;
}

std::string GUITaskProcessor::buildPromptForReferring(const std::string& question) {
	// For GUI Referring, the question contains a coordinate box like [x1,y1,x2,y2]
	// We need to ask the model to describe what's in that box

	std::string prompt = "You are an expert in mobile app GUI understanding. ";
	prompt += "Please identify and describe the UI component at the specified location. ";
	prompt += "The question is: \"" + question + "\". ";
	prompt += "Return only a brief textual description of the component's function or content, nothing else.";
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

					// Check if the content text contains coordinates
					if (contentText.length() > 2 && contentText.front() == '[' && contentText.back() == ']') {
						WriteLog(L"[parseResultForGrounding] Found coordinates in content text: " + std::wstring(contentText.begin(), contentText.end()));
						// Scale the coordinates back to original image size
						std::string scaledCoords = scaleCoordinatesInAnswer(contentText);
						return scaledCoords;
					}
				}
				else {
					WriteLog(L"[parseResultForGrounding] Could not find end quote for value");
				}
			}
			else {
				WriteLog(L"[parseResultForGrounding] Could not find start quote for value");
			}
		}
		else {
			WriteLog(L"[parseResultForGrounding] Could not find text field");
		}
	}
	else {
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
				if (!isdigit(potentialCoords[i]) && potentialCoords[i] != ',' && potentialCoords[i] != ' ') {
					valid = false;
					break;
				}
			}

			if (valid && potentialCoords.front() == '[' && potentialCoords.back() == ']') {
				WriteLog(L"[parseResultForGrounding] Found coordinates directly using string parsing: " + std::wstring(potentialCoords.begin(), potentialCoords.end()));
				// Scale the coordinates back to original image size
				std::string scaledCoords = scaleCoordinatesInAnswer(potentialCoords);
				return scaledCoords;
			}
			else {
				WriteLog(L"[parseResultForGrounding] Direct search found text in brackets but contains invalid characters");
			}
		}
		else {
			WriteLog(L"[parseResultForGrounding] Found open bracket but no close bracket");
		}
	}
	else {
		WriteLog(L"[parseResultForGrounding] No open bracket found in response");
	}

	// If still not found, return empty string
	WriteLog(L"[parseResultForGrounding] No coordinates found, returning empty string");
	return "";
}

std::string GUITaskProcessor::parseResultForReferring(const std::string& response) {
	WriteLog(L"[parseResultForReferring] Processing response");
	WriteLog(L"[parseResultForReferring] Response content: " + std::wstring(response.begin(), response.end()));

	// For Referring tasks, we need to extract the description from the API response
	// The response format example: {"output":{"choices":[{"message":{"content":[{"text":"点击转发帖子"}],"role":"assistant"},"finish_reason":"stop"}]}}

	// Find the content field
	size_t contentPos = response.find("\"content\"");
	if (contentPos != std::string::npos) {
		WriteLog(L"[parseResultForReferring] Found content field at position: " + std::to_wstring(contentPos));

		// Find the text field within content
		size_t textPos = response.find("\"text\"", contentPos);
		if (textPos != std::string::npos) {
			WriteLog(L"[parseResultForReferring] Found text field at position: " + std::to_wstring(textPos));

			// Find the start of the value (skip "text":)
			size_t valueStart = response.find("\"", textPos + 7);
			if (valueStart != std::string::npos) {
				valueStart++; // Skip the first quote
				WriteLog(L"[parseResultForReferring] Value start position: " + std::to_wstring(valueStart));

				// Find the end of the value
				size_t valueEnd = response.find("\"", valueStart);
				if (valueEnd != std::string::npos) {
					WriteLog(L"[parseResultForReferring] Value end position: " + std::to_wstring(valueEnd));

					// Extract the content text
					std::string contentText = response.substr(valueStart, valueEnd - valueStart);
					WriteLog(L"[parseResultForReferring] Extracted content text: " + std::wstring(contentText.begin(), contentText.end()));
					return contentText;
				}
				else {
					WriteLog(L"[parseResultForReferring] Could not find end quote for value");
				}
			}
			else {
				WriteLog(L"[parseResultForReferring] Could not find start quote for value");
			}
		}
		else {
			WriteLog(L"[parseResultForReferring] Could not find text field");
		}
	}
	else {
		WriteLog(L"[parseResultForReferring] Could not find content field");
	}

	// If the above method fails, return the whole response as is
	WriteLog(L"[parseResultForReferring] Returning response as is");
	return response;
}

std::string GUITaskProcessor::parseResultForVQA(const std::string& response) {
	// For VQA tasks, return the response text directly
	return response;
}

bool GUITaskProcessor::saveResults(const std::string& outputPath, const Json::Value& results) {
	std::wcout << L"[GUITaskProcessor] Saving results to: " <<
		std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
	WriteLog(L"[GUITaskProcessor] Saving results to: " + std::wstring(outputPath.begin(), outputPath.end()));

	// Determine the source file path based on the output path
	std::string sourceFilePath;
	if (outputPath.find("gui_grounding_result.json") != std::string::npos) {
		sourceFilePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Grounding\\GUI_Grounding.json";
	}
	else if (outputPath.find("gui_referring_result.json") != std::string::npos) {
		sourceFilePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_Referring\\GUI_Referring.json";
	}
	else if (outputPath.find("gui_vqa_result.json") != std::string::npos) {
		sourceFilePath = "D:\\Git_ZPY\\IntentFlow\\test\\GUI_VQA\\GUI_VQA.json";
	}
	else {
		std::wcout << L"[GUITaskProcessor] Unknown output file type, using default save method" << std::endl;
		WriteLog(L"[GUITaskProcessor] Unknown output file type, using default save method");
		// Fallback to original method
		std::ofstream outputFile(outputPath);
		if (!outputFile.is_open()) {
			std::wcout << L"[GUITaskProcessor] Failed to open output file: " <<
				std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
			WriteLog(L"[GUITaskProcessor] Failed to open output file: " + std::wstring(outputPath.begin(), outputPath.end()));
			return false;
		}

		for (const auto& result : results) {
			Json::Value outputResult;
			outputResult["image"] = result["image"];
			outputResult["type"] = result["type"];
			outputResult["question_id"] = result["question_id"];
			outputResult["question"] = result["question"];
			outputResult["answer"] = result["answer"];

			Json::StreamWriterBuilder builder;
			builder["indentation"] = "";
			std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
			std::ostringstream oss;
			writer->write(outputResult, &oss);
			outputFile << oss.str() << std::endl;
		}

		outputFile.close();
		std::wcout << L"[GUITaskProcessor] Saved " << results.size() << L" results" << std::endl;
		WriteLog(L"[GUITaskProcessor] Saved " + std::to_wstring(results.size()) + L" results");
		return true;
	}

	WriteLog(L"[GUITaskProcessor] Source file path: " + std::wstring(sourceFilePath.begin(), sourceFilePath.end()));

	// Load the original source file
	std::ifstream sourceFile(sourceFilePath);
	if (!sourceFile.is_open()) {
		std::wcout << L"[GUITaskProcessor] Failed to open source file: " <<
			std::wstring(sourceFilePath.begin(), sourceFilePath.end()) << std::endl;
		WriteLog(L"[GUITaskProcessor] Failed to open source file: " + std::wstring(sourceFilePath.begin(), sourceFilePath.end()));
		return false;
	}

	// Read all lines from the source file
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(sourceFile, line)) {
		if (!line.empty()) {
			lines.push_back(line);
		}
	}
	sourceFile.close();

	WriteLog(L"[GUITaskProcessor] Loaded source file lines, count: " + std::to_wstring(lines.size()));

	// Create a map of question_id to answer
	std::map<std::string, std::string> answerMap;
	for (const auto& result : results) {
		std::string questionId = result["question_id"].asString();
		std::string answer = result["answer"].asString();
		answerMap[questionId] = answer;
	}

	WriteLog(L"[GUITaskProcessor] Created answer map, size: " + std::to_wstring(answerMap.size()));

	// Open output file
	std::ofstream outputFile(outputPath);
	if (!outputFile.is_open()) {
		std::wcout << L"[GUITaskProcessor] Failed to open output file: " <<
			std::wstring(outputPath.begin(), outputPath.end()) << std::endl;
		WriteLog(L"[GUITaskProcessor] Failed to open output file: " + std::wstring(outputPath.begin(), outputPath.end()));
		return false;
	}

	// Process each line
	for (const std::string& originalLine : lines) {
		std::string processedLine = originalLine;

		// Extract question_id from the line
		size_t questionIdPos = processedLine.find("\"question_id\"");
		if (questionIdPos != std::string::npos) {
			size_t colonPos = processedLine.find(":", questionIdPos);
			if (colonPos != std::string::npos) {
				size_t startQuotePos = processedLine.find("\"", colonPos);
				if (startQuotePos != std::string::npos) {
					size_t endQuotePos = processedLine.find("\"", startQuotePos + 1);
					if (endQuotePos != std::string::npos) {
						std::string questionId = processedLine.substr(startQuotePos + 1, endQuotePos - startQuotePos - 1);

						// Look up the new answer
						auto it = answerMap.find(questionId);
						if (it != answerMap.end()) {
							// Find the answer field and replace its value
							size_t answerPos = processedLine.find("\"answer\"");
							if (answerPos != std::string::npos) {
								size_t answerColonPos = processedLine.find(":", answerPos);
								if (answerColonPos != std::string::npos) {
									size_t answerStartQuotePos = processedLine.find("\"", answerColonPos);
									if (answerStartQuotePos != std::string::npos) {
										size_t answerEndQuotePos = processedLine.find("\"", answerStartQuotePos + 1);
										if (answerEndQuotePos != std::string::npos) {
											// Replace the answer value
											std::string newLine = processedLine.substr(0, answerStartQuotePos + 1);
											newLine += it->second;
											newLine += processedLine.substr(answerEndQuotePos);
											processedLine = newLine;
											WriteLog(L"[GUITaskProcessor] Updated answer for question ID: " + std::wstring(questionId.begin(), questionId.end()));
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// Write the processed line to output file
		outputFile << processedLine << std::endl;
	}

	outputFile.close();
	std::wcout << L"[GUITaskProcessor] Saved " << lines.size() << L" results" << std::endl;
	WriteLog(L"[GUITaskProcessor] Saved " + std::to_wstring(lines.size()) + L" results");
	return true;
}

// Function to scale coordinates based on image resizing
std::string scaleCoordinatesInQuestion(const std::string& question, const std::string& imagePath) {
	// Convert question to wide string and back to UTF-8 to ensure proper encoding
	std::wstring wideQuestion = QwenAPI::UTF8ToUnicode(question);
	std::string utf8Question = QwenAPI::UnicodeToUTF8(wideQuestion);

	WriteLog(L"[scaleCoordinatesInQuestion] Processing question: " + std::wstring(utf8Question.begin(), utf8Question.end()));
	WriteLog(L"[scaleCoordinatesInQuestion] Image path: " + std::wstring(imagePath.begin(), imagePath.end()));

	// Get image dimensions
	std::pair<int, int> dimensions = getImageDimensions(imagePath);
	int originalWidth = dimensions.first;
	int originalHeight = dimensions.second;

	if (originalWidth <= 0 || originalHeight <= 0) {
		WriteLog(L"[scaleCoordinatesInQuestion] Failed to get image dimensions");
		return utf8Question;
	}

	WriteLog(L"[scaleCoordinatesInQuestion] Original image size: " +
		std::to_wstring(originalWidth) + L"x" + std::to_wstring(originalHeight));

	// Target dimensions (960x960) - same as in QwenAPI::scaleImage, not maintaining aspect ratio
	const int targetWidth = 960;
	const int targetHeight = 960;

	// Calculate scale factors for each dimension separately (not maintaining aspect ratio)
	float scaleX = (float)targetWidth / originalWidth;
	float scaleY = (float)targetHeight / originalHeight;

	WriteLog(L"[scaleCoordinatesInQuestion] Scale factors - X: " + std::to_wstring(scaleX) +
		L", Y: " + std::to_wstring(scaleY));

	// Find coordinate pattern in question like ([x1,y1,x2,y2]) or ([x,y])
	// Simple string parsing approach instead of regex
	size_t parenOpenPos = utf8Question.find('(');
	size_t parenClosePos = utf8Question.find(')', parenOpenPos);

	if (parenOpenPos != std::string::npos && parenClosePos != std::string::npos && parenClosePos > parenOpenPos) {
		// Extract the content inside parentheses
		std::string coordContent = utf8Question.substr(parenOpenPos + 1, parenClosePos - parenOpenPos - 1);
		WriteLog(L"[scaleCoordinatesInQuestion] Coordinate content: " + std::wstring(coordContent.begin(), coordContent.end()));

		// Find the bracket coordinates inside the parentheses
		size_t bracketOpenPos = coordContent.find('[');
		size_t bracketClosePos = coordContent.find(']', bracketOpenPos);

		if (bracketOpenPos != std::string::npos && bracketClosePos != std::string::npos && bracketClosePos > bracketOpenPos) {
			// Extract the coordinate values
			std::string coordValues = coordContent.substr(bracketOpenPos + 1, bracketClosePos - bracketOpenPos - 1);
			WriteLog(L"[scaleCoordinatesInQuestion] Coordinate values: " + std::wstring(coordValues.begin(), coordValues.end()));

			// Parse coordinate values
			std::vector<std::string> coords;
			size_t start = 0;
			size_t commaPos = 0;

			// Split by comma
			while ((commaPos = coordValues.find(',', start)) != std::string::npos) {
				coords.push_back(coordValues.substr(start, commaPos - start));
				start = commaPos + 1;
			}
			coords.push_back(coordValues.substr(start)); // Add the last value

			// Make sure we have valid coordinates (either 2 or 4 values)
			if (coords.size() == 2 || coords.size() == 4) {
				WriteLog(L"[scaleCoordinatesInQuestion] Found " + std::to_wstring(coords.size()) + L" coordinates");

				// Convert and scale coordinates
				std::vector<int> scaledCoords;
				for (size_t i = 0; i < coords.size(); ++i) {
					// Trim whitespace
					std::string trimmedCoord = coords[i];
					trimmedCoord.erase(0, trimmedCoord.find_first_not_of(" \t"));
					trimmedCoord.erase(trimmedCoord.find_last_not_of(" \t") + 1);

					try {
						int value = std::stoi(trimmedCoord);
						// Scale X coordinates with scaleX and Y coordinates with scaleY
						int scaledValue;
						if (i % 2 == 0) { // X coordinate (0, 2, ...)
							scaledValue = static_cast<int>(value * scaleX);
						}
						else { // Y coordinate (1, 3, ...)
							scaledValue = static_cast<int>(value * scaleY);
						}
						scaledCoords.push_back(scaledValue);
						WriteLog(L"[scaleCoordinatesInQuestion] Original: " + std::wstring(trimmedCoord.begin(), trimmedCoord.end()) +
							L", Scaled: " + std::to_wstring(scaledValue));
					}
					catch (const std::exception&) {
						WriteLog(L"[scaleCoordinatesInQuestion] Failed to parse coordinate: " + std::wstring(coords[i].begin(), coords[i].end()));
						return utf8Question; // Return original if parsing fails
					}
				}

				// Build scaled coordinate string
				std::string scaledCoordStr = "[";
				for (size_t i = 0; i < scaledCoords.size(); ++i) {
					if (i > 0) scaledCoordStr += ",";
					scaledCoordStr += std::to_string(scaledCoords[i]);
				}
				scaledCoordStr += "]";

				WriteLog(L"[scaleCoordinatesInQuestion] Scaled coordinates string: " + std::wstring(scaledCoordStr.begin(), scaledCoordStr.end()));

				// Replace the coordinate part in the question
				std::string scaledQuestion = utf8Question.substr(0, parenOpenPos + 1) + scaledCoordStr +
					utf8Question.substr(parenClosePos);

				WriteLog(L"[scaleCoordinatesInQuestion] Scaled question: " + std::wstring(scaledQuestion.begin(), scaledQuestion.end()));
				return scaledQuestion;
			}
			else {
				WriteLog(L"[scaleCoordinatesInQuestion] Invalid number of coordinates: " + std::to_wstring(coords.size()));
			}
		}
		else {
			WriteLog(L"[scaleCoordinatesInQuestion] No bracket coordinates found in parentheses");
		}
	}
	else {
		WriteLog(L"[scaleCoordinatesInQuestion] No parentheses found in question");
	}

	return utf8Question;
}
