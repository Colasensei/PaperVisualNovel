// fileutils.cpp
#include "fileutils.h"
#include "ui.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <set>
// ==================== 存档管理 ====================

bool saveGame(const std::string& scriptPath, size_t currentLine,
              const GameState& gameState, const std::string& saveName) {
    // 构建存档路径
    fs::path scriptFilePath(scriptPath);
    fs::path saveDir = scriptFilePath.parent_path() / "saves";

    // 创建存档目录（如果不存在）
    if (!fs::exists(saveDir)) {
        fs::create_directory(saveDir);
    }

    // 存档文件路径
    fs::path savePath = saveDir / (saveName + ".sav");

    // 构建存档数据
    SaveData saveData;
    saveData.currentLine = currentLine;
    saveData.gameState = gameState;
    saveData.scriptPath = scriptPath;

    // 获取当前时间
    time_t now = time(nullptr);
    tm timeInfo;
    localtime_s(&timeInfo, &now);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeInfo);
    saveData.saveTime = timeStr;

    // 序列化存档
    std::ofstream fout(savePath);
    if (!fout.is_open()) {
        return false;
    }

    fout << "[SAVE_INFO]" << std::endl;
    fout << "script_path=" << scriptPath << std::endl;
    fout << "current_line=" << currentLine << std::endl;
    fout << "save_time=" << saveData.saveTime << std::endl;
    fout << std::endl;

    // 写入游戏状态
    fout << gameState.serialize();

    fout.close();
    return true;
}

bool loadGame(const std::string& savePath, SaveData& saveData) {
    std::ifstream fin(savePath);
    if (!fin.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string data = buffer.str();
    fin.close();

    // 解析存档数据
    std::stringstream ss(data);
    std::string line;
    std::string currentSection;

    while (getline(ss, line)) {
        if (line.empty()) continue;

        if (line[0] == '[' && line.back() == ']') {
            currentSection = line;
            continue;
        }

        if (currentSection == "[SAVE_INFO]") {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);

                if (key == "script_path") {
                    saveData.scriptPath = value;
                }
                else if (key == "current_line") {
                    try {
                        saveData.currentLine = std::stoi(value);
                    }
                    catch (...) {
                        saveData.currentLine = 0;
                    }
                }
                else if (key == "save_time") {
                    saveData.saveTime = value;
                }
            }
        }
        else if (currentSection == "[VARIABLES]" ||
                 currentSection == "[CHOICE_HISTORY]" ||
                 currentSection == "[COLLECTED_ENDINGS]") {
            // 这些部分由 GameState::deserialize 处理
            continue;
        }
    }

    // 反序列化游戏状态
    saveData.gameState.deserialize(data);

    return true;
}

bool hasSaveFile(const std::string& scriptPath) {
    fs::path scriptFilePath(scriptPath);
    fs::path saveDir = scriptFilePath.parent_path() / "saves";
    fs::path savePath = saveDir / "autosave.sav";

    return fs::exists(savePath);
}

std::string getSaveInfo(const std::string& scriptPath) {
    fs::path scriptFilePath(scriptPath);
    fs::path saveDir = scriptFilePath.parent_path() / "saves";
    fs::path savePath = saveDir / "autosave.sav";

    if (!fs::exists(savePath)) {
        return "无存档";
    }

    // 读取存档时间
    std::ifstream fin(savePath);
    if (!fin.is_open()) {
        return "存档损坏";
    }

    std::string line;
    while (getline(fin, line)) {
        if (line.find("save_time=") != std::string::npos) {
            std::string timeStr = line.substr(10); // 移除 "save_time="
            return "存档时间: " + timeStr;
        }
    }

    return "有存档";
}

// ==================== 文件安全操作 ====================

bool safeViewFile(const std::string& filepath) {
    Log(LogGrade::INFO, "Try to open file: " + filepath);
    if (!fs::exists(filepath)) {
        Log(LogGrade::ERR, "File not found: " + filepath);
        MessageBoxA(NULL, "错误：文件不存在", "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    std::string extension = fs::path(filepath).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    const std::vector<std::string> allowedExtensions = {
        ".txt", ".md", ".log", ".ini", ".inf", ".cfg", ".json", ".xml",
        ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".ico",
        ".mp3", ".wav", ".ogg", ".flac",
        ".mp4", ".avi", ".mkv", ".mov",
        ".pdf", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx"
    };

    bool isAllowed = false;
    for (const auto& ext : allowedExtensions) {
        if (extension == ext) {
            isAllowed = true;
            break;
        }
    }

    if (!isAllowed) {
        Log(LogGrade::WARNING, "File type not allowed: " + filepath);
        MessageBoxA(NULL,
                   "安全限制：不允许打开此类型的文件",
                   "安全警告",
                   MB_ICONWARNING | MB_OK);
        return false;
    }

    auto filesize = fs::file_size(filepath);
    const size_t MAX_FILE_SIZE = 2000 * 1024 * 1024;

    if (filesize > MAX_FILE_SIZE) {
        Log(LogGrade::WARNING, "File size too large: " + filepath);
        MessageBoxA(NULL,
                   "文件过大，无法安全打开",
                   "安全警告",
                   MB_ICONWARNING | MB_OK);
        return false;
    }

    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        filepath.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );

    if ((INT_PTR)result <= 32) {
        DWORD error = GetLastError();
        Log(LogGrade::ERR, "Failed to open file: " + filepath + " Error code: " + std::to_string(error));
        std::string errorMsg = "无法打开文件。错误代码: " + std::to_string(error);
        MessageBoxA(NULL, errorMsg.c_str(), "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    return true;
}

void overwriteLine(const std::string& filename, int lineToOverwrite, 
                   const std::string& newContent) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        Log(LogGrade::ERR, "Failed to open file: " + filename);
        MessageBoxA(NULL, "错误：无法读取设置文件", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (getline(inputFile, line)) {
        lines.push_back(line);
    }
    inputFile.close();

    if (lineToOverwrite > 0 && lineToOverwrite <= static_cast<int>(lines.size())) {
        lines[lineToOverwrite - 1] = newContent;
    }
    else {
        Log(LogGrade::ERR, "Invalid line number: " + std::to_string(lineToOverwrite));
        MessageBoxA(NULL, "错误：行号无效", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        Log(LogGrade::ERR, "Failed to open file: " + filename);
        MessageBoxA(NULL, "错误：无法写入设置", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    for (const auto& l : lines) {
        outputFile << l << std::endl;
    }
    outputFile.close();
    Log(LogGrade::INFO, "Line " + std::to_string(lineToOverwrite) + " overwritten in " + filename);
}

// ==================== 结局文件操作 ====================

std::vector<std::string> readCollectedEndings(const std::string& gameFolder) {

    Log(LogGrade::INFO, "Reading collected endings for game: " + gameFolder);
    std::vector<std::string> endings;
    std::string filepath = "Novel\\" + gameFolder + "\\data.inf";

    std::ifstream fin(filepath);
    if (!fin.is_open()) {
        return endings;
    }

    std::string line;
    bool inEndingsSection = false;

    while (getline(fin, line)) {
        // 跳过空行
        if (line.empty()) continue;

        // 检查是否是结局开始标记
        if (line == "[ENDINGS]") {
            inEndingsSection = true;
            continue;
        }

        // 检查是否是其他章节开始
        if (line[0] == '[' && line != "[ENDINGS]") {
            inEndingsSection = false;
            continue;
        }

        // 如果是结局部分，读取结局名
        if (inEndingsSection) {
            endings.push_back(line);
        }
    }

    fin.close();

    Log(LogGrade::INFO, "Collected " + std::to_string(endings.size()) + " endings for game: " + gameFolder);
    return endings;
}

void saveEnding(const std::string& gameFolder, const std::string& endingName, 
                GameState& gameState) {
    std::string filepath = "Novel\\" + gameFolder + "\\data.inf";
    Log(LogGrade::INFO, "Saving ending: " + endingName + " for game: " + gameFolder);

    // 读取现有内容
    std::vector<std::string> lines;
    std::ifstream fin(filepath);
    if (fin.is_open()) {
        std::string line;
        while (getline(fin, line)) {
            lines.push_back(line);
        }
        fin.close();
    }

    // 查找或创建 [ENDINGS] 部分
    int endingsSectionIndex = -1;
    bool hasEnding = false;

    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i] == "[ENDINGS]") {
            endingsSectionIndex = i;
            // 检查是否已经记录了这个结局
            for (size_t j = i + 1; j < lines.size(); j++) {
                if (lines[j][0] == '[') break; // 遇到新的章节标记
                if (lines[j] == endingName) {
                    hasEnding = true;
                    break;
                }
            }
            break;
        }
    }

    // 如果没有 [ENDINGS] 部分，创建它
    if (endingsSectionIndex == -1) {
        lines.push_back("[ENDINGS]");
        endingsSectionIndex = lines.size() - 1;
    }

    // 如果还没有记录这个结局，添加它
    if (!hasEnding) {
        // 找到 [ENDINGS] 部分后第一个非结局行
        size_t insertPos = endingsSectionIndex + 1;
        while (insertPos < lines.size() && lines[insertPos][0] != '[') {
            insertPos++;
        }

        lines.insert(lines.begin() + insertPos, endingName);

        // 保存到文件
        std::ofstream fout(filepath);
        if (fout.is_open()) {
            for (const auto& line : lines) {
                fout << line << std::endl;
            }
            fout.close();
        }

        // 更新游戏状态
        gameState.addEnding(endingName);
        Log(LogGrade::INFO, "Ending saved: " + endingName + " for game: " + gameFolder);
    }
}

void loadAllEndings(const std::vector<std::string>& lines, GameState& gameState) {

    Log(LogGrade::INFO, "Loading all endings from script");
    for (const auto& line : lines) {
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "endname" || cmd == "ENDNAME") {
            std::string endingName;
            getline(ss, endingName);

            // 去除开头空格
            size_t start = endingName.find_first_not_of(" ");
            if (start != std::string::npos) {
                endingName = endingName.substr(start);
            }

            if (!endingName.empty()) {
                gameState.registerEnding(endingName);
            }
        }
    }
}

// ==================== 游戏统计 ====================

int countTotalEndingsInScript(const std::string& scriptPath) {
    Log(LogGrade::INFO, "Counting total endings in script: " + scriptPath);
    std::set<std::string> uniqueEndings;

    std::ifstream in(scriptPath);
    if (!in.is_open()) {
        return 0;
    }

    std::string line;
    int lineNum = 0;
    while (getline(in, line)) {
        lineNum++;

        // 去除可能的回车符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 跳过空行和注释
        if (line.empty() || line[0] == '/' || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "endname" || cmd == "ENDNAME") {
            std::string endingName;
            // 读取剩余部分作为结局名
            getline(ss, endingName);
            Log(LogGrade::DEBUG, "Found ending: " + endingName + " at line " + std::to_string(lineNum));
            // 去除开头空格
            size_t start = endingName.find_first_not_of(" \t");
            if (start != std::string::npos) {
                endingName = endingName.substr(start);
            }

            // 去除可能的结尾空格和回车
            size_t end = endingName.find_last_not_of(" \t\r");
            if (end != std::string::npos) {
                endingName = endingName.substr(0, end + 1);
            }

            if (!endingName.empty()) {
                uniqueEndings.insert(endingName);
            }
        }
    }
    in.close();

    return uniqueEndings.size();
}

std::pair<int, int> getGameEndingStats(const std::string& gameFolderPath) {
    Log(LogGrade::INFO, "Getting ending stats for game: " + gameFolderPath);
    int collected = 0;
    int total = 0;

    // 方法1：首先尝试从 endings.dat 读取（新系统）
    std::string endingsFileNew = gameFolderPath + "endings.dat";
    std::ifstream finNew(endingsFileNew);
    if (finNew.is_open()) {
        std::string line;
        while (getline(finNew, line)) {
            // 去除可能的回车符
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // 只统计非空行，且不是标记行
            if (!line.empty() && line != "[ENDINGS]") {
                collected++;
            }
        }
        finNew.close();
    }

    // 方法2：如果新系统没有数据，尝试从 data.inf 读取（兼容旧系统）
    if (collected == 0) {
        std::string endingsFileOld = gameFolderPath + "data.inf";
        std::ifstream finOld(endingsFileOld);
        if (finOld.is_open()) {
            std::string line;
            bool inEndingsSection = false;

            while (getline(finOld, line)) {
                // 去除可能的回车符
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                // 跳过空行
                if (line.empty()) continue;

                // 检查是否是结局开始标记
                if (line == "[ENDINGS]") {
                    inEndingsSection = true;
                    continue;  // 重要：跳过标记行本身
                }

                // 检查是否是其他章节开始
                if (line[0] == '[' && line != "[ENDINGS]") {
                    inEndingsSection = false;
                    continue;
                }

                // 如果是结局部分，读取结局名
                if (inEndingsSection) {
                    collected++;
                }
            }
            finOld.close();
        }
    }

    // 统计总结局数
    // 首先查找.pgn文件
    for (const auto& entry : fs::directory_iterator(gameFolderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".pgn") {
            std::string pgnFile = entry.path().string();
            total = countTotalEndingsInScript(pgnFile);
            break;
        }
    }

    // 如果从脚本中统计失败，尝试从其他方式获取
    if (total == 0 && collected > 0) {
        total = collected; // 至少已经收集的数量
    }

    // 确保不会出现 collected > total 的情况
    if (collected > total && total > 0) {
        collected = total;
    }
    Log(LogGrade::DEBUG, "Collected: " + std::to_string(collected) + ", Total: " + std::to_string(total));

    return { collected, total };
}

std::string getGameFolderName(const std::string& fullPath) {
    fs::path path(fullPath);
    return path.filename().string();
}

// ==================== 配置文件 ====================

// 辅助函数：去除字符串两端的空白字符
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    Log(LogGrade::DEBUG, "Trimming string: " + str);
    if (std::string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

std::string readCfg(const std::string& key) {
    const std::string filename = "data.cfg";

    Log(LogGrade::INFO, "Reading configuration file.");
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        Log(LogGrade::ERR, "Failed to open configuration file.");
        MessageBoxA(NULL, ("错误：无法打开配置文件\n" + filename).c_str(),
            "文件错误", MB_ICONERROR | MB_OK);
        return "";  // 返回空字符串表示读取失败
    }

    std::string line;
    while (std::getline(inFile, line)) {
        // 跳过空行和注释行（以#或;开头）
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty() ||
            trimmedLine[0] == '#' ||
            trimmedLine[0] == ';') {
            continue;
        }

        // 查找等号位置
        size_t equalsPos = trimmedLine.find('=');
        if (equalsPos == std::string::npos) {
            continue;  // 没有等号，不是有效的键值对
        }

        // 提取键名
        std::string currentKey = trim(trimmedLine.substr(0, equalsPos));

        // 检查是否匹配目标键（大小写敏感）
        if (currentKey == key) {
            // 提取并返回值
            std::string value = trim(trimmedLine.substr(equalsPos + 1));
            // 移除可能的引号
            if (value.length() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                    (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.length() - 2);
            }
            Log(LogGrade::INFO, "Found key: " + key + ", value: " + value);
            inFile.close();
            return value;
        }
        
    }

    inFile.close();
    // 没有找到指定的键
    Log(LogGrade::ERR, "Key not found in configuration file: " + key);
    MessageBoxA(NULL, ("错误：配置项未找到\n键名: " + key).c_str(),
        "配置错误", MB_ICONWARNING | MB_OK);
    return "";  // 返回空字符串表示键不存在
}

void updateFirstRunFlag(bool value) {
	Log(LogGrade::INFO, "Updating FirstRunFlag in configuration file.");
    const std::string filename = "data.cfg";
    const std::string targetKey = "FirstRunFlag";
    // 读取整个文件到内存
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        Log(LogGrade::ERR, "Failed to open configuration file.");
        MessageBoxA(NULL, ("错误：无法打开文件 " + filename).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool keyFound = false;

    while (std::getline(inFile, line)) {
        // 查找包含目标键的行
        if (line.find(targetKey) != std::string::npos) {
            // 分离键和值
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                // 保留等号前的部分，更新等号后的值
                std::string newLine = line.substr(0, equalsPos + 1);
                newLine += " ";
                newLine += (value ? "1" : "0");

                lines.push_back(newLine);
                keyFound = true;
                Log(LogGrade::DEBUG, "Updated FirstRunFlag to " + value);
            }
            else {
                // 如果没有等号，保持原样
                lines.push_back(line);
            }
        }
        else {
            lines.push_back(line);
        }
    }

    inFile.close();

    // 如果文件中没有找到FirstRunFlag，可以选择添加它
    if (!keyFound) {
        Log(LogGrade::INFO, "FirstRunFlag not found, adding it.");
        lines.push_back(targetKey + " = " + (value ? "1" : "0"));
    }

    // 写回文件
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        Log(LogGrade::ERR, "Failed to open configuration file for writing.");
        MessageBoxA(NULL, ("错误：无法写入文件 " + filename).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return;
    }

    for (const auto& l : lines) {
        outFile << l << std::endl;
    }
    Log(LogGrade::INFO, "Configuration file updated successfully.");
    outFile.close();
    return;
   
}