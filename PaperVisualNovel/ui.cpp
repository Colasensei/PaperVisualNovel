// ui.cpp
#include "ui.h"
#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <conio.h>
#include <vector>
#include "fileutils.h"
#include "gamestate.h"

extern bool DebugLogEnabled;

std::string logGradeToString(LogGrade logGrade) {
    switch (logGrade) {
    case LogGrade::INFO:    return "INFO";
    case LogGrade::WARNING: return "WARNING";
    case LogGrade::ERR:   return "ERROR";
    case LogGrade::DEBUG:   return "DEBUG";
    default:                return "UNKNOWN";
    }
}


// 有效命令列表
const std::vector<std::string> validCommands = {
    "help", "vars", "set", "add", "history", "endings", "info", "goto", "exit", "quit", "log"
};

// 日志等级映射表
const std::map<std::string, LogGrade> logGradeMap = {
    {"info", LogGrade::INFO},
    {"warning", LogGrade::WARNING},
    {"error", LogGrade::ERR},
    {"debug", LogGrade::DEBUG}
};

// ==================== 控制台颜色输出 ====================

void vnout(const std::string& out, double time, color color,
           bool with_newline, bool use_typewriter_effect) {
    // 设置颜色
    switch (color) {
    case black: std::cout << "\033[30m"; break;
    case blue: std::cout << "\033[34m"; break;
    case green: std::cout << "\033[32m"; break;
    case aqua: std::cout << "\033[36m"; break;
    case red: std::cout << "\033[31m"; break;
    case purple: std::cout << "\033[35m"; break;
    case yellow: std::cout << "\033[33m"; break;
    case white: std::cout << "\033[37m"; break;
    case gray: std::cout << "\033[90m"; break;
    default: std::cout << "\033[37m"; break;
    }

    if (out.empty()) {
        if (with_newline) std::cout << std::endl;
        std::cout << "\033[37m";
        return;
    }

    int total_delay_ms = static_cast<int>(time * 1000);
    if (total_delay_ms <= 0) {
        std::cout << out;
        if (with_newline) std::cout << std::endl;
        std::cout << "\033[37m";
        return;
    }

    int char_delay = total_delay_ms / static_cast<int>(out.length());
    if (char_delay < 10) char_delay = 10;

    for (size_t i = 0; i < out.length(); i++) {
        std::cout << out[i] << std::flush;
        if (use_typewriter_effect) {
            if (out[i] == ',' || out[i] == ';') {
                Sleep(char_delay * 3);
            }
            else if (out[i] == '!' || out[i] == '?') {
                Sleep(char_delay * 5);
            }
            else {
                Sleep(char_delay);
            }
        }
        else {
            Sleep(char_delay);
        }
    }

    if (with_newline) {
        std::cout << std::endl;
    }
    std::cout << "\033[37m";
}

// ==================== 显示进度条 ====================

void simpleProgressBar(float progress) {
    int barWidth = 50;
    int pos = static_cast<int>(barWidth * progress);

    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;

    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else std::cout << " ";
    }
    std::cout << "] " << static_cast<int>(progress * 100.0f) << "%\r";
    std::cout.flush();
}

// ==================== 获取当前时间字符串 ====================

std::string logtimer() {
    time_t now = time(nullptr);
    tm tm_struct;
    localtime_s(&tm_struct, &now);
    std::stringstream ss;
    ss << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S");
    return '[' + ss.str() + ']';
}

// ==================== 获取按键名称 ====================

std::string getKeyName() {
    int key = _getch();

    if (key == 0 || key == 224) {
        int extKey = _getch();

        switch (extKey) {
        case 59: return "F1";
        case 60: return "F2";
        case 61: return "F3";
        case 62: return "F4";
        case 63: return "F5";
        case 64: return "F6";
        case 65: return "F7";
        case 66: return "F8";
        case 67: return "F9";
        case 68: return "F10";
        case 133: return "F11";
        case 134: return "F12";
        case 72: return "UP";
        case 80: return "DOWN";
        case 75: return "LEFT";
        case 77: return "RIGHT";
        case 71: return "HOME";
        case 79: return "END";
        case 73: return "PAGE_UP";
        case 81: return "PAGE_DOWN";
        case 82: return "INSERT";
        case 83: return "DELETE";
        case 141: return "NUMPAD_/";
        default: return "UNKNOWN_EXT_" + std::to_string(extKey);
        }
    }

    switch (key) {
    case 8: return "BACKSPACE";
    case 9: return "TAB";
    case 13: return "ENTER";
    case 27: return "ESC";
    case 32: return "SPACE";
    case 33: return "PAGE_UP_ALT";
    case 34: return "PAGE_DOWN_ALT";
    case 35: return "END_ALT";
    case 36: return "HOME_ALT";
    case 37: return "LEFT_ALT";
    case 38: return "UP_ALT";
    case 39: return "RIGHT_ALT";
    case 40: return "DOWN_ALT";
    case 45: return "INSERT_ALT";
    case 46: return "DELETE_ALT";
    default:
        if (key >= 32 && key <= 126) {
            return std::string(1, static_cast<char>(key));
        }
        return "UNKNOWN_" + std::to_string(key);
    }
}

// ==================== 计算编辑距离 ====================

int calculateEditDistance(const std::string& s1, const std::string& s2) {
    const int m = s1.length();
    const int n = s2.length();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (int i = 0; i <= m; ++i) {
        for (int j = 0; j <= n; ++j) {
            if (i == 0) {
                dp[i][j] = j;
            }
            else if (j == 0) {
                dp[i][j] = i;
            }
            else if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else {
                int deletion = dp[i - 1][j] + 1;
                int insertion = dp[i][j - 1] + 1;
                int substitution = dp[i - 1][j - 1] + 1;
                dp[i][j] = std::min(deletion, std::min(insertion, substitution));
            }
        }
    }
    return dp[m][n];
}

// ==================== 检查字符串相似度 ====================

bool isSimilar(const std::string& input, const std::string& target, int maxDistance) {
    // 转换为小写进行不区分大小写的比较
    std::string lowerInput = input;
    std::string lowerTarget = target;

    std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);

    // 计算编辑距离
    int distance = calculateEditDistance(lowerInput, lowerTarget);

    // 如果编辑距离小于等于阈值，则认为相似
    return distance <= maxDistance;
}

// ==================== 操作处理函数 ====================

int operate() {
    // 外部全局变量声明
    extern CurrentGameInfo g_currentGameInfo;
    extern bool saveGame(const std::string&, size_t, const GameState&, const std::string&);
    extern void Run();

    while (true) {
        std::string op = getKeyName();
        if (op == "ENTER") {
            std::cout << std::endl;
            return 0;
        }
        if (op == "ESC") {
            Log(LogGrade::INFO, "Start to print menu");
            cout << std::endl;
            // 游戏菜单选项
            std::vector<std::string> menu_options = {
                "1. 继续游戏",
                "2. 保存并退出",
                "3. 不保存退出"
            };

            std::string selected = "";
            std::string op2 = "";

            // 检查gum是否可用
            if (gum::GumWrapper::is_available()) {
                try {
                    // 使用gum显示菜单选择
                    Log(LogGrade::INFO, "Using gum for menu selection");
                    selected = gum::GumWrapper::choose(menu_options);

                    if (!selected.empty()) {
                        // 提取第一个字符作为选项数字
                        op2 = selected.substr(0, 1);
                    }
                }
                catch (const std::exception& e) {
                    Log(LogGrade::ERR, "Gum selection error: " + std::string(e.what()));
                    op2 = ""; // 清空，使用回退逻辑
                }
            }

            // 如果gum不可用或失败，回退到原始方法
            if (op2.empty() || !(op2 == "1" || op2 == "2" || op2 == "3")) {
                Log(LogGrade::INFO, "Falling back to original menu display");
                std::cout << std::endl;
                std::cout << "\033[90m";
                std::cout << "-----游戏菜单-----" << std::endl;
                std::cout << "1. 继续游戏" << std::endl;
                std::cout << "2. 保存并退出" << std::endl;
                std::cout << "3. 不保存退出" << std::endl;
                std::cout << "------------------" << std::endl;
                std::cout << "\033[937m";
                Log(LogGrade::INFO, "End to print menu");

                op2 = getKeyName();
            }

            Log(LogGrade::INFO, "Menu selection: " + op2);

            // 处理选择结果
            if (op2 == "1") {
                Log(LogGrade::INFO, "Continue game");
                return 0;
            }
            else if (op2 == "2") {
                // 保存游戏
                Log(LogGrade::INFO, "Start to save game");
                if (g_currentGameInfo.gameState != nullptr &&
                    !g_currentGameInfo.scriptPath.empty()) {
                    if (saveGame(g_currentGameInfo.scriptPath,
                        g_currentGameInfo.currentLine,
                        *g_currentGameInfo.gameState, "autosave")) {
                        Log(LogGrade::INFO, "Game saved");
                        std::cout<<ANSI_GREEN << "游戏已保存" <<"\033[37m" << std::endl;//颜色：绿色
                        Sleep(800);
                    }
                    else {
                        Log(LogGrade::ERR, "Failed to save game");
                        std::cout <<ANSI_RED << "保存失败" <<"\033[37m" << std::endl;//颜色：红色
                        Sleep(800);
                    }
                }
                return 1; // 保存并退出
            }
            else if (op2 == "3") {
                Log(LogGrade::INFO, "Quit game without saving");
                return 1;
            }
            else {
                // 无效输入，默认继续游戏
                Log(LogGrade::WARNING, "Invalid menu selection, default to continue");
                return 0;
            }
        }
        // 添加调试终端功能 - 按下F12键进入调试模式
        if (op == "F12") {
            Log(LogGrade::INFO, "Enter debug mode");
            std::cout << std::endl;
            if (readCfg("DevModeEnabled") == "1")
            {
                Log(LogGrade::INFO, "Debug mode enabled");
                std::cout << "\033[90m键入'help'以获取帮助" << std::endl;
                // 调试终端主循环
                while (1) {
                    std::cout << "\033[90m";
                    std::cout << std::endl << "Debug> ";

                    // 获取用户输入
                    std::string command;
                    std::getline(std::cin, command);
                    Log(LogGrade::DEBUG, "Debug command: " + command);
                    if (command == "exit" || command == "quit") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Exit debug mode");
                        std::cout << "退出调试终端" << std::endl;
                        std::cout << "\033[37m";
                        break;
                    }
                    else if (command == "help") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Print help information");
                        std::cout << "可用命令:" << std::endl;
                        std::cout << "  vars               - 显示所有变量及其值" << std::endl;
                        std::cout << "  set <name> <value> - 设置变量值" << std::endl;
                        std::cout << "  add <name> <value> - 增加变量值" << std::endl;
                        std::cout << "  history            - 显示选择历史" << std::endl;
                        std::cout << "  endings            - 显示结局收集情况" << std::endl;
                        std::cout << "  info               - 显示当前游戏信息" << std::endl;
                        std::cout << "  goto <line>        - 跳转到指定行号" << std::endl;
                        std::cout << "  help               - 显示此帮助信息" << std::endl;
                        std::cout << "  exit/quit          - 退出调试终端" << std::endl;
                        std::cout << "  log <Grade> <Out>  - 输出日志行" << std::endl;
                    }
                    else if (command == "vars") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Print all variables");
                        if (g_currentGameInfo.gameState != nullptr) {
                            auto& gameState = *g_currentGameInfo.gameState;

                            // 显示整数变量
                            std::cout << "整数变量:" << std::endl;
                            auto& intVars = gameState.getAllVariables();

                            if (intVars.empty()) {
                                std::cout << "无整数变量" << std::endl;
                            }
                            else {
                                for (const auto& var : intVars) {
                                    std::cout << var.first << " = " << var.second << std::endl;
                                }
                                std::cout << "总计: " << intVars.size() << " 个整数变量" << std::endl;
                            }

                            std::cout<<endl <<"====================" <<endl<< std::endl;

                            // 显示字符串变量
                            std::cout << "字符串变量:" << std::endl;
                            auto& strVars = gameState.getAllStringVariables();

                            if (strVars.empty()) {
                                std::cout << "无字符串变量" << std::endl;
                            }
                            else {
                                for (const auto& var : strVars) {
                                    std::cout << var.first << " = \"" << var.second << "\"" << std::endl;
                                }
                                std::cout << "总计: " << strVars.size() << " 个字符串变量" << std::endl;
                            }
                        }
                        else {
                            Log(LogGrade::ERR, "Game state not initialized");
                            std::cout << "错误：游戏状态未初始化" << std::endl;
                        }
                    }
                    else if (command.substr(0, 4) == "log ")
                    {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Print log line");
                        // 解析日志命令：log <Grade> <Out>
                        std::stringstream ss(command.substr(4)); // 跳过 "log " 前缀
                        std::string gradeStr, message;

                        // 读取日志等级
                        if (!(ss >> gradeStr)) {
                            std::cout << "用法: log <Grade> <Out>" << std::endl;
                            std::cout << "可用等级: info, warning, error, debug" << std::endl;
                            continue;
                        }

                        // 读取剩余内容作为日志消息
                        std::getline(ss, message);
                        // 去除消息开头的空格
                        if (!message.empty() && message[0] == ' ') {
                            message = message.substr(1);
                        }

                        // 查找日志等级
                        auto it = logGradeMap.find(gradeStr);
                        if (it != logGradeMap.end()) {
                            Log(it->second, message);
                        }
                        else {
                            std::cout << "错误：无效的日志等级 '" << gradeStr << "'" << std::endl;
                            std::cout << "可用等级: info, warning, error, debug" << std::endl;
                        }
                    }

                    else if (command.substr(0, 4) == "set ") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Set variable value");
                        if (g_currentGameInfo.gameState != nullptr) {
                            std::stringstream ss(command.substr(4)); // 跳过 "set " 前缀

                            // 获取变量名
                            std::string varName;
                            if (!(ss >> varName)) {
                                std::cout << "用法: set <变量名> <值>" << std::endl;
                                std::cout << "       set <变量名> \"字符串值\" (设置字符串变量)" << std::endl;
                                continue;
                            }

                            // 读取剩余部分作为值
                            std::string valueStr;
                            std::getline(ss, valueStr);

                            // 去除开头的空格
                            if (!valueStr.empty() && valueStr[0] == ' ') {
                                valueStr = valueStr.substr(1);
                            }

                            if (valueStr.empty()) {
                                std::cout << "错误：值不能为空" << std::endl;
                                std::cout << "用法: set <变量名> <值>" << std::endl;
                                std::cout << "       set <变量名> \"字符串值\" (设置字符串变量)" << std::endl;
                                continue;
                            }

                            // 检查是否是字符串（带引号）
                            bool isString = false;
                            std::string actualValue;

                            if (valueStr[0] == '"' && valueStr.back() == '"') {
                                // 带引号的字符串
                                isString = true;
                                actualValue = valueStr.substr(1, valueStr.length() - 2);
                            }
                            else if (valueStr[0] == '\'' && valueStr.back() == '\'') {
                                // 带单引号的字符串
                                isString = true;
                                actualValue = valueStr.substr(1, valueStr.length() - 2);
                            }
                            else {
                                // 尝试解析为整数
                                try {
                                    int intValue = std::stoi(valueStr);

                                    // 设置整数变量
                                    g_currentGameInfo.gameState->setVar(varName, intValue);
                                    std::cout << "已设置整数变量 " << varName << " = " << intValue << std::endl;
                                    Log(LogGrade::INFO, "Set integer variable: " + varName + " = " + std::to_string(intValue));
                                    continue;
                                }
                                catch (const std::invalid_argument&) {
                                    
                                        isString = true;
                                        actualValue = valueStr;
                                   
                                }
                                catch (const std::out_of_range&) {
                                    std::cout << "错误：数字超出范围" << std::endl;
                                    continue;
                                }
                            }

                            // 设置字符串变量
                            if (isString) {
                                g_currentGameInfo.gameState->setStringVar(varName, actualValue);
                                std::cout << "已设置字符串变量 " << varName << " = \"" << actualValue << "\"" << std::endl;

                                // 显示字符串长度
                                std::cout << "字符串长度: " << actualValue.length() << " 字符" << std::endl;

                                // 如果字符串包含特殊字符，给出提示
                                if (actualValue.find('\n') != std::string::npos) {
                                    std::cout << "提示：字符串包含换行符" << std::endl;
                                }
                                if (actualValue.find('\t') != std::string::npos) {
                                    std::cout << "提示：字符串包含制表符" << std::endl;
                                }

                                Log(LogGrade::INFO, "Set string variable: " + varName + " = \"" + actualValue + "\"");
                            }
                        }
                        else {
                            Log(LogGrade::ERR, "Game state not initialized");
                            std::cout << "错误：游戏状态未初始化" << std::endl;
                        }
                    }
                    else if (command.substr(0, 4) == "add ") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Add value to variable");
                        if (g_currentGameInfo.gameState != nullptr) {
                            std::stringstream ss(command.substr(4));
                            std::string varName;
                            int value;

                            if (ss >> varName >> value) {
                                g_currentGameInfo.gameState->addVar(varName, value);
                                std::cout << "已增加变量 " << varName << " += " << value << std::endl;
                                std::cout << "当前值: " << g_currentGameInfo.gameState->getVar(varName) << std::endl;
                            }
                            else {
                                std::cout << "用法: add <变量名> <值>" << std::endl;
                            }
                        }
                        else {
                            std::cout << "错误：游戏状态未初始化" << std::endl;
                        }
                    }
                    else if (command == "history") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Print choice history");
                        if (g_currentGameInfo.gameState != nullptr) {
                            auto& history = g_currentGameInfo.gameState->getChoiceHistory();
                            std::cout << "选择历史 (" << history.size() << " 项):" << std::endl;
                            std::cout << "------------------------" << std::endl;

                            for (size_t i = 0; i < history.size(); i++) {
                                std::cout << i + 1 << ". " << history[i] << std::endl;
                            }
                        }
                        else {
                            std::cout << "错误：游戏状态未初始化" << std::endl;
                        }
                    }
                    else if (command == "endings") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Print collected endings");
                        if (g_currentGameInfo.gameState != nullptr) {
                            auto& gameState = *g_currentGameInfo.gameState;
                            int collected = gameState.getCollectedEndingsCount();
                            int total = gameState.getTotalEndingsCount();

                            std::cout << "结局收集情况: " << collected << "/" << total << std::endl;
                            std::cout << "----------------" << std::endl;

                            if (collected > 0) {
                                auto& endings = gameState.getCollectedEndings();
                                std::cout << "已收集的结局:" << std::endl;
                                for (size_t i = 0; i < endings.size(); i++) {
                                    std::cout << "  " << i + 1 << ". " << endings[i] << std::endl;
                                }
                            }

                            if (total > 0) {
                                std::cout << "收集进度: " << (collected * 100 / total) << "%" << std::endl;
                            }
                        }
                        else {
                            std::cout << "错误：游戏状态未初始化" << std::endl;
                        }
                    }
                    else if (command == "info") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Print game info");
                        std::cout << "当前游戏信息:" << std::endl;
                        std::cout << "----------------" << std::endl;
                        std::cout << "脚本路径: " << g_currentGameInfo.scriptPath << std::endl;
                        std::cout << "当前行号: " << g_currentGameInfo.currentLine << std::endl;
                        std::cout << "游戏状态: " << (g_currentGameInfo.gameState != nullptr ? "已初始化" : "未初始化") << std::endl;

                        if (g_currentGameInfo.gameState != nullptr) {
                            auto& gameState = *g_currentGameInfo.gameState;
                            std::cout << "已收集结局: " << gameState.getCollectedEndingsCount() << std::endl;
                            std::cout << "选择历史数量: " << gameState.getChoiceHistory().size() << std::endl;
                        }
                    }
                    else if (command.substr(0, 5) == "goto ") {
                        Log(LogGrade::INFO, "DEBUG COMMAND: Go to specific line");
                        std::stringstream ss(command.substr(5));
                        int lineNum;

                        if (ss >> lineNum) {
                            if (lineNum > 0) {
                                // 这是一个特殊的返回值，告诉调用者要跳转到指定行
                                g_currentGameInfo.currentLine = lineNum - 1; // 转换为0-based索引
                                std::cout << "将跳转到第 " << lineNum << " 行" << std::endl;
                                std::cout << "退出调试终端并继续游戏..." << std::endl;
                                cout << std::endl;
                                std::cout << "\033[37m";
                                return 3; // 特殊返回值表示跳转
                            }
                            else {
                                std::cout << "错误：行号必须为正数" << std::endl;
                            }
                        }
                        else {
                            std::cout << "用法: goto <行号>" << std::endl;
                        }
                    }
                    else if (command.empty()) {
                        // 空命令，不做任何操作
                    }
                    else {
                        // 检查是否有相似命令
                        for (const auto& target : validCommands) {
                            if (isSimilar(command, target)) {
                                std::cout << "Do you mean: " << target << " ? " << std::endl;
                                break;
                            }
                        }
                        Log(LogGrade::ERR, "Unknown command: " + command);
                        std::cout << "未知命令: " << command << std::endl;
                        std::cout << "\033[90m";
                        std::cout << "输入 'help' 查看可用命令" << std::endl;
                    }
                }
            }
            else {
                Log(LogGrade::WARNING, "The Debug is not enabled");
                std::cout << "\033[90m" << "调试模式未启用" << "\033[37m" << std::endl;
                
            }
            
            
            

        }
    }
}

/**
 * @brief 检查并清理日志文件
 * @param logFilePath 日志文件路径
 * @return 是否成功清理
 */
bool checkAndCleanLogFile(const std::string& logFilePath) {
    namespace fs = std::filesystem;

    // 检查文件是否存在
    if (!fs::exists(logFilePath)) {
        return true;
    }

    try {
        // 获取文件大小
        auto fileSize = fs::file_size(logFilePath);
        const size_t MAX_LOG_SIZE = 20 * 1024 * 1024; // 20MB

        // 如果文件超过20MB，清空文件
        if (fileSize > MAX_LOG_SIZE) {
            std::ofstream outFile(logFilePath, std::ios::trunc);
            if (!outFile.is_open()) {
                return false;
            }

            // 写入清空说明
            outFile << "[" << logtimer() << "] INFO 日志文件已超过20MB，自动清空" << std::endl;
            outFile.close();

            // 输出控制台提示
            std::cout << "日志文件已超过20MB，已自动清空" << std::endl;
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "检查日志文件时出错: " << e.what() << std::endl;
        return false;
    }
}



/**
 * @brief 写入日志到文件
 */
void Log(LogGrade logGrade, const std::string& out) {

    if (logGrade == LogGrade::DEBUG)
    {
        if (!DebugLogEnabled)
        {
            return;// 如果调试日志未启用，则返回
        }
    }
    // 日志文件路径
    const std::string LOG_FILE_PATH = "pvn_engine.log";

    // 检查并清理日志文件
    if (!checkAndCleanLogFile(LOG_FILE_PATH)) {
        std::cerr << "无法清理日志文件，日志写入可能失败" << std::endl;
    }

    // 格式化日志字符串
    std::stringstream logStream;
	logStream << logtimer() << " "
        << logGradeToString(logGrade) << " "
        << out;

    std::string logMessage = logStream.str();

    

    // 写入到日志文件
    std::ofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "无法打开日志文件: " << LOG_FILE_PATH << std::endl;
        return;
    }

    logFile << logMessage << std::endl;
    logFile.close();
}