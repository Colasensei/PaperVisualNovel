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

    // 保持原有的ANSI颜色代码（但不在Windows中使用）
    namespace ansi {
        constexpr const char* RESET = "";
        constexpr const char* BLACK = "";
        constexpr const char* RED = "";
        constexpr const char* GREEN = "";
        constexpr const char* YELLOW = "";
        constexpr const char* BLUE = "";
        constexpr const char* MAGENTA = "";
        constexpr const char* CYAN = "";
        constexpr const char* WHITE = "";
        constexpr const char* BG_BLACK = "";
        constexpr const char* BG_RED = "";
        constexpr const char* BG_GREEN = "";
        constexpr const char* BG_YELLOW = "";
        constexpr const char* BG_BLUE = "";
        constexpr const char* BG_MAGENTA = "";
        constexpr const char* BG_CYAN = "";
        constexpr const char* BG_WHITE = "";
        constexpr const char* BOLD = "";
        constexpr const char* DIM = "";
        constexpr const char* ITALIC = "";
        constexpr const char* UNDERLINE = "";
        constexpr const char* BLINK = "";
        constexpr const char* REVERSE = "";
        constexpr const char* HIDDEN = "";
    }

    // 基础Gum交互类 - 完全兼容原有接口
    class GumWrapper {
    private:
        static std::string execute_gum_command(const std::string& command) {
#ifdef _WIN32
            // 保存原始控制台编码
            UINT original_cp = GetConsoleOutputCP();

            // 临时切换到UTF-8以便与gum交互
            SetConsoleOutputCP(CP_UTF8);
#endif

            FILE* pipe = nullptr;

#ifdef _WIN32
            pipe = _popen(command.c_str(), "r");
#else
            pipe = popen(command.c_str(), "r");
#endif

            if (!pipe) {
#ifdef _WIN32
                SetConsoleOutputCP(original_cp);
#endif
                throw std::runtime_error("无法执行gum命令: " + command);
            }

            std::array<char, 128> buffer;
            std::string result;

            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }

#ifdef _WIN32
            _pclose(pipe);
            // 恢复原始编码
            SetConsoleOutputCP(original_cp);
#else
            pclose(pipe);
#endif

            // 移除末尾换行符
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }

            return result;
        }

        // 转义字符串中的特殊字符
        static std::string escape_string(const std::string& str) {
            std::string escaped;
            for (char c : str) {
                if (c == '"' || c == '\\' || c == '$' || c == '`') {
                    escaped += '\\';
                }
                escaped += c;
            }
            return escaped;
        }

    public:
        // 检查gum是否可用 - 保持原有接口
        static bool is_available() {
            try {
                std::string version = execute_gum_command("gum --version");
                return !version.empty();
            }
            catch (...) {
                return false;
            }
        }

        // 基础选择函数 - 保持原有接口
        static std::string choose(const std::vector<std::string>& options,
            const std::string& prompt = "") {
            if (options.empty()) {
                return "";
            }

            std::stringstream cmd;
            cmd << "gum choose";

            if (!prompt.empty()) {
                cmd << " \"" << escape_string(prompt) << "\"";
            }

            for (const auto& opt : options) {
                cmd << " \"" << escape_string(opt) << "\"";
            }

            return execute_gum_command(cmd.str());
        }

        // 带标题和限制的选择 - 保持原有接口
        static std::vector<std::string> choose_multiple(
            const std::vector<std::string>& options,
            const std::string& header = "",
            int limit = 0,
            int height = 0) {

            std::vector<std::string> selected;

            if (options.empty()) {
                return selected;
            }

            std::stringstream cmd;
            cmd << "gum choose";

            if (!header.empty()) {
                cmd << " --header=\"" << escape_string(header) << "\"";
            }

            if (limit > 0) {
                cmd << " --limit=" << limit;
            }

            if (height > 0) {
                cmd << " --height=" << height;
            }

            cmd << " --no-limit";

            for (const auto& opt : options) {
                cmd << " \"" << escape_string(opt) << "\"";
            }

            std::string result = execute_gum_command(cmd.str());

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

        // 过滤选择 - 保持原有接口
        static std::string filter(const std::vector<std::string>& options,
            const std::string& placeholder = "过滤...",
            const std::string& header = "") {

            std::stringstream cmd;
            cmd << "gum filter";

            if (!placeholder.empty()) {
                cmd << " --placeholder=\"" << escape_string(placeholder) << "\"";
            }

            if (!header.empty()) {
                cmd << " --header=\"" << escape_string(header) << "\"";
            }

            // 准备输入数据
            std::string input_data;
            for (const auto& opt : options) {
                input_data += opt + "\n";
            }

            // 执行命令
            std::string full_cmd = cmd.str();

#ifdef _WIN32
            // 保存原始控制台编码
            UINT original_cp = GetConsoleOutputCP();
            SetConsoleOutputCP(CP_UTF8);

            // 使用管道传递输入
            FILE* pipe = _popen(full_cmd.c_str(), "w");
#else
            FILE* pipe = popen(full_cmd.c_str(), "w");
#endif

            if (!pipe) {
#ifdef _WIN32
                SetConsoleOutputCP(original_cp);
#endif
                throw std::runtime_error("无法执行gum filter命令");
            }

            fwrite(input_data.c_str(), 1, input_data.size(), pipe);

#ifdef _WIN32
            _pclose(pipe);
            SetConsoleOutputCP(original_cp);
            // 重新获取结果
            std::string result = execute_gum_command(cmd.str() + " << EOF\n" + input_data + "EOF");
#else
            pclose(pipe);
            std::string result = execute_gum_command("echo \"" + input_data + "\" | " + cmd.str());
#endif

            return result;
        }

        // 输入对话框 - 保持原有接口
        static std::string input(const std::string& prompt = "",
            const std::string& placeholder = "",
            const std::string& value = "",
            int width = 0) {

            std::stringstream cmd;
            cmd << "gum input";

            if (!prompt.empty()) {
                cmd << " --prompt=\"" << escape_string(prompt) << "\"";
            }

            if (!placeholder.empty()) {
                cmd << " --placeholder=\"" << escape_string(placeholder) << "\"";
            }

            if (!value.empty()) {
                cmd << " --value=\"" << escape_string(value) << "\"";
            }

            if (width > 0) {
                cmd << " --width=" << width;
            }

            return execute_gum_command(cmd.str());
        }

        // 确认对话框 - 正确检查退出码
        static bool confirm(const std::string& prompt = "确认?",
            bool default_yes = false) {

            std::stringstream cmd;
            cmd << "gum confirm";

            if (!prompt.empty()) {
                cmd << " \"" << escape_string(prompt) << "\"";
            }

            if (default_yes) {
                cmd << " --default=true";
            }
            else {
                cmd << " --default=false";
            }

            return system(cmd.str().c_str());
        }

        // 进度条 - 保持原有接口
        static void progress_bar(const std::string& title = "",
            int duration = 5,
            const std::string& cmd = "") {

            std::stringstream gum_cmd;
            gum_cmd << "gum spin";

            if (!title.empty()) {
                gum_cmd << " --title=\"" << escape_string(title) << "\"";
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

    // 流畅接口构建器 - 保持原有接口
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
            // 临时切换编码
            UINT original_cp = GetConsoleOutputCP();
            SetConsoleOutputCP(CP_UTF8);

            pipe = _popen(command_.str().c_str(), "r");
#else
            pipe = popen(command_.str().c_str(), "r");
#endif

            if (!pipe) {
#ifdef _WIN32
                SetConsoleOutputCP(original_cp);
#endif
                throw std::runtime_error("无法执行gum命令");
            }

            std::array<char, 128> buffer;
            std::string result;

            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }

#ifdef _WIN32
            _pclose(pipe);
            SetConsoleOutputCP(original_cp);
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
            return (result == "true");
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