#pragma once
#include <string>
#include <vector>
#include <memory>

// Forward declaration
class QwenAPI;

// Test Interface Module
class TestInterface {
public:
    // Test task types
    enum class TaskType {
        GUI_GROUNDING,    // GUI Grounding: Input question, output control coordinates (x1, y1, x2, y2)
        GUI_REFERRING,    // GUI Referring: Input coordinates, output corresponding control function description
        GUI_VQA           // GUI VQA: Input question, output answer text and related control coordinates
    };

    // Test result structure
    struct TestResult {
        bool success = false;
        std::string taskId;           // Task ID
        TaskType taskType;            // Task type
        std::string input;            // Input content
        std::string output;           // Output content
        std::string errorMessage;     // Error message
        std::string imagePath;        // Image path
        long long timestamp;          // Timestamp
    };

    // GUI Grounding result structure
    struct GroundingResult {
        int x1, y1, x2, y2;  // Control coordinates
        std::string description;  // Control description (if any)
    };

    // GUI VQA result structure
    struct VQAResult {
        std::string answer;  // Answer text
        int x1, y1, x2, y2;  // Related control coordinates (if any)
        bool hasCoordinates; // Whether coordinate information is included
    };

    // Constructor
    explicit TestInterface(std::shared_ptr<QwenAPI> qwenAPI);
    void Initialize(); // 添加初始化方法

    // Main function interfaces
    TestResult executeGroundingTest(const std::string& imagePath, const std::string& question);
    TestResult executeReferringTest(const std::string& imagePath, int x, int y);
    TestResult executeVQATest(const std::string& imagePath, const std::string& question);

    // Batch processing interfaces
    std::vector<TestResult> executeBatchGroundingTest(const std::vector<std::string>& imagePaths, 
                                                      const std::vector<std::string>& questions);
    std::vector<TestResult> executeBatchReferringTest(const std::vector<std::string>& imagePaths,
                                                      const std::vector<std::pair<int, int>>& coordinates);
    std::vector<TestResult> executeBatchVQATest(const std::vector<std::string>& imagePaths,
                                                const std::vector<std::string>& questions);

    // Result parsing interfaces
    GroundingResult parseGroundingResult(const std::string& resultText);
    VQAResult parseVQAResult(const std::string& resultText);

    // Statistics interface
    struct Statistics {
        int totalTests;
        int successfulTests;
        int failedTests;
        double successRate;
        double averageResponseTime;  // milliseconds
    };
    
    Statistics getStatistics() const;

private:
    std::shared_ptr<QwenAPI> qwenAPI_;
    Statistics statistics_;
    
    // Internal helper functions
    std::string buildGroundingPrompt(const std::string& question);
    std::string buildReferringPrompt(int x, int y);
    std::string buildVQAPrompt(const std::string& question);
    
    TestResult executeTest(const std::string& imagePath, const std::string& prompt, TaskType taskType, const std::string& input);
};