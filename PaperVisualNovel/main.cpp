// main.cpp
#include "header.h"
#include "gamestate.h"
#include "fileutils.h"
#include "ui.h"
// 全局变量定义
int quantity = 0;
CurrentGameInfo g_currentGameInfo = { "", 0, nullptr };

/**
 * @brief 主函数
 */
int main() {
    Log(LogGrade::INFO, "----------------------------------------");
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