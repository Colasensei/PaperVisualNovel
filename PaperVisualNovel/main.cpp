// main.cpp
#include "header.h"
#include "gamestate.h"
#include "fileutils.h"
#include "ui.h"
// 全局变量定义
int quantity = 0;
bool DebugLogEnabled = false;  // 是否启用调试日志

CurrentGameInfo g_currentGameInfo = { "", 0, nullptr };


/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {

    Log(LogGrade::INFO, "\n\n----------------------------------------");
    Log(LogGrade::INFO, "The program is running...");

    if (readCfg("DebugLogEnabled") == "1")
    {
        DebugLogEnabled=1;
    }

    if (!gum::GumWrapper::is_available()) {
        Log(LogGrade::ERR, "Gum library is not available.");
        MessageBoxA(NULL, "警告：Gum库不可用，即将进行安装。完毕后请重新启动程序。",
                   "警告", MB_ICONWARNING | MB_OK);
        system("winget install charmbracelet.gum");
        cout<<"安装完毕，请重新启动程序。";
        Sleep(5000);
        return 1;
    }

    // 初始化随机数种子
    srand(static_cast<unsigned int>(time(nullptr)));
    Log(LogGrade::DEBUG, "Random seed initialized.");

    
    // 设置控制台标题
    SetConsoleTitleA("Paper Visual Novel");
    Log(LogGrade::DEBUG, "Console title set.");

    // 处理命令行参数
    if (argc > 1) {
        // 有命令行参数，直接运行指定的PGN文件
        std::string filePath = argv[1];
        Log(LogGrade::INFO, "Command line argument detected: " + filePath);

        // 检查文件是否存在
        if (!fs::exists(filePath)) {
            MessageBoxA(NULL, "警告：指定的文件不存在",
                       "警告", MB_ICONWARNING | MB_OK);
            Log(LogGrade::ERR, "File not found: " + filePath);
            return 1;
        }

        // 运行指定的PGN文件
        string where = fs::path(filePath).parent_path().string() + "\\";
        string file = fs::path(filePath).filename().string();
        RunPgn(where, file);
        return 0;
    }

    // 没有命令行参数，正常启动流程
    Log(LogGrade::INFO, "No command line arguments, starting normal flow");
    
    if (!(readCfg("AutoRun") == "0"))
    {
        string pgn = readCfg("AutoRun");
        string where = "Novel\\"+pgn+"\\";
        string file = pgn+".pgn";
        if (!fs::exists(where + file))
        {
            MessageBoxA(NULL, "警告：自动运行文件不存在",
                       "警告", MB_ICONWARNING | MB_OK);
            Log(LogGrade::ERR, "File not found: " + where + file);
            return 1;
        }
        RunPgn(where, file);
        return 0;
    }

    // 检查是否为首次运行
    string firstRun = readCfg("FirstRunFlag");
    Log(LogGrade::DEBUG, "First run flag checked.");
    if (firstRun=="true"||firstRun=="1") {
        Log(LogGrade::DEBUG, "First run detected.");
        string where = "Novel\\HelloWorld\\";
        string file = "HelloWorld.pgn";
        
        if (fs::exists(where + file)) {
			Log(LogGrade::DEBUG, "Initial tutorial file found.");
            updateFirstRunFlag(false);

            RunPgn(where, file);
        } else {
            Log(LogGrade::ERR, "Initial tutorial file not found.");
            MessageBoxA(NULL, "警告：找不到初始教程文件", 
                       "警告", MB_ICONWARNING | MB_OK);
        }
        
        
    }
    // 运行主菜单
    Log(LogGrade::INFO, "Running main menu...");
    Run();
    return 0;
}