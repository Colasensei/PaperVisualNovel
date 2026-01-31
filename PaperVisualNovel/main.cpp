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
int main() {
    if (readCfg("DebugLogEnabled") == "1")
    {
        DebugLogEnabled=1;
    }

    if (!gum::GumWrapper::is_available()) {
        MessageBoxA(NULL, "警告：Gum库不可用，即将进行安装。完毕后请重新启动程序。",
                   "警告", MB_ICONWARNING | MB_OK);
        system("winget install charmbracelet.gum");
        return 1;
    }

    Log(LogGrade::INFO, "\n\n----------------------------------------");
    Log(LogGrade::INFO, "The program is running...");
    // 初始化随机数种子
    srand(static_cast<unsigned int>(time(nullptr)));
    Log(LogGrade::DEBUG, "Random seed initialized.");

    
    // 设置控制台标题
    SetConsoleTitleA("Paper Visual Novel");
    Log(LogGrade::DEBUG, "Console title set.");
    
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