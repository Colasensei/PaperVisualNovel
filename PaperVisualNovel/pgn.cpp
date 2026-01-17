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
            Run();
            return;
        }
        else if (status == -2) {
            // ESC菜单中选择了不保存退出
            Log(LogGrade::INFO, "ESC menu selected exit without saving");
            Run();
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
    Run();
}

// 实现 Run() 函数
void Run() {
	Log(LogGrade::INFO, "Running main menu...");
    system("cls");
    vnout("PaperVisualNovel", 0.8, white, true);
    vnout("千页小说引擎", 0.8, white, true);
    vnout("ver Beta1.0", 0.8, white, true);
    vnout("1.加载游戏 2.教程 3.关于 4.退出", 0.8, yellow, true);
    Log(LogGrade::INFO, "Running main menu Done");
    while (true) {
        string op = getKeyName();
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
            Run();
			return;
        }

        string where = basePath + folderNames[choice_num - 1] + "\\";
        string file = folderNames[choice_num - 1] + ".pgn";
        string full_path = where + file;

        Log(LogGrade::INFO, "Game choose: "+file);
        Log(LogGrade::DEBUG, "Game path: "+full_path);
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
            cout << "==============================" << endl;
            cout << "检测到存档文件" << endl;
            cout << "1. 继续游戏（从存档开始）" << endl;
            cout << "2. 开始新游戏" << endl;
            cout << "3. 删除存档" << endl;
            cout << "4. 返回" << endl;
            cout << "==============================" << endl;
            cout << "请选择: ";

            string saveChoice = getKeyName();
            Log(LogGrade::DEBUG, "Save choice: " + saveChoice);

            if (saveChoice == "1") {
              
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
                // 返回游戏列表
                Run();
                return;
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
            cout << "关于 PaperVisualNovel" << endl;
            cout << "======================" << endl;
            cout << "PaperVisualNovel 是一个用 C++ 编写的视觉小说引擎。" << endl;
            cout << "你可以使用它来创建你自己的视觉小说。" << endl;
            cout << "感谢使用 PaperVisualNovel！" << endl;
            cout << "作者：colaSensei (in BILIBILI & Github)" << endl;
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

