#ifndef GUM_WRAPPER_HPP
#define GUM_WRAPPER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include <stdexcept>
#include <sstream>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace gum {

    // ANSI颜色代码
    namespace ansi {
        constexpr const char* RESET = "\033[0m";
        constexpr const char* BLACK = "\033[30m";
        constexpr const char* RED = "\033[31m";
        constexpr const char* GREEN = "\033[32m";
        constexpr const char* YELLOW = "\033[33m";
        constexpr const char* BLUE = "\033[34m";
        constexpr const char* MAGENTA = "\033[35m";
        constexpr const char* CYAN = "\033[36m";
        constexpr const char* WHITE = "\033[37m";
        constexpr const char* BG_BLACK = "\033[40m";
        constexpr const char* BG_RED = "\033[41m";
        constexpr const char* BG_GREEN = "\033[42m";
        constexpr const char* BG_YELLOW = "\033[43m";
        constexpr const char* BG_BLUE = "\033[44m";
        constexpr const char* BG_MAGENTA = "\033[45m";
        constexpr const char* BG_CYAN = "\033[46m";
        constexpr const char* BG_WHITE = "\033[47m";
        constexpr const char* BOLD = "\033[1m";
        constexpr const char* DIM = "\033[2m";
        constexpr const char* ITALIC = "\033[3m";
        constexpr const char* UNDERLINE = "\033[4m";
        constexpr const char* BLINK = "\033[5m";
        constexpr const char* REVERSE = "\033[7m";
        constexpr const char* HIDDEN = "\033[8m";
    }

    // 基础Gum交互类
    class GumWrapper {
    private:
        static bool enable_ansi_windows() {
#ifdef _WIN32
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE) return false;

            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode)) return false;

            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            return SetConsoleMode(hOut, dwMode);
#else
            return true;
#endif
        }

        static std::string execute_gum_command(const std::string& command) {
            // 启用Windows ANSI支持
            static bool ansi_enabled = enable_ansi_windows();

#ifdef _WIN32
            FILE* pipe = _popen(command.c_str(), "r");
#else
            FILE* pipe = popen(command.c_str(), "r");
#endif

            if (!pipe) {
                throw std::runtime_error("无法执行gum命令: " + command);
            }

            std::array<char, 128> buffer;
            std::string result;

            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }

#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif

            // 移除末尾换行符
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }

            return result;
        }

    public:
        // 检查gum是否可用
        static bool is_available() {
            try {
                std::string version = execute_gum_command("gum --version");
                return !version.empty() && version.find("gum") != std::string::npos;
            }
            catch (...) {
                return false;
            }
        }

        // 基础选择函数
        static std::string choose(const std::vector<std::string>& options,
            const std::string& prompt = "") {
            if (options.empty()) {
                return "";
            }

            std::stringstream cmd;
            cmd << "gum choose";

            if (!prompt.empty()) {
                cmd << " \"" << prompt << "\"";
            }

            for (const auto& opt : options) {
                cmd << " \"" << opt << "\"";
            }

            return execute_gum_command(cmd.str());
        }

        // 带标题和限制的选择
        static std::vector<std::string> choose_multiple(
            const std::vector<std::string>& options,
            const std::string& header = "",
            int limit = 0,
            int height = 0) {

            std::stringstream cmd;
            cmd << "gum choose";

            if (!header.empty()) {
                cmd << " --header=\"" << header << "\"";
            }

            if (limit > 0) {
                cmd << " --limit=" << limit;
            }

            if (height > 0) {
                cmd << " --height=" << height;
            }

            cmd << " --no-limit";

            for (const auto& opt : options) {
                cmd << " \"" << opt << "\"";
            }

            std::string result = execute_gum_command(cmd.str());
            std::vector<std::string> selected;

            if (!result.empty()) {
                std::stringstream ss(result);
                std::string item;
                while (std::getline(ss, item, '\n')) {
                    if (!item.empty()) {
                        selected.push_back(item);
                    }
                }
            }

            return selected;
        }

        // 过滤选择
        static std::string filter(const std::vector<std::string>& options,
            const std::string& placeholder = "过滤...",
            const std::string& header = "") {

            std::stringstream cmd;
            cmd << "gum filter";

            if (!placeholder.empty()) {
                cmd << " --placeholder=\"" << placeholder << "\"";
            }

            if (!header.empty()) {
                cmd << " --header=\"" << header << "\"";
            }

            std::string input;
            for (const auto& opt : options) {
                input += opt + "\n";
            }

#ifdef _WIN32
            FILE* pipe = _popen(cmd.str().c_str(), "w");
#else
            FILE* pipe = popen(cmd.str().c_str(), "w");
#endif

            if (!pipe) {
                throw std::runtime_error("无法执行gum filter命令");
            }

            fwrite(input.c_str(), 1, input.size(), pipe);

#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif

            // 重新执行获取结果
            std::string result = execute_gum_command(cmd.str() + " <<< \"" + input + "\"");
            return result;
        }

        // 输入对话框
        static std::string input(const std::string& prompt = "",
            const std::string& placeholder = "",
            const std::string& value = "",
            int width = 0) {

            std::stringstream cmd;
            cmd << "gum input";

            if (!prompt.empty()) {
                cmd << " --prompt=\"" << prompt << "\"";
            }

            if (!placeholder.empty()) {
                cmd << " --placeholder=\"" << placeholder << "\"";
            }

            if (!value.empty()) {
                cmd << " --value=\"" << value << "\"";
            }

            if (width > 0) {
                cmd << " --width=" << width;
            }

            return execute_gum_command(cmd.str());
        }

        // 确认对话框
        static bool confirm(const std::string& prompt = "确认?",
            bool default_yes = false) {

            std::stringstream cmd;
            cmd << "gum confirm";

            if (!prompt.empty()) {
                cmd << " \"" << prompt << "\"";
            }

            if (default_yes) {
                cmd << " --default=true";
            }

            std::string result = execute_gum_command(cmd.str());
            return (result == "true" || result == "1");
        }

        // 带样式的文本输出
        static void print_styled(const std::string& text,
            const std::vector<std::string>& styles = {},
            bool newline = true) {

            std::stringstream output;

            for (const auto& style : styles) {
                output << style;
            }

            output << text << ansi::RESET;

            if (newline) {
                output << std::endl;
            }

            std::cout << output.str();
            std::cout.flush();
        }

        // 进度条
        static void progress_bar(const std::string& title = "",
            int duration = 5,
            const std::string& cmd = "") {

            std::stringstream gum_cmd;
            gum_cmd << "gum spin";

            if (!title.empty()) {
                gum_cmd << " --title=\"" << title << "\"";
            }

            if (!cmd.empty()) {
                gum_cmd << " -- " << cmd;
            }
            else {
                gum_cmd << " -- sleep " << duration;
            }

            execute_gum_command(gum_cmd.str());
        }
    };

    // 流畅接口构建器
    class GumBuilder {
    private:
        std::stringstream command_;
        std::vector<std::string> options_;

    public:
        GumBuilder() {
            command_ << "gum";
        }

        // 命令类型
        GumBuilder& choose() {
            command_ << " choose";
            return *this;
        }

        GumBuilder& filter() {
            command_ << " filter";
            return *this;
        }

        GumBuilder& input() {
            command_ << " input";
            return *this;
        }

        GumBuilder& confirm() {
            command_ << " confirm";
            return *this;
        }

        // 选项配置
        GumBuilder& header(const std::string& header) {
            command_ << " --header=\"" << header << "\"";
            return *this;
        }

        GumBuilder& placeholder(const std::string& placeholder) {
            command_ << " --placeholder=\"" << placeholder << "\"";
            return *this;
        }

        GumBuilder& prompt(const std::string& prompt) {
            command_ << " --prompt=\"" << prompt << "\"";
            return *this;
        }

        GumBuilder& height(int height) {
            command_ << " --height=" << height;
            return *this;
        }

        GumBuilder& limit(int limit) {
            command_ << " --limit=" << limit;
            return *this;
        }

        GumBuilder& width(int width) {
            command_ << " --width=" << width;
            return *this;
        }

        // 添加选项
        GumBuilder& add_option(const std::string& option) {
            options_.push_back(option);
            return *this;
        }

        GumBuilder& add_options(const std::vector<std::string>& options) {
            options_.insert(options_.end(), options.begin(), options.end());
            return *this;
        }

        // 执行命令
        std::string execute() {
            for (const auto& opt : options_) {
                command_ << " \"" << opt << "\"";
            }

            FILE* pipe = nullptr;

#ifdef _WIN32
            pipe = _popen(command_.str().c_str(), "r");
#else
            pipe = popen(command_.str().c_str(), "r");
#endif

            if (!pipe) {
                throw std::runtime_error("无法执行gum命令");
            }

            std::array<char, 128> buffer;
            std::string result;

            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }

#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif

            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }

            return result;
        }

        // 执行并返回bool（用于confirm）
        bool execute_bool() {
            std::string result = execute();
            return (result == "true" || result == "1");
        }

        // 执行并返回多选结果
        std::vector<std::string> execute_multiple() {
            std::string result = execute();
            std::vector<std::string> selected;

            if (!result.empty()) {
                std::stringstream ss(result);
                std::string item;
                while (std::getline(ss, item, '\n')) {
                    if (!item.empty()) {
                        selected.push_back(item);
                    }
                }
            }

            return selected;
        }
    };

} // namespace gum

#endif // GUM_WRAPPER_HPP