// fileutils.h
#pragma once
#ifndef FILEUTILS_H
#define FILEUTILS_H

#include "gamestate.h"
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;


// 插件信息结构体
struct PluginInfo {
    std::string name;          // 插件文件夹名
    std::string runCommand;    // 运行命令（如 "python"）
    std::string runFile;       // 运行文件（如 "test.py"）
    std::string description;   // 插件描述（可选）
    std::string version;       // 插件版本（可选）
    std::string author;        // 作者（可选）
};

/**
 * @brief 运行指定插件
 * @param pluginName 插件名称（文件夹名）
 * @param runArgs 运行参数（可选）
 * @return true表示成功执行，false表示失败
 */
bool runPlugin(const std::string& pluginName, const std::string& runArgs = "");


std::string trim(const std::string& str);

// 插件管理函数
std::vector<PluginInfo> readInstalledPlugins();
bool hasPlugin(const std::string& pluginName);
std::string getPluginFullCommand(const PluginInfo& plugin);

// 存档管理
bool saveGame(const std::string& scriptPath, size_t currentLine,
    const GameState& gameState, const std::string& saveName = "autosave");

bool hasSaveFile(const std::string& scriptPath);
std::string getSaveInfo(const std::string& scriptPath);

// 文件安全操作
bool safeViewFile(const std::string& filepath);
void overwriteLine(const std::string& filename, int lineToOverwrite, 
                   const std::string& newContent);



// 结局文件操作
std::vector<std::string> readCollectedEndings(const std::string& gameFolder);
void saveEnding(const std::string& gameFolder, const std::string& endingName, 
                GameState& gameState);
void loadAllEndings(const std::vector<std::string>& lines, GameState& gameState);

// 游戏统计
int countTotalEndingsInScript(const std::string& scriptPath);
std::pair<int, int> getGameEndingStats(const std::string& gameFolderPath);
std::string getGameFolderName(const std::string& fullPath);

// 配置文件
std::string readCfg(const std::string& key);
void updateFirstRunFlag(bool value);

#endif // FILEUTILS_H