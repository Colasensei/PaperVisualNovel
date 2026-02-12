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
#include <map>

// 辅助函数：去除字符串两端的空白字符
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    Log(LogGrade::DEBUG, LogCode::PERFORMANCE, "Trimming string: " + str);
    if (std::string::npos == first) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

// ==================== 存档管理 ====================

bool saveGame(const std::string& scriptPath, size_t currentLine,
    const GameState& gameState, const std::string& saveName) {
    auto saveStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::GAME_SAVED,
        "Attempting to save game: " + scriptPath + " (save name: " + saveName + ")");

    // 构建存档路径
    fs::path scriptFilePath(scriptPath);
    fs::path saveDir = scriptFilePath.parent_path() / "saves";

    // 创建存档目录（如果不存在）
    if (!fs::exists(saveDir)) {
        Log(LogGrade::DEBUG, LogCode::GAME_SAVED,
            "Save directory does not exist, creating: " + saveDir.string());
        if (!fs::create_directory(saveDir)) {
            Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
                "Failed to create save directory: " + saveDir.string());
            return false;
        }
    }

    // 存档文件路径
    fs::path savePath = saveDir / (saveName + ".sav");
    Log(LogGrade::DEBUG, LogCode::GAME_SAVED, "Save file path: " + savePath.string());

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
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Cannot open save file for writing: " + savePath.string());
        return false;
    }

    fout << "[SAVE_INFO]" << std::endl;
    fout << "script_path=" << scriptPath << std::endl;
    fout << "current_line=" << currentLine << std::endl;
    fout << "save_time=" << saveData.saveTime << std::endl;
    fout << std::endl;

    // 写入游戏状态
    std::string serializedState = gameState.serialize();
    fout << serializedState;

    size_t saveSize = fout.tellp();
    fout.close();

    auto saveEndTime = std::chrono::high_resolution_clock::now();
    auto saveTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(saveEndTime - saveStartTime).count();

    
    Log(LogGrade::INFO, LogCode::GAME_SAVED,
        "Game saved successfully: " + savePath.string() +
        " (" + std::to_string(saveSize) + " bytes, took " + std::to_string(saveTimeMs) + "ms)");

    return true;
}

bool loadGame(const std::string& savePath, SaveData& saveData) {
    auto loadStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::GAME_LOADED,
        "Attempting to load save file: " + savePath);

    std::ifstream fin(savePath);
    if (!fin.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Cannot open save file: " + savePath);
        return false;
    }

    // 获取文件大小
    size_t fileSize = 0;
    try {
        fileSize = fs::file_size(savePath);
    }
    catch (const std::exception& e) {
        Log(LogGrade::WARNING, LogCode::SAVE_CORRUPTED,
            "Cannot get save file size: " + std::string(e.what()));
    }

    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string data = buffer.str();
    fin.close();

    Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
        "Save file read: " + std::to_string(data.length()) + " bytes");

    // 解析存档数据
    std::stringstream ss(data);
    std::string line;
    std::string currentSection;

    std::map<std::string, int> sectionLineCounts;
    int totalLines = 0;

    while (std::getline(ss, line)) {
        totalLines++;
        if (line.empty()) continue;

        if (line[0] == '[' && line.back() == ']') {
            currentSection = line;
            sectionLineCounts[currentSection] = 0;
            continue;
        }

        if (!currentSection.empty()) {
            sectionLineCounts[currentSection]++;
        }

        if (currentSection == "[SAVE_INFO]") {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = line.substr(0, equalsPos);
                std::string value = line.substr(equalsPos + 1);

                if (key == "script_path") {
                    saveData.scriptPath = value;
                    Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
                        "Loaded script_path: " + value);
                }
                else if (key == "current_line") {
                    try {
                        saveData.currentLine = std::stoi(value);
                        Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
                            "Loaded current_line: " + value);
                    }
                    catch (...) {
                        Log(LogGrade::WARNING, LogCode::SAVE_CORRUPTED,
                            "Invalid current_line value: " + value + ", using 0");
                        saveData.currentLine = 0;
                    }
                }
                else if (key == "save_time") {
                    saveData.saveTime = value;
                    Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
                        "Loaded save_time: " + value);
                }
            }
        }
    }

    // 反序列化游戏状态
    try {
        saveData.gameState.deserialize(data);
        Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
            "Game state deserialized successfully");
    }
    catch (const std::exception& e) {
        Log(LogGrade::ERR, LogCode::SAVE_CORRUPTED,
            "Failed to deserialize game state: " + std::string(e.what()));
        return false;
    }

    auto loadEndTime = std::chrono::high_resolution_clock::now();
    auto loadTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEndTime - loadStartTime).count();

    // 记录解析统计
    Log(LogGrade::DEBUG, LogCode::PERFORMANCE,
        "Save file statistics: " + std::to_string(totalLines) + " total lines");
    for (const auto& [section, count] : sectionLineCounts) {
        Log(LogGrade::DEBUG, LogCode::PERFORMANCE,
            "  " + section + ": " + std::to_string(count) + " lines");
    }

    
    Log(LogGrade::INFO, LogCode::GAME_LOADED,
        "Save file loaded successfully: " + savePath +
        " (took " + std::to_string(loadTimeMs) + "ms)");

    return true;
}

bool hasSaveFile(const std::string& scriptPath) {
    fs::path scriptFilePath(scriptPath);
    fs::path saveDir = scriptFilePath.parent_path() / "saves";
    fs::path savePath = saveDir / "autosave.sav";

    bool exists = fs::exists(savePath);
    Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
        "Checking save file: " + savePath.string() + " - " + (exists ? "exists" : "not found"));

    return exists;
}

std::string getSaveInfo(const std::string& scriptPath) {
    fs::path scriptFilePath(scriptPath);
    fs::path saveDir = scriptFilePath.parent_path() / "saves";
    fs::path savePath = saveDir / "autosave.sav";

    if (!fs::exists(savePath)) {
        Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
            "Save info requested for non-existent file: " + savePath.string());
        return "无存档";
    }

    // 读取存档时间
    std::ifstream fin(savePath);
    if (!fin.is_open()) {
        Log(LogGrade::WARNING, LogCode::SAVE_CORRUPTED,
            "Cannot open save file to read info: " + savePath.string());
        return "存档损坏";
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (line.find("save_time=") != std::string::npos) {
            std::string timeStr = line.substr(10);
            Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
                "Save info retrieved: " + timeStr);
            return "存档时间: " + timeStr;
        }
    }

    Log(LogGrade::WARNING, LogCode::SAVE_CORRUPTED,
        "Save file missing save_time field: " + savePath.string());
    return "有存档";
}

// ==================== 文件安全操作 ====================

bool safeViewFile(const std::string& filepath) {
    auto fileOpenStart = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::GAME_START,
        "Attempting to open file securely: " + filepath);

    if (!fs::exists(filepath)) {
        Log(LogGrade::ERR, LogCode::FILE_NOT_FOUND,
            "File not found: " + filepath);

        formatErrorOutput(
            logCodeToString(LogCode::FILE_NOT_FOUND),
            "FileError",
            "File does not exist",
            "",
            0,
            std::string::npos,
            "Check if the file path is correct and the file exists",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/W2001.md"
        );

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
        Log(LogGrade::WARNING, LogCode::FILE_NOT_FOUND,
            "File type not allowed: " + filepath + " (extension: " + extension + ")");

        formatErrorOutput(
            logCodeToString(LogCode::FILE_NOT_FOUND),
            "SecurityError",
            "File type is not allowed for security reasons",
            "",
            0,
            std::string::npos,
            "Only specific file types can be opened. Check documentation for allowed extensions.",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/W2001.md"
        );

        MessageBoxA(NULL,
            "安全限制：不允许打开此类型的文件",
            "安全警告",
            MB_ICONWARNING | MB_OK);
        return false;
    }

    try {
        auto filesize = fs::file_size(filepath);
        const size_t MAX_FILE_SIZE = 2000 * 1024 * 1024; // 2GB

        Log(LogGrade::DEBUG, LogCode::PERFORMANCE,
            "File size: " + std::to_string(filesize) + " bytes");

        if (filesize > MAX_FILE_SIZE) {
            Log(LogGrade::WARNING, LogCode::FILE_NOT_FOUND,
                "File size too large: " + std::to_string(filesize) + " bytes, max: " +
                std::to_string(MAX_FILE_SIZE));

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

        auto fileOpenEnd = std::chrono::high_resolution_clock::now();
        auto fileOpenTime = std::chrono::duration_cast<std::chrono::milliseconds>(fileOpenEnd - fileOpenStart).count();

        if ((INT_PTR)result <= 32) {
            DWORD error = GetLastError();
            Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
                "Failed to open file: " + filepath + " Error code: " + std::to_string(error));

            std::string errorMsg = "无法打开文件。错误代码: " + std::to_string(error);
            MessageBoxA(NULL, errorMsg.c_str(), "错误", MB_ICONERROR | MB_OK);
            return false;
        }

        Log(LogGrade::INFO, LogCode::GAME_START,
            "File opened successfully: " + filepath +
            " (took " + std::to_string(fileOpenTime) + "ms)");

        return true;
    }
    catch (const std::exception& e) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Exception while opening file: " + std::string(e.what()));
        return false;
    }
}

void overwriteLine(const std::string& filename, int lineToOverwrite,
    const std::string& newContent) {
    Log(LogGrade::INFO, LogCode::GAME_SAVED,
        "Attempting to overwrite line " + std::to_string(lineToOverwrite) +
        " in file: " + filename);

    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Failed to open file for reading: " + filename);
        MessageBoxA(NULL, "错误：无法读取设置文件", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inputFile, line)) {
        lines.push_back(line);
    }
    inputFile.close();

    Log(LogGrade::DEBUG, LogCode::GAME_SAVED,
        "File read: " + std::to_string(lines.size()) + " lines");

    if (lineToOverwrite > 0 && lineToOverwrite <= static_cast<int>(lines.size())) {
        std::string oldContent = lines[lineToOverwrite - 1];
        lines[lineToOverwrite - 1] = newContent;
        Log(LogGrade::DEBUG, LogCode::GAME_SAVED,
            "Line " + std::to_string(lineToOverwrite) +
            " changed from: \"" + oldContent + "\" to: \"" + newContent + "\"");
    }
    else {
        Log(LogGrade::ERR, LogCode::MEMORY_ERROR,
            "Invalid line number: " + std::to_string(lineToOverwrite) +
            " (file has " + std::to_string(lines.size()) + " lines)");
        MessageBoxA(NULL, "错误：行号无效", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Failed to open file for writing: " + filename);
        MessageBoxA(NULL, "错误：无法写入设置", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    size_t bytesWritten = 0;
    for (const auto& l : lines) {
        outputFile << l << std::endl;
        bytesWritten += l.length() + 2; // +2 for newline
    }
    outputFile.close();

    Log(LogGrade::INFO, LogCode::GAME_SAVED,
        "Line " + std::to_string(lineToOverwrite) + " overwritten in " + filename +
        " (" + std::to_string(bytesWritten) + " bytes written)");
}

// ==================== 结局文件操作 ====================

std::vector<std::string> readCollectedEndings(const std::string& gameFolder) {
    auto readStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Reading collected endings for game: " + gameFolder);

    std::vector<std::string> endings;
    std::string filepath = "Novel\\" + gameFolder + "\\data.inf";

    Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
        "Endings file path: " + filepath);

    std::ifstream fin(filepath);
    if (!fin.is_open()) {
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "No endings file found, returning empty list");
        return endings;
    }

    std::string line;
    bool inEndingsSection = false;
    int endingsCount = 0;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        if (line == "[ENDINGS]") {
            inEndingsSection = true;
            Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                "Found [ENDINGS] section");
            continue;
        }

        if (line[0] == '[' && line != "[ENDINGS]") {
            if (inEndingsSection) {
                Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                    "Exiting [ENDINGS] section at: " + line);
            }
            inEndingsSection = false;
            continue;
        }

        if (inEndingsSection) {
            endings.push_back(line);
            endingsCount++;
            Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                "Found ending: " + line);
        }
    }

    fin.close();

    auto readEndTime = std::chrono::high_resolution_clock::now();
    auto readTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(readEndTime - readStartTime).count();


    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Collected " + std::to_string(endings.size()) + " endings for game: " + gameFolder +
        " (took " + std::to_string(readTimeMs) + "ms)");

    return endings;
}

void saveEnding(const std::string& gameFolder, const std::string& endingName,
    GameState& gameState) {
    auto saveStartTime = std::chrono::high_resolution_clock::now();

    std::string filepath = "Novel\\" + gameFolder + "\\data.inf";
    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Saving ending: \"" + endingName + "\" for game: " + gameFolder);

    Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
        "Endings file path: " + filepath);

    // 读取现有内容
    std::vector<std::string> lines;
    std::ifstream fin(filepath);
    if (fin.is_open()) {
        std::string line;
        while (std::getline(fin, line)) {
            lines.push_back(line);
        }
        fin.close();
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "Read " + std::to_string(lines.size()) + " lines from existing file");
    }
    else {
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "No existing endings file, will create new");
    }

    // 查找或创建 [ENDINGS] 部分
    int endingsSectionIndex = -1;
    bool hasEnding = false;

    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i] == "[ENDINGS]") {
            endingsSectionIndex = i;
            Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                "Found [ENDINGS] section at line " + std::to_string(i + 1));

            // 检查是否已经记录了这个结局
            for (size_t j = i + 1; j < lines.size(); j++) {
                if (lines[j][0] == '[') break;
                if (lines[j] == endingName) {
                    hasEnding = true;
                    Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                        "Ending already exists at line " + std::to_string(j + 1));
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
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "Created new [ENDINGS] section at line " + std::to_string(endingsSectionIndex + 1));
    }

    // 如果还没有记录这个结局，添加它
    if (!hasEnding) {
        size_t insertPos = endingsSectionIndex + 1;
        while (insertPos < lines.size() && lines[insertPos][0] != '[') {
            insertPos++;
        }

        lines.insert(lines.begin() + insertPos, endingName);
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "Inserted ending at line " + std::to_string(insertPos + 1));

        // 保存到文件
        std::ofstream fout(filepath);
        if (fout.is_open()) {
            for (const auto& line : lines) {
                fout << line << std::endl;
            }
            fout.close();

            auto saveEndTime = std::chrono::high_resolution_clock::now();
            auto saveTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(saveEndTime - saveStartTime).count();

            // 更新游戏状态
            gameState.addEnding(endingName);

           

            Log(LogGrade::INFO, LogCode::ENDING_SAVED,
                "Ending saved: \"" + endingName + "\" for game: " + gameFolder +
                " (took " + std::to_string(saveTimeMs) + "ms)");
        }
        else {
            Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
                "Failed to write endings file: " + filepath);
        }
    }
    else {
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "Ending already exists, skipping save: " + endingName);
    }
}

void loadAllEndings(const std::vector<std::string>& lines, GameState& gameState) {
    auto loadStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Loading all endings from script");

    int endingsCount = 0;
    int lineNumber = 0;

    for (const auto& line : lines) {
        lineNumber++;
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "endname" || cmd == "ENDNAME") {
            std::string endingName;
            std::getline(ss, endingName);

            size_t start = endingName.find_first_not_of(" ");
            if (start != std::string::npos) {
                endingName = endingName.substr(start);
            }

            size_t end = endingName.find_last_not_of(" \t\r");
            if (end != std::string::npos) {
                endingName = endingName.substr(0, end + 1);
            }

            if (!endingName.empty()) {
                gameState.registerEnding(endingName);
                endingsCount++;
                Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                    "Registered ending at line " + std::to_string(lineNumber) +
                    ": \"" + endingName + "\"");
            }
        }
    }

    auto loadEndTime = std::chrono::high_resolution_clock::now();
    auto loadTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(loadEndTime - loadStartTime).count();

    

    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Registered " + std::to_string(endingsCount) + " total endings from script" +
        " (took " + std::to_string(loadTimeMs) + "ms)");
}

// ==================== 游戏统计 ====================

int countTotalEndingsInScript(const std::string& scriptPath) {
    auto countStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Counting total endings in script: " + scriptPath);

    std::set<std::string> uniqueEndings;

    std::ifstream in(scriptPath);
    if (!in.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Cannot open script file: " + scriptPath);
        return 0;
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(in, line)) {
        lineNum++;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty() || line[0] == '/' || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "endname" || cmd == "ENDNAME") {
            std::string endingName;
            std::getline(ss, endingName);

            size_t start = endingName.find_first_not_of(" \t");
            if (start != std::string::npos) {
                endingName = endingName.substr(start);
            }

            size_t end = endingName.find_last_not_of(" \t\r");
            if (end != std::string::npos) {
                endingName = endingName.substr(0, end + 1);
            }

            if (!endingName.empty()) {
                uniqueEndings.insert(endingName);
                Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                    "Found ending at line " + std::to_string(lineNum) +
                    ": \"" + endingName + "\"");
            }
        }
    }
    in.close();

    auto countEndTime = std::chrono::high_resolution_clock::now();
    auto countTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(countEndTime - countStartTime).count();

   

    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Found " + std::to_string(uniqueEndings.size()) + " unique endings in script: " + scriptPath +
        " (took " + std::to_string(countTimeMs) + "ms)");

    return uniqueEndings.size();
}

std::pair<int, int> getGameEndingStats(const std::string& gameFolderPath) {
    auto statsStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::ENDING_SAVED,
        "Getting ending stats for game: " + gameFolderPath);

    int collected = 0;
    int total = 0;

    // 方法1：首先尝试从 endings.dat 读取（新系统）
    std::string endingsFileNew = gameFolderPath + "endings.dat";
    std::ifstream finNew(endingsFileNew);
    if (finNew.is_open()) {
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "Reading from new endings format: " + endingsFileNew);

        std::string line;
        while (std::getline(finNew, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty() && line != "[ENDINGS]") {
                collected++;
            }
        }
        finNew.close();
        Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
            "Found " + std::to_string(collected) + " endings in new format");
    }

    // 方法2：如果新系统没有数据，尝试从 data.inf 读取（兼容旧系统）
    if (collected == 0) {
        std::string endingsFileOld = gameFolderPath + "data.inf";
        std::ifstream finOld(endingsFileOld);
        if (finOld.is_open()) {
            Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                "Reading from legacy endings format: " + endingsFileOld);

            std::string line;
            bool inEndingsSection = false;

            while (std::getline(finOld, line)) {
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                if (line.empty()) continue;

                if (line == "[ENDINGS]") {
                    inEndingsSection = true;
                    continue;
                }

                if (line[0] == '[' && line != "[ENDINGS]") {
                    inEndingsSection = false;
                    continue;
                }

                if (inEndingsSection) {
                    collected++;
                }
            }
            finOld.close();
            Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
                "Found " + std::to_string(collected) + " endings in legacy format");
        }
    }

    // 统计总结局数
    for (const auto& entry : fs::directory_iterator(gameFolderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".pgn") {
            std::string pgnFile = entry.path().string();
            total = countTotalEndingsInScript(pgnFile);
            break;
        }
    }

    // 如果从脚本中统计失败，尝试从其他方式获取
    if (total == 0 && collected > 0) {
        total = collected;
        Log(LogGrade::WARNING, LogCode::ENDING_SAVED,
            "Could not count total endings from script, using collected count as total");
    }

    // 确保不会出现 collected > total 的情况
    if (collected > total && total > 0) {
        Log(LogGrade::WARNING, LogCode::ENDING_SAVED,
            "Collected endings (" + std::to_string(collected) +
            ") exceed total endings (" + std::to_string(total) +
            "), adjusting to total");
        collected = total;
    }

    auto statsEndTime = std::chrono::high_resolution_clock::now();
    auto statsTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(statsEndTime - statsStartTime).count();

    

    Log(LogGrade::DEBUG, LogCode::ENDING_SAVED,
        "Collected: " + std::to_string(collected) +
        ", Total: " + std::to_string(total) +
        " (took " + std::to_string(statsTimeMs) + "ms)");

    return { collected, total };
}

std::string getGameFolderName(const std::string& fullPath) {
    fs::path path(fullPath);
    std::string folderName = path.filename().string();
    Log(LogGrade::DEBUG, LogCode::GAME_LOADED,
        "Extracted folder name: " + folderName + " from path: " + fullPath);
    return folderName;
}

// ==================== 插件管理 ====================

std::vector<PluginInfo> readInstalledPlugins() {
    auto pluginsReadStart = std::chrono::high_resolution_clock::now();

    std::vector<PluginInfo> plugins;
    std::string pluginsDir = "Plugins";

    Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
        "Reading installed plugins from: " + pluginsDir);

    if (!fs::exists(pluginsDir)) {
        Log(LogGrade::WARNING, LogCode::PLUGIN_MISSING,
            "Plugins directory does not exist: " + pluginsDir);
        return plugins;
    }

    int pluginsFound = 0;
    int pluginsLoaded = 0;

    try {
        for (const auto& entry : fs::directory_iterator(pluginsDir)) {
            if (entry.is_directory()) {
                pluginsFound++;
                std::string pluginName = entry.path().filename().string();
                std::string aboutFilePath = entry.path().string() + "\\about.cfg";

                Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
                    "Checking plugin: " + pluginName + " at " + entry.path().string());

                if (fs::exists(aboutFilePath)) {
                    PluginInfo plugin;
                    plugin.name = pluginName;

                    std::ifstream aboutFile(aboutFilePath);
                    if (aboutFile.is_open()) {
                        int configLines = 0;
                        std::string line;

                        while (std::getline(aboutFile, line)) {
                            configLines++;
                            std::string trimmedLine = trim(line);
                            if (trimmedLine.empty() || trimmedLine[0] == '#') {
                                continue;
                            }

                            size_t equalsPos = trimmedLine.find('=');
                            if (equalsPos == std::string::npos) {
                                continue;
                            }

                            std::string key = trim(trimmedLine.substr(0, equalsPos));
                            std::string value = trim(trimmedLine.substr(equalsPos + 1));

                            if (value.length() >= 2 &&
                                ((value.front() == '"' && value.back() == '"') ||
                                    (value.front() == '\'' && value.back() == '\''))) {
                                value = value.substr(1, value.length() - 2);
                            }

                            if (key == "RunCommand") {
                                plugin.runCommand = value;
                            }
                            else if (key == "RunFile") {
                                plugin.runFile = value;
                            }
                            else if (key == "Description") {
                                plugin.description = value;
                            }
                            else if (key == "Version") {
                                plugin.version = value;
                            }
                            else if (key == "Author") {
                                plugin.author = value;
                            }
                        }
                        aboutFile.close();

                        Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
                            "Plugin " + pluginName + " config: " +
                            std::to_string(configLines) + " lines, " +
                            "RunCommand=" + plugin.runCommand + ", " +
                            "RunFile=" + plugin.runFile);

                        if (!plugin.runCommand.empty() && !plugin.runFile.empty()) {
                            plugins.push_back(plugin);
                            pluginsLoaded++;
                            Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
                                "Plugin loaded: " + pluginName +
                                (plugin.version.empty() ? "" : " v" + plugin.version));
                        }
                        else {
                            Log(LogGrade::WARNING, LogCode::PLUGIN_MISSING,
                                "Plugin " + pluginName +
                                " missing required fields (RunCommand or RunFile)");
                        }
                    }
                    else {
                        Log(LogGrade::WARNING, LogCode::FILE_OPEN_FAILED,
                            "Cannot open about.cfg for plugin: " + pluginName);
                    }
                }
                else {
                    Log(LogGrade::WARNING, LogCode::PLUGIN_MISSING,
                        "Plugin " + pluginName + " missing about.cfg file");
                }
            }
        }
    }
    catch (const std::exception& e) {
        Log(LogGrade::ERR, LogCode::MEMORY_ERROR,
            "Error reading plugins directory: " + std::string(e.what()));

        formatErrorOutput(
            logCodeToString(LogCode::MEMORY_ERROR),
            "FileSystemError",
            "Cannot read plugins directory",
            "",
            0,
            std::string::npos,
            "Check if the Plugins directory is accessible and not corrupted",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3007.md"
        );

        MessageBoxA(NULL, "读取插件目录时出错", "错误", MB_ICONERROR | MB_OK);
    }

    auto pluginsReadEnd = std::chrono::high_resolution_clock::now();
    auto pluginsReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(pluginsReadEnd - pluginsReadStart).count();

   

    Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
        "Total plugins found: " + std::to_string(pluginsFound) +
        ", loaded: " + std::to_string(pluginsLoaded) +
        " (took " + std::to_string(pluginsReadTime) + "ms)");

    return plugins;
}

bool hasPlugin(const std::string& pluginName) {
    std::string pluginDir = "Plugins\\" + pluginName;
    bool exists = fs::exists(pluginDir);

    Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
        "Checking plugin existence: " + pluginName +
        " - " + (exists ? "found" : "not found"));

    return exists;
}

std::string getPluginFullCommand(const PluginInfo& plugin) {
    std::string pluginPath = "Plugins\\" + plugin.name + "\\";
    std::string fullCommand = plugin.runCommand;

    if (plugin.runCommand.find(' ') == std::string::npos &&
        plugin.runCommand.find('\\') == std::string::npos &&
        plugin.runCommand.find('/') == std::string::npos) {

        std::string possibleExe = pluginPath + plugin.runCommand + ".exe";
        if (fs::exists(possibleExe)) {
            Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
                "Found executable at: " + possibleExe);
            fullCommand = possibleExe;
        }
    }

    fullCommand += " " + pluginPath + plugin.runFile;

    Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
        "Built plugin command: " + fullCommand);

    return fullCommand;
}

// ==================== 插件运行 ====================

bool runPlugin(const std::string& pluginName, const std::string& runArgs) {
    auto pluginStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
        "Attempting to run plugin: " + pluginName +
        (runArgs.empty() ? "" : " with args: \"" + runArgs + "\""));

    std::string pluginDir = "Plugins\\" + pluginName;

    if (!fs::exists(pluginDir)) {
        Log(LogGrade::ERR, LogCode::PLUGIN_MISSING,
            "Plugin directory not found: " + pluginDir);

        formatErrorOutput(
            logCodeToString(LogCode::PLUGIN_MISSING),
            "PluginError",
            "Plugin directory not found",
            "",
            0,
            std::string::npos,
            "Make sure the plugin is installed in Plugins/" + pluginName + "/ directory",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/W2002.md"
        );

        MessageBoxA(NULL, ("错误：插件目录不存在 - " + pluginName).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    std::string aboutFilePath = pluginDir + "\\about.cfg";
    if (!fs::exists(aboutFilePath)) {
        Log(LogGrade::ERR, LogCode::PLUGIN_MISSING,
            "about.cfg file not found for plugin: " + pluginName);

        formatErrorOutput(
            logCodeToString(LogCode::PLUGIN_MISSING),
            "PluginError",
            "Plugin configuration file missing",
            "",
            0,
            std::string::npos,
            "Each plugin must have an about.cfg file in its root directory",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/W2002.md"
        );

        MessageBoxA(NULL, ("错误：插件配置文件缺失 - " + pluginName).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    PluginInfo pluginInfo;
    pluginInfo.name = pluginName;

    std::ifstream aboutFile(aboutFilePath);
    if (!aboutFile.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Cannot open about.cfg for plugin: " + pluginName);
        MessageBoxA(NULL, ("错误：无法读取插件配置 - " + pluginName).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    std::string line;
    bool hasRunCommand = false;
    bool hasRunFile = false;
    int configLines = 0;

    while (std::getline(aboutFile, line)) {
        configLines++;
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }

        size_t equalsPos = trimmedLine.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        std::string key = trim(trimmedLine.substr(0, equalsPos));
        std::string value = trim(trimmedLine.substr(equalsPos + 1));

        if (value.length() >= 2 &&
            ((value.front() == '"' && value.back() == '"') ||
                (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }

        if (key == "RunCommand") {
            pluginInfo.runCommand = value;
            hasRunCommand = true;
        }
        else if (key == "RunFile") {
            pluginInfo.runFile = value;
            hasRunFile = true;
        }
        else if (key == "Description") {
            pluginInfo.description = value;
        }
    }
    aboutFile.close();

    Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
        "Plugin " + pluginName + " config: " + std::to_string(configLines) + " lines");

    if (!hasRunCommand) {
        Log(LogGrade::ERR, LogCode::PLUGIN_MISSING,
            "Plugin " + pluginName + " missing RunCommand in about.cfg");
        MessageBoxA(NULL, ("错误：插件配置缺少RunCommand - " + pluginName).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    if (!hasRunFile) {
        Log(LogGrade::ERR, LogCode::PLUGIN_MISSING,
            "Plugin " + pluginName + " missing RunFile in about.cfg");
        MessageBoxA(NULL, ("错误：插件配置缺少RunFile - " + pluginName).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return false;
    }

    if (pluginInfo.runCommand == ".exe" || pluginInfo.runCommand == "bin" || pluginInfo.runCommand == "/") {
        pluginInfo.runCommand = "";
        Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
            "Empty RunCommand detected, using direct execution");
    }

    std::string fullCommand;
    fs::path runFilePath(pluginInfo.runFile);

    if (runFilePath.is_relative()) {
        fullCommand = pluginInfo.runCommand + " " + pluginDir + "\\" + pluginInfo.runFile;
    }
    else {
        fullCommand = pluginInfo.runCommand + " " + pluginInfo.runFile;
    }

    if (!runArgs.empty()) {
        fullCommand += " " + runArgs;
    }

    Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
        "Executing plugin command: " + fullCommand);

    int result = system(fullCommand.c_str());

    auto pluginEndTime = std::chrono::high_resolution_clock::now();
    auto pluginExecTime = std::chrono::duration_cast<std::chrono::milliseconds>(pluginEndTime - pluginStartTime).count();

    if (result == 0) {
       
        Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
            "Plugin executed successfully: " + pluginName +
            " (took " + std::to_string(pluginExecTime) + "ms)");
        return true;
    }
    else {
        Log(LogGrade::ERR, LogCode::PLUGIN_EXEC_FAILED,
            "Plugin execution failed with code " + std::to_string(result) +
            ": " + pluginName + " (took " + std::to_string(pluginExecTime) + "ms)");

        formatErrorOutput(
            logCodeToString(LogCode::PLUGIN_EXEC_FAILED),
            "PluginError",
            "Plugin execution failed",
            "",
            0,
            std::string::npos,
            "Check if the plugin's RunCommand and RunFile are valid and have proper permissions",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3006.md"
        );

        MessageBoxA(NULL, ("插件执行失败，错误代码: " + std::to_string(result)).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return false;
    }
}

// ==================== 配置文件 ====================

std::string readCfg(const std::string& key) {
    auto cfgReadStart = std::chrono::high_resolution_clock::now();

    const std::string filename = "data.cfg";

    Log(LogGrade::INFO, LogCode::GAME_START,
        "Reading configuration key: " + key + " from " + filename);

    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Failed to open configuration file: " + filename);

        formatErrorOutput(
            logCodeToString(LogCode::FILE_OPEN_FAILED),
            "FileError",
            "Cannot open configuration file",
            "",
            0,
            std::string::npos,
            "Make sure data.cfg exists in the application directory",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3004.md"
        );

        MessageBoxA(NULL, ("错误：无法打开配置文件\n" + filename).c_str(),
            "文件错误", MB_ICONERROR | MB_OK);
        return "";
    }

    std::string line;
    int lineNum = 0;
    std::string value;

    while (std::getline(inFile, line)) {
        lineNum++;
        std::string trimmedLine = trim(line);

        if (trimmedLine.empty() || trimmedLine[0] == '#' || trimmedLine[0] == ';') {
            continue;
        }

        size_t equalsPos = trimmedLine.find('=');
        if (equalsPos == std::string::npos) {
            Log(LogGrade::DEBUG, LogCode::GAME_START,
                "Skipping line " + std::to_string(lineNum) + ": no equals sign");
            continue;
        }

        std::string currentKey = trim(trimmedLine.substr(0, equalsPos));

        if (currentKey == key) {
            value = trim(trimmedLine.substr(equalsPos + 1));

            if (value.length() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                    (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.length() - 2);
            }

            Log(LogGrade::INFO, LogCode::GAME_START,
                "Found key: " + key + " = \"" + value + "\" at line " + std::to_string(lineNum));
            break;
        }
    }

    inFile.close();

    auto cfgReadEnd = std::chrono::high_resolution_clock::now();
    auto cfgReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(cfgReadEnd - cfgReadStart).count();

    if (value.empty()) {
        Log(LogGrade::WARNING, LogCode::FILE_NOT_FOUND,
            "Key not found in configuration file: " + key +
            " (scanned " + std::to_string(lineNum) + " lines, took " +
            std::to_string(cfgReadTime) + "ms)");

        formatErrorOutput(
            logCodeToString(LogCode::FILE_NOT_FOUND),
            "ConfigError",
            "Configuration key not found",
            "",
            0,
            std::string::npos,
            "Add '" + key + " = value' to data.cfg file",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/W2001.md"
        );

        MessageBoxA(NULL, ("错误：配置项未找到\n键名: " + key).c_str(),
            "配置错误", MB_ICONWARNING | MB_OK);
    }
    else {
        
    }

    return value;
}

void updateFirstRunFlag(bool value) {
    auto updateStartTime = std::chrono::high_resolution_clock::now();

    Log(LogGrade::INFO, LogCode::GAME_SAVED,
        "Updating FirstRunFlag in configuration file to: " + std::string(value ? "1" : "0"));

    const std::string filename = "data.cfg";
    const std::string targetKey = "FirstRunFlag";

    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Failed to open configuration file for reading: " + filename);
        MessageBoxA(NULL, ("错误：无法打开文件 " + filename).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool keyFound = false;
    int lineNum = 0;

    while (std::getline(inFile, line)) {
        lineNum++;
        if (line.find(targetKey) != std::string::npos) {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string newLine = line.substr(0, equalsPos + 1);
                newLine += " ";
                newLine += (value ? "1" : "0");

                lines.push_back(newLine);
                keyFound = true;

                Log(LogGrade::DEBUG, LogCode::GAME_SAVED,
                    "Updated FirstRunFlag at line " + std::to_string(lineNum) +
                    " from \"" + line + "\" to \"" + newLine + "\"");
            }
            else {
                lines.push_back(line);
                Log(LogGrade::WARNING, LogCode::SAVE_CORRUPTED,
                    "Found FirstRunFlag line without equals sign at line " +
                    std::to_string(lineNum));
            }
        }
        else {
            lines.push_back(line);
        }
    }

    inFile.close();

    if (!keyFound) {
        Log(LogGrade::INFO, LogCode::GAME_SAVED,
            "FirstRunFlag not found in file, appending at end");
        lines.push_back(targetKey + " = " + (value ? "1" : "0"));
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED,
            "Failed to open configuration file for writing: " + filename);
        MessageBoxA(NULL, ("错误：无法写入文件 " + filename).c_str(),
            "错误", MB_ICONERROR | MB_OK);
        return;
    }

    size_t bytesWritten = 0;
    for (const auto& l : lines) {
        outFile << l << std::endl;
        bytesWritten += l.length() + 2;
    }
    outFile.close();

    auto updateEndTime = std::chrono::high_resolution_clock::now();
    auto updateTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(updateEndTime - updateStartTime).count();

    

    Log(LogGrade::INFO, LogCode::GAME_SAVED,
        "Configuration file updated successfully. FirstRunFlag = " + std::string(value ? "1" : "0") +
        " (" + std::to_string(bytesWritten) + " bytes written, took " +
        std::to_string(updateTimeMs) + "ms)");

    return;
}