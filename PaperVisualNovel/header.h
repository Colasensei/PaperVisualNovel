// header.h
#pragma once
#ifndef PGN_HEADER_H
#define PGN_HEADER_H

#define VERSION "Beta 1.2.1"

//color
#define ANSI_WHITE "\033[37m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_RESET "\033[0m"
#define ANSI_BLACK "\033[30m"
#define ANSI_GRAY "\033[90m"
#define ANSI_BWHITE "\033[1;37m"
#define ANSI_BRED "\033[1;31m"
#define ANSI_BGREEN "\033[1;32m"
#define ANSI_BYELLOW "\033[1;33m"
#define ANSI_BBLUE "\033[1;34m"
#define ANSI_BMAGENTA "\033[1;35m"
#define ANSI_BCYAN "\033[1;36m"
#define ANSI_BBLACK "\033[1;30m"
#define ANSI_AQUA "\033[36m"
#define ANSI_PURPLE "\033[35m"

// 系统头文件
#define NOMINMAX
#include <iostream>
#include <fstream>
#include <stack>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <map>
#include <set>
#include <Windows.h>
#include <filesystem>
#include <conio.h>
#include <cstdlib>
#include <shellapi.h>

#include "gamestate.h"
#include "gum_wrapper.h"

using namespace std;
namespace fs = std::filesystem;


// 全局常量
const size_t MAX_FILE_SIZE = 2000 * 1024 * 1024; // 最大文件大小限制



// 枚举和结构体声明
enum color {
    black = 0,
    blue,
    green,
    aqua,
    red,
    purple,
    yellow,
    white,
    gray
};

struct SaveData {
    size_t currentLine;        // 当前执行到的行号
    GameState gameState;       // 游戏状态
    string scriptPath;         // 脚本路径
    string saveTime;           // 保存时间
};
bool loadGame(const std::string& savePath, SaveData& saveData);

struct CurrentGameInfo {
    string scriptPath;
    size_t currentLine;
    GameState* gameState;
};

// 全局变量声明
extern int quantity;
extern CurrentGameInfo g_currentGameInfo;

// 函数声明
void Run();
void RunPgn(const string& where, const string& file, bool loadFromSave = false,
    size_t savedLine = 0, const GameState& savedState = GameState());

#endif // PGN_HEADER_H