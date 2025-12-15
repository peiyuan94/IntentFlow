# 语流小助手应用蓝图

## 项目概述
语流小助手是一个基于AI的手机自动操作智能体应用，能够通过理解用户的指令，自动完成一系列复杂的手机操作，实现用户效率和体验的高效提升。

## 核心功能
1. 手机屏幕截图理解
2. 基于理解结果的自动化操作执行
3. 三种测试任务支持（用于验证核心理解能力）：
   - GUI Grounding: 根据用户问题，给出截图上指定控件的位置（绝对坐标）
   - GUI Referring: 根据指定位置信息，回答对应位置的功能或文字描述
   - GUI VQA: 基于屏幕信息的手机意图操作问答（文字+坐标）

## 技术架构
- 图像处理：阿里云通义千问VL-Max API
- 用户界面：MFC框架
- 核心能力：图像理解与自动化操作映射

## 测试数据集
- 共1008张图片，2000个问题
- 三种测试类型用于验证核心功能
- 测试数据用于确认图像理解和格式化输出功能正常工作

## 扩展性设计
- 预留手机自动化操作接口
- 模块化设计便于功能扩展
- 标准化输出格式支持多种应用场景

## 部署说明
- 开发环境：Visual Studio with MFC
- 依赖项：阿里云SDK
- 配置要求：API密钥配置文件


```mermaid
graph TD
    %% Main Flow
    A[Input: 原始图片 + 任务类型] --> B[图像预处理]
    B --> C[构建提示词]
    C --> D[调用通义千问VL-Max API]
    D --> E[解析API响应]
    E --> F[后处理]
    F --> G[返回结果]

    %% Image Preprocessing
    subgraph B1 [图像预处理]
    B1_1[读取图片] --> B1_2[调整尺寸]
    B1_2 --> B1_3[转换为Base64]
    end

    %% Prompt Construction
    subgraph C1 [构建提示词]
    C1_1[添加任务描述] --> C1_2[添加格式要求]
    C1_2 --> C1_3[添加示例]
    C1_3 --> C1_4[添加具体问题/坐标]
    end

    %% API Call
    subgraph D1 [API调用]
    D1_1[构建请求体] --> D1_2[发送HTTP请求]
    D1_2 --> D1_3[接收响应]
    end

    %% Response Processing
    subgraph E1 [响应解析]
    E1_1[解析JSON] --> E1_2[验证格式]
    E1_2 --> E1_3[提取关键信息]
    end

    %% Post-processing
    subgraph F1 [后处理]
    F1_1[坐标反算] --> F1_2[结果格式化]
    end

    %% Image Preprocessing Details
    B1_2 -.->|"尺寸调整算法:\n1. 目标尺寸: 960x960\n2. 使用LANCZOS4插值\n3. 不保持宽高比"| B1_2
    B1_3 -.->|"Base64编码:\n1. 使用JPEG格式\n2. 质量: 90%"| B1_3

    %% Prompt Construction Details
    C1_1 -.->|"任务描述模板:\n- Grounding: 识别UI元素位置\n- Referring: 描述指定位置元素\n- VQA: 回答图片相关问题"| C1_1
    C1_2 -.->|"格式要求:\n- JSON格式输出\n- 包含置信度\n- 坐标格式: [x1,y1,x2,y2]"| C1_2

    %% Coordinate Conversion
    F1_1 -.->|"坐标反算公式:\nscaleX = originalWidth/960\nscaleY = originalHeight/960\nx' = x * scaleX\ny' = y * scaleY"| F1_1
```
