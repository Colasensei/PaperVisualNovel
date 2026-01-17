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


    // 初始化随机数种子
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // 设置控制台标题
    SetConsoleTitleA("Paper Visual Novel");
    
    // 检查是否为首次运行
    string firstRun = readCfg("FirstRunFlag");
    if (firstRun=="true"||firstRun=="1") {

        string where = "Novel\\HelloWorld\\";
        string file = "HelloWorld.pgn";
        
        if (fs::exists(where + file)) {
            updateFirstRunFlag(false);

            RunPgn(where, file);
        } else {
            MessageBoxA(NULL, "警告：找不到初始教程文件", 
                       "警告", MB_ICONWARNING | MB_OK);
        }
        
        
    }
    // 运行主菜单
    Run();
    return 0;
}