// pgn.cpp
#include "gamestate.h"
#include "parser.h"
#include "fileutils.h"
#include "ui.h"
#include "condition.h"

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
    size_t savedLine, const GameState& savedState ) {
    system("cls");

    Log(LogGrade::INFO, "Preparing to run game "+file);
    string pgn = where + file;
    Log(LogGrade::DEBUG, "Game file path: " + pgn);
    // 使用Windows API进行正确的编码转换
    int len = MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, wstr, len);
    SetConsoleTitle((L"Paper Visual Novel   " + std::wstring(wstr)).c_str());
    delete[] wstr;




    ifstream in(pgn);
    if (!in.is_open()) {
        Log(LogGrade::ERR, "Failed to open game file "+pgn);
        MessageBoxA(NULL, "错误：无法打开游戏文件", "错误", MB_ICONERROR | MB_OK);
        return;
    }

    vector<string> lines;
    string line;
    while (getline(in, line)) {
        lines.push_back(line);
    }
    in.close();
    Log(LogGrade::DEBUG, "Read " + to_string(lines.size()) + " lines from game file");
    map<string, int> labels = parseLabels(lines);
    Log(LogGrade::DEBUG, "Parsed labels");
    GameState gameState;
    Log(LogGrade::DEBUG, "Created game state");

    if (loadFromSave) {
        gameState = savedState;
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
            vector<string> collectedEndings = readCollectedEndings(gameFolder);
            for (const auto& ending : collectedEndings) {
                gameState.addEnding(ending);
            }
        }
    }
    Log(LogGrade::DEBUG, "Loaded collected endings");
    //d
    loadAllEndings(lines, gameState);
    Log(LogGrade::DEBUG, "Loaded all endings");
    size_t currentLine = loadFromSave ? savedLine : 0;

    // 设置全局游戏信息
    g_currentGameInfo.scriptPath = pgn;
    g_currentGameInfo.gameState = &gameState;
    Log(LogGrade::DEBUG, "Set global game info");


    Log(LogGrade::INFO, "Starting game loop");

    while (currentLine < lines.size()) {
        // 更新当前行号
        g_currentGameInfo.currentLine = currentLine;
        
        auto [status, nextLine] = executeLine(lines[currentLine], gameState,
            currentLine, lines, where, 0, labels);

        if (status == -1) {
            // ESC菜单中选择了保存并退出
            Log(LogGrade::INFO, "ESC menu selected save and exit");
            return;
        }
        else if (status == -2) {
            // ESC菜单中选择了不保存退出
            Log(LogGrade::INFO, "ESC menu selected exit without saving");
            return;
        }
        else if (status == 1) {
            currentLine = nextLine;
            Log(LogGrade::DEBUG, "DEBUG terminal Jumped to line " + to_string(currentLine+1));
        }
        else {
            currentLine = nextLine;
            Log(LogGrade::DEBUG, "Moved to line " + to_string(currentLine+1));
        }
    }

    // 游戏正常结束，清除全局信息
    g_currentGameInfo = { "", 0, nullptr };
    Log(LogGrade::INFO, "Game loop finished");

    cout << "脚本执行完毕" << endl;
    system("pause");
    Log(LogGrade::INFO, "Game finished");
    return;
}

// 实现 Run() 函数
void Run() {
    Log(LogGrade::INFO, "Running main menu...");
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
    Log(LogGrade::INFO, "Running main menu Done");
    while (true) {
        std::vector<std::string> menu_options = {
         "1. 加载游戏",
         "2. 教程",
         "3. 关于",
         "4. 退出"
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
        Log(LogGrade::INFO, "Running main menu op: " + op);
        if (op == "1") {
            string basePath = "Novel\\";
            Log(LogGrade::INFO, "Load Game choose Menu.");
            if (!fs::exists(basePath)) {
                Log(LogGrade::ERR, "Game directory does not exist");
                MessageBoxA(NULL, "错误：游戏目录不存在", "错误", MB_ICONERROR | MB_OK);
                continue;
            }

            vector<string> folderNames;
            vector<pair<int, int>> endingStats; // 存储每个游戏的结局统计
            vector<string> saveInfos; // 新增：存档信息

            // 收集游戏文件夹信息
            for (const auto& entry : fs::directory_iterator(basePath)) {
                if (entry.is_directory()) {
                    string folderPath = entry.path().string() + "\\";
                    string folderName = getGameFolderName(entry.path().string());
                    folderNames.push_back(folderName);

                    // 获取该游戏的结局统计
                    auto stats = getGameEndingStats(folderPath);
                    endingStats.push_back(stats);
                    // 检查存档状态
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
                Log(LogGrade::ERR, "No game folders found");
                MessageBoxA(NULL, "错误：没有找到游戏文件夹", "错误", MB_ICONERROR | MB_OK);
            }
            else {
                system("cls");
                cout << "========== 游戏列表 ==========" << endl;
                cout << "（括号内为结局收集情况，右侧为存档状态）" << endl;
                cout << "==============================" << endl;
                cout << endl;

                // 显示带结局统计和存档状态的游戏列表
                for (size_t i = 0; i < folderNames.size(); i++) {
                    int collected = endingStats[i].first;
                    int total = endingStats[i].second;

                    cout << i + 1 << ". " << folderNames[i];

                    // 显示结局收集情况
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

                    // 显示存档状态（右对齐）
                    cout << "   ";
                    if (saveInfos[i] != "无存档") {
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10); // 绿色
                        cout << saveInfos[i];
                        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
                    }
                    else {
                        cout << saveInfos[i];
                    }

                    cout << endl;
                }

                cout << endl;

                cout << endl;
                cout << "请选择游戏 (输入数字): ";
                Log(LogGrade::INFO, "Game choose menu loaded.");

                getKeyforGameMenu:
                string choice_str = getKeyName();
                int choice_num;
                Log(LogGrade::DEBUG, "menu choice_str: " + choice_str);
                if (choice_str == "ESC") {
                    Run();
                    return;
                }

                if (!safeStringToInt(choice_str, choice_num) ||
                    choice_num < 1 ||
                    choice_num > static_cast<int>(folderNames.size())) {
                    Log(LogGrade::ERR, "Invalid choice");
                    MessageBoxA(NULL, "错误：无效的选择", "错误", MB_ICONERROR | MB_OK);
                    goto getKeyforGameMenu;
                }

                string where = basePath + folderNames[choice_num - 1] + "\\";
                string file = folderNames[choice_num - 1] + ".pgn";
                string full_path = where + file;

                Log(LogGrade::INFO, "Game choose: " + file);
                Log(LogGrade::DEBUG, "Game path: " + full_path);
                if (!fs::exists(full_path)) {
                    Log(LogGrade::ERR, "Game file not found");
                    MessageBoxA(NULL, "错误：找不到游戏文件", "错误", MB_ICONERROR | MB_OK);
                    Run();
                    return;
                }

                // 检查是否有存档
                if (hasSaveFile(full_path)) {
                    Log(LogGrade::INFO, "Save file found");
                    system("cls");
                    cout << "检测到存档文件，是否继续游戏？" << endl;
                    // 存档菜单选项
                    std::vector<std::string> save_menu_options = {
                        "1. 继续游戏（从存档开始）",
                        "2. 开始新游戏",
                        "3. 删除存档",
                        "4. 返回"
                    };

                    std::string selected = "";
                    std::string saveChoice = "";

                    // 使用gum进行选择
                    if (gum::GumWrapper::is_available()) {
                        try {
                            selected = gum::GumWrapper::choose(save_menu_options);

                            if (!selected.empty()) {
                                // 提取第一个字符作为选项数字
                                saveChoice = selected.substr(0, 1);
                            }
                        }
                        catch (const std::exception& e) {
                            Log(LogGrade::ERR, "Gum selection error: " + std::string(e.what()));
                            // gum失败，回退到原始方法
                            saveChoice = "";
                        }
                    }

                    // 如果gum不可用或失败，回退到原始方法
                    if (saveChoice.empty() || !(saveChoice == "1" || saveChoice == "2" || saveChoice == "3" || saveChoice == "4")) {
                        Log(LogGrade::INFO, "Falling back to original save menu display");
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

                    Log(LogGrade::DEBUG, "Save choice: " + saveChoice);

                    // 处理选择结果
                    if (saveChoice == "1") {
                        // 继续游戏（从存档开始）
                        SaveData saveData;
                        fs::path savePath = fs::path(where) / "saves" / "autosave.sav";
                        Log(LogGrade::DEBUG, "Load save path: " + savePath.string());

                        if (loadGame(savePath.string(), saveData)) {
                            Log(LogGrade::INFO, "Save file loaded");
                            RunPgn(where, file, true, saveData.currentLine, saveData.gameState);
                        }
                        else {
                            Log(LogGrade::ERR, "Save file load failed");
                            MessageBoxA(NULL, "错误：无法加载存档", "错误", MB_ICONERROR | MB_OK);
                            RunPgn(where, file);
                        }
                    }
                    else if (saveChoice == "2") {
                        Log(LogGrade::INFO, "Start new game");
                        // 开始新游戏
                        RunPgn(where, file);
                    }
                    else if (saveChoice == "3") {
                        Log(LogGrade::INFO, "Delete save file");
                        // 删除存档
                        fs::path savePath = fs::path(where) / "saves" / "autosave.sav";
                        if (fs::remove(savePath)) {
                            Log(LogGrade::INFO, "Save file deleted");
                            cout << "存档已删除" << endl;
                            Sleep(1000);
                        }
                        RunPgn(where, file);
                    }
                    else if (saveChoice == "4") {
                        Log(LogGrade::INFO, "Return to main menu");

                        return;
                    }
                    else {
                        // 无效选择，默认开始新游戏
                        Log(LogGrade::WARNING, "Invalid save choice, default to new game");
                        RunPgn(where, file);
                    }
                }
                else {
                    Log(LogGrade::INFO, "No save file found");
                    // 没有存档，直接开始新游戏
                    RunPgn(where, file);
                }

                Run();
            }
        }
        else if (op == "2") {
            Log(LogGrade::INFO, "Tutorial selected");
            string where = "Novel\\HelloWorld\\";
            string file = "HelloWorld.pgn";

            if (fs::exists(where + file)) {
                Log(LogGrade::DEBUG, "Tutorial file found");
                RunPgn(where, file);
            }
            else {
                Log(LogGrade::ERR, "Tutorial file not found");
                MessageBoxA(NULL, "错误：找不到教程文件", "错误", MB_ICONERROR | MB_OK);
            }

            system("cls");
            Run();
        }
        else if (op == "3") {
            Log(LogGrade::INFO, "About selected");
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
            cout << "构建日期：" << __DATE__ << endl;
            cout << "版本号：" << VERSION << endl;
            cout << endl << "按任意键返回..." << endl;
            getKeyName();
            Log(LogGrade::INFO, "Return to main menu");
            Run();
        }
        else if (op == "4") {

            Log(LogGrade::INFO, "Exit selected");
            Log(LogGrade::INFO, "Thank you for using PaperVisualNovel");
            exit(0);
        }
    }

}
