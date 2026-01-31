// header.h
#pragma once
#ifndef PGN_HEADER_H
#define PGN_HEADER_H

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