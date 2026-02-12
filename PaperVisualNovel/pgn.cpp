// pgn.cpp
#include "gamestate.h"
#include "parser.h"
#include "fileutils.h"
#include "ui.h"
#include "condition.h"
#include <chrono>

// ==================== 字符串转整数（安全版） ====================

bool safeStringToInt(const std::string& str, int& result) {
    try {
        result = std::stoi(str);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

// 实现 RunPgn() 函数
void RunPgn(const string& where, const string& file, bool loadFromSave,
    size_t savedLine, const GameState& savedState) {

    auto gameStartTime = std::chrono::high_resolution_clock::now();

    system("cls");

    Log(LogGrade::INFO, LogCode::GAME_LOADED, "Preparing to run game " + file);
    string pgn = where + file;
    Log(LogGrade::DEBUG, LogCode::GAME_LOADED, "Game file path: " + pgn);

    // 使用Windows API进行正确的编码转换
    int len = MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, wstr, len);
    SetConsoleTitle((L"Paper Visual Novel   " + std::wstring(wstr)).c_str());
    delete[] wstr;

    auto fileReadStart = std::chrono::high_resolution_clock::now();
    ifstream in(pgn);
    if (!in.is_open()) {
        Log(LogGrade::ERR, LogCode::FILE_OPEN_FAILED, "Failed to open game file " + pgn);
        formatErrorOutput(
            logCodeToString(LogCode::FILE_OPEN_FAILED),
            "FileError",
            "Cannot open game file",
            "",
            0,
            std::string::npos,
            "Check if file exists and has read permissions",
            "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3004.md"
        );
        MessageBoxA(NULL, "错误：无法打开游戏文件", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    vector<string> lines;
    string line;
    size_t fileSize = 0;

    while (getline(in, line)) {
        lines.push_back(line);
        fileSize += line.length() + 2; // 估计文件大小
    }
    in.close();

    auto fileReadEnd = std::chrono::high_resolution_clock::now();
    auto fileReadTime = std::chrono::duration_cast<std::chrono::milliseconds>(fileReadEnd - fileReadStart).count();

    

    auto labelParseStart = std::chrono::high_resolution_clock::now();
    map<string, int> labels = parseLabels(lines);
    auto labelParseEnd = std::chrono::high_resolution_clock::now();
    auto labelParseTime = std::chrono::duration_cast<std::chrono::milliseconds>(labelParseEnd - labelParseStart).count();

    

    GameState gameState;

    if (loadFromSave) {
        gameState = savedState;
        Log(LogGrade::INFO, LogCode::GAME_LOADED, "Loaded game state from save");
    }
    else {
        string gameFolder = "";
        size_t novelPos = where.find("Novel\\");
        if (novelPos != string::npos) {
            size_t startPos = novelPos + 6;
            size_t endPos = where.find("\\", startPos);
            if (endPos != string::npos) {
                gameFolder = where.substr(startPos, endPos - startPos);
            }
        }

        if (!gameFolder.empty()) {
            auto endingsLoadStart = std::chrono::high_resolution_clock::now();
            vector<string> collectedEndings = readCollectedEndings(gameFolder);
            for (const auto& ending : collectedEndings) {
                gameState.addEnding(ending);
            }
            auto endingsLoadEnd = std::chrono::high_resolution_clock::now();
            auto endingsLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endingsLoadEnd - endingsLoadStart).count();

            
        }
    }

    auto allEndingsStart = std::chrono::high_resolution_clock::now();
    loadAllEndings(lines, gameState);
    auto allEndingsEnd = std::chrono::high_resolution_clock::now();
    auto allEndingsTime = std::chrono::duration_cast<std::chrono::milliseconds>(allEndingsEnd - allEndingsStart).count();

    

    size_t currentLine = loadFromSave ? savedLine : 0;

    // 设置全局游戏信息
    g_currentGameInfo.scriptPath = pgn;
    g_currentGameInfo.gameState = &gameState;
    Log(LogGrade::DEBUG, LogCode::GAME_LOADED, "Set global game info");

    auto gameLoadEnd = std::chrono::high_resolution_clock::now();
    auto gameLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(gameLoadEnd - gameStartTime).count();

    Log(LogGrade::INFO, LogCode::GAME_START, "Starting game loop");

    int executedLines = 0;
    auto loopStartTime = std::chrono::high_resolution_clock::now();

    while (currentLine < lines.size()) {
        executedLines++;
        auto lineExecStart = std::chrono::high_resolution_clock::now();

        // 更新当前行号
        g_currentGameInfo.currentLine = currentLine;

        auto [status, nextLine] = executeLine(lines[currentLine], gameState,
            currentLine, lines, where, 0, labels);

        auto lineExecEnd = std::chrono::high_resolution_clock::now();
        auto lineExecTime = std::chrono::duration_cast<std::chrono::microseconds>(lineExecEnd - lineExecStart).count();

        if (executedLines % 100 == 0) {
            
        }

        if (status == -1) {
            Log(LogGrade::INFO, LogCode::GAME_SAVED, "ESC menu selected save and exit");
            return;
        }
        else if (status == -2) {
            Log(LogGrade::INFO, LogCode::GAME_START, "ESC menu selected exit without saving");
            return;
        }
        else if (status == 1) {
            currentLine = nextLine;
            Log(LogGrade::DEBUG, LogCode::GAME_START, "DEBUG terminal Jumped to line " + to_string(currentLine + 1));
        }
        else {
            currentLine = nextLine;
        }
    }

    auto loopEndTime = std::chrono::high_resolution_clock::now();
    auto loopTotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(loopEndTime - loopStartTime).count();

    

    if (executedLines > 0) {
        float avgTimePerLine = static_cast<float>(loopTotalTime) / executedLines;
        Log(LogGrade::DEBUG, LogCode::PERFORMANCE,
            "Average execution time: " + std::to_string(avgTimePerLine) + "ms per line");
    }

    // 游戏正常结束，清除全局信息
    g_currentGameInfo = { "", 0, nullptr };
    Log(LogGrade::INFO, LogCode::GAME_START, "Game loop finished");

    cout << "脚本执行完毕" << endl;
    system("pause");
    Log(LogGrade::INFO, LogCode::GAME_START, "Game finished");
    return;
}

// 实现 Run() 函数
void Run() {

    while (true) {
        Log(LogGrade::INFO, LogCode::GAME_START, "Running main menu...");
        system("cls");
        printf("%s\n", "   ___  ______  __");
        printf("%s\n", "  / _ \\/ ___/ |/ /");
        printf("%s\n", " / ___/ (_ /    / ");
        printf("%s\n", "/_/   \\___/_/|_/  ");
        printf("%s\n", "                  ");

        vnout("PaperVisualNovel", 0.8, white, true);
        vnout("千页小说引擎", 0.8, white, true);
        vnout(VERSION, 0.8, white, true);
        cout << endl;
        Log(LogGrade::INFO, LogCode::GAME_START, "Running main menu Done");
        std::vector<std::string> menu_options = {
         "1. 加载游戏",
         "2. 教程",
         "3. 插件",
         "4. 关于",
         "5. 退出"
        };
        std::string selected = gum::GumWrapper::choose(
            menu_options
        );
        std::string op = "";
        if (!selected.empty()) {
            // 提取第一个字符作为选项（"1. 加载游戏" -> "1"）
            op = selected.substr(0, 1);

        }
        else {
            cout << "未选择任何选项" << endl;
        }
        Log(LogGrade::INFO, LogCode::GAME_START, "Running main menu op: " + op);
        if (op == "1") {
            string basePath = "Novel\\";
            Log(LogGrade::INFO, LogCode::GAME_START, "Load Game choose Menu.");
            if (!fs::exists(basePath)) {
                Log(LogGrade::ERR, LogCode::FILE_NOT_FOUND, "Game directory does not exist");
                MessageBoxA(NULL, "错误：游戏目录不存在", "错误", MB_ICONERROR | MB_OK);
                continue;
            }

            vector<string> folderNames;
            vector<pair<int, int>> endingStats;
            vector<string> saveInfos;

            for (const auto& entry : fs::directory_iterator(basePath)) {
                if (entry.is_directory()) {
                    string folderPath = entry.path().string() + "\\";
                    string folderName = getGameFolderName(entry.path().string());
                    folderNames.push_back(folderName);

                    auto stats = getGameEndingStats(folderPath);
                    endingStats.push_back(stats);

                    string pgnFile = folderPath + folderName + ".pgn";
                    if (fs::exists(pgnFile)) {
                        saveInfos.push_back(getSaveInfo(pgnFile));
                    }
                    else {
                        saveInfos.push_back("无游戏文件");
                    }
                }
            }
            if (folderNames.empty()) {
                Log(LogGrade::ERR, LogCode::FILE_NOT_FOUND, "No game folders found");
                MessageBoxA(NULL, "错误：没有找到游戏文件夹", "错误", MB_ICONERROR | MB_OK);
            }
            else {
                system("cls");
                cout << "========== 游戏列表 ==========" << endl;
                cout << "（括号内为结局收集情况，右侧为存档状态）" << endl;
                cout << "==============================" << endl;
                cout << endl;

                for (size_t i = 0; i < folderNames.size(); i++) {
                    int collected = endingStats[i].first;
                    int total = endingStats[i].second;

                    cout << i + 1 << ". " << folderNames[i];

                    if (total > 0) {
                        float percentage = (total > 0) ? (static_cast<float>(collected) / total * 100) : 0;

                        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

                        if (collected == total && total > 0) {
                            SetConsoleTextAttribute(hConsole, 10);
                            cout << " [" << collected << "/" << total << "]";
                        }
                        else if (percentage >= 50) {
                            SetConsoleTextAttribute(hConsole, 14);
                            cout << " [" << collected << "/" << total << "]";
                        }
                        else if (collected > 0) {
                            SetConsoleTextAttribute(hConsole, 13);
                            cout << " [" << collected << "/" << total << "]";
                        }
                        else {
                            SetConsoleTextAttribute(hConsole, 8);
                            cout << " [" << collected << "/" << total << "]";
                        }

                        SetConsoleTextAttribute(hConsole, 7);
                    }
                    else {
                        cout << " [无结局]";
                    }

                    cout << "   ";
                    if (saveInfos[i] != "无存档") {
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
                        cout << saveInfos[i];
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
                    }
                    else {
                        cout << saveInfos[i];
                    }

                    cout << endl;
                }

                cout << endl;
                cout << "请选择游戏 (输入数字): ";
                Log(LogGrade::INFO, LogCode::GAME_START, "Game choose menu loaded.");

            getKeyforGameMenu:
                string choice_str = getKeyName();
                int choice_num;
                Log(LogGrade::DEBUG, LogCode::GAME_START, "menu choice_str: " + choice_str);
                if (choice_str == "ESC") {
                    Log(LogGrade::INFO, LogCode::GAME_START, "Game choose menu exit.");
                    continue;
                    return;
                }

                if (!safeStringToInt(choice_str, choice_num) ||
                    choice_num < 1 ||
                    choice_num > static_cast<int>(folderNames.size())) {
                    Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid choice");
                    MessageBoxA(NULL, "错误：无效的选择", "错误", MB_ICONERROR | MB_OK);
                    goto getKeyforGameMenu;
                }

                string where = basePath + folderNames[choice_num - 1] + "\\";
                string file = folderNames[choice_num - 1] + ".pgn";
                string full_path = where + file;

                Log(LogGrade::INFO, LogCode::GAME_LOADED, "Game choose: " + file);
                Log(LogGrade::DEBUG, LogCode::GAME_LOADED, "Game path: " + full_path);
                if (!fs::exists(full_path)) {
                    Log(LogGrade::ERR, LogCode::FILE_NOT_FOUND, "Game file not found");
                    MessageBoxA(NULL, "错误：找不到游戏文件", "错误", MB_ICONERROR | MB_OK);
                    goto getKeyforGameMenu;
                    return;
                }

                if (hasSaveFile(full_path)) {
                    Log(LogGrade::INFO, LogCode::GAME_LOADED, "Save file found");
                    system("cls");
                    cout << "检测到存档文件，是否继续游戏？" << endl;
                    vector<string> save_menu_options = {
                        "1. 继续游戏（从存档开始）",
                        "2. 开始新游戏",
                        "3. 删除存档",
                        "4. 返回"
                    };

                    std::string selected = "";
                    std::string saveChoice = "";

                    if (gum::GumWrapper::is_available()) {
                        try {
                            selected = gum::GumWrapper::choose(save_menu_options);

                            if (!selected.empty()) {
                                saveChoice = selected.substr(0, 1);
                            }
                        }
                        catch (const std::exception& e) {
                            Log(LogGrade::ERR, LogCode::PLUGIN_EXEC_FAILED, "Gum selection error: " + std::string(e.what()));
                            saveChoice = "";
                        }
                    }

                    if (saveChoice.empty() || !(saveChoice == "1" || saveChoice == "2" || saveChoice == "3" || saveChoice == "4")) {
                        Log(LogGrade::WARNING, LogCode::FALLBACK_USED, "Falling back to original save menu display");
                        cout << "==============================" << endl;
                        cout << "检测到存档文件" << endl;
                        cout << "1. 继续游戏（从存档开始）" << endl;
                        cout << "2. 开始新游戏" << endl;
                        cout << "3. 删除存档" << endl;
                        cout << "4. 返回" << endl;
                        cout << "==============================" << endl;
                        cout << "请选择: ";

                        saveChoice = getKeyName();
                    }

                    Log(LogGrade::DEBUG, LogCode::GAME_LOADED, "Save choice: " + saveChoice);

                    if (saveChoice == "1") {
                        SaveData saveData;
                        fs::path savePath = fs::path(where) / "saves" / "autosave.sav";
                        Log(LogGrade::DEBUG, LogCode::GAME_LOADED, "Load save path: " + savePath.string());

                        if (loadGame(savePath.string(), saveData)) {
                            Log(LogGrade::INFO, LogCode::GAME_LOADED, "Save file loaded");
                            RunPgn(where, file, true, saveData.currentLine, saveData.gameState);
                        }
                        else {
                            Log(LogGrade::ERR, LogCode::SAVE_CORRUPTED, "Save file load failed");
                            MessageBoxA(NULL, "错误：无法加载存档", "错误", MB_ICONERROR | MB_OK);
                            RunPgn(where, file);
                        }
                    }
                    else if (saveChoice == "2") {
                        Log(LogGrade::INFO, LogCode::GAME_START, "Start new game");
                        RunPgn(where, file);
                    }
                    else if (saveChoice == "3") {
                        Log(LogGrade::INFO, LogCode::GAME_SAVED, "Delete save file");
                        fs::path savePath = fs::path(where) / "saves" / "autosave.sav";
                        if (fs::remove(savePath)) {
                            Log(LogGrade::INFO, LogCode::GAME_SAVED, "Save file deleted");
                            cout << "存档已删除" << endl;
                            Sleep(1000);
                        }
                        RunPgn(where, file);
                    }
                    else if (saveChoice == "4") {
                        Log(LogGrade::INFO, LogCode::GAME_START, "Return to main menu");
                        return;
                    }
                    else {
                        Log(LogGrade::WARNING, LogCode::JUMP_INVALID, "Invalid save choice, default to new game");
                        RunPgn(where, file);
                    }
                }
                else {
                    Log(LogGrade::INFO, LogCode::GAME_START, "No save file found");
                    RunPgn(where, file);
                }
                continue;
            }
        }
        else if (op == "2") {
            Log(LogGrade::INFO, LogCode::GAME_START, "Tutorial selected");
            string where = "Novel\\HelloWorld\\";
            string file = "HelloWorld.pgn";

            if (fs::exists(where + file)) {
                Log(LogGrade::DEBUG, LogCode::GAME_LOADED, "Tutorial file found");
                RunPgn(where, file);
            }
            else {
                Log(LogGrade::ERR, LogCode::FILE_NOT_FOUND, "Tutorial file not found");
                MessageBoxA(NULL, "错误：找不到教程文件", "错误", MB_ICONERROR | MB_OK);
            }

            system("cls");
            continue;
        }
        else if (op == "3") {
            Log(LogGrade::INFO, LogCode::GAME_START, "Plugin management selected");

            std::vector<PluginInfo> plugins = readInstalledPlugins();

            if (plugins.empty()) {
                system("cls");
                std::cout << "========== 插件管理 ==========" << std::endl;
                std::cout << "当前没有安装任何插件。" << std::endl;
                std::cout << "==============================" << std::endl;
                system("pause");
                continue;
            }
            else {
                system("cls");
                std::cout << "========== 已安装插件 ==========" << std::endl;
                std::cout << "插件数量: " << plugins.size() << std::endl;
                std::cout << "================================" << std::endl;
                std::cout << std::endl;

                for (size_t i = 0; i < plugins.size(); i++) {
                    const PluginInfo& plugin = plugins[i];

                    std::cout << i + 1 << ". " << plugin.name;

                    if (!plugin.version.empty()) {
                        std::cout << " v" << plugin.version;
                    }

                    std::cout << std::endl;

                    if (!plugin.description.empty()) {
                        std::cout << "   描述: " << plugin.description << std::endl;
                    }

                    if (!plugin.author.empty()) {
                        std::cout << "   作者: " << plugin.author << std::endl;
                    }

                    std::cout << "   命令: " << plugin.runCommand << " " << plugin.runFile << std::endl;
                    std::cout << std::endl;
                }
                system("pause");
                continue;
            }
        }
        else if (op == "4") {
            Log(LogGrade::INFO, LogCode::GAME_START, "About selected");
            system("cls");
            printf("%s\n", "   ___                    ");
            printf("%s\n", "  / _ \\___ ____  ___ ____ ");
            printf("%s\n", " / ___/ _ `/ _ \\/ -_) __/ ");
            printf("%s\n", "/_/   \\_,_/ .__/\\__/_/    ");
            printf("%s\n", "  _   ___/_/            __");
            printf("%s\n", " | | / (_)__ __ _____ _/ /");
            printf("%s\n", " | |/ / (_-</ // / _ `/ / ");
            printf("%s\n", " |___/_/___/\\_,_/\\_,_/_/  ");
            printf("%s\n", "  / |/ /__ _  _____ / /   ");
            printf("%s\n", " /    / _ \\ |/ / -_) /    ");
            printf("%s\n", "/_/|_/\\___/___/\\__/_/     ");
            printf("%s\n", "                          ");
            cout << "关于 PaperVisualNovel" << endl;
            cout << "======================" << endl;
            cout << "PaperVisualNovel 是一个用 C++ 编写的视觉小说引擎。" << endl;
            cout << "你可以使用它来创建你自己的视觉小说。" << endl;
            cout << "感谢使用 PaperVisualNovel！" << endl;
            cout << "作者：colaSensei (in BILIBILI & Github)" << endl;
            std::cout << "软件包含了以下组件：" << std::endl;
            std::cout << " - Gum: Copyright (c) 2024 go-gum" << std::endl;
            cout << "构建日期：" << __DATE__ << endl;
            cout << "版本号：" << VERSION << endl;
            cout << endl << "按任意键返回..." << endl;
            getKeyName();
            Log(LogGrade::INFO, LogCode::GAME_START, "Return to main menu");
            continue;
        }
        else if (op == "5") {
            Log(LogGrade::INFO, LogCode::GAME_START, "Exit selected");
            Log(LogGrade::INFO, LogCode::GAME_START, "Thank you for using PaperVisualNovel");
            exit(0);
        }
    }
}