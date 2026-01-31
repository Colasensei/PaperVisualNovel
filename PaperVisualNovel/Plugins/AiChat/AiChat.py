# coding=gb2312
import sys
import time
import requests
import json
import os
from typing import List, Dict, Any, Optional
from datetime import datetime

class GB2312Chatbot:
    def __init__(self, api_key: str, system_prompt: str = "", ai_name: str = "AI", log_file: Optional[str] = None):
        """
        初始化聊天机器人
        
        Args:
            api_key: OpenAI API密钥
            system_prompt: 系统提示词
            ai_name: AI显示名称
            log_file: 日志文件路径
        """
        self.api_key = api_key
        self.api_url = "https://api.deepseek.com/v1/chat/completions"
        self.ai_name = ai_name
        self.log_file = log_file
        self.conversation_history: List[Dict[str, str]] = []
        
        # 添加系统提示词（如果存在）
        if system_prompt:
            self.conversation_history.append({
                "role": "system",
                "content": system_prompt
            })
    
    def log_message(self, message: str, level: str = "INFO"):
        """
        记录日志信息
        
        Args:
            message: 日志消息
            level: 日志级别 (INFO, ERROR, DEBUG)
        """
        if not self.log_file:
            return
            
        try:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            log_entry = f"[{timestamp}] [{level}] {message}\n"
            
            # 确保日志目录存在
            log_dir = os.path.dirname(self.log_file)
            if log_dir and not os.path.exists(log_dir):
                os.makedirs(log_dir)
            
            # 使用GB2312编码写入日志
            with open(self.log_file, 'a', encoding='gb2312', errors='replace') as f:
                f.write(log_entry)
                
        except Exception as e:
            # 如果日志写入失败，输出到控制台
            print(f"[LOG ERROR] Failed to write log: {str(e)}")
    
    def filter_gb2312(self, text: str) -> str:
        """
        过滤文本，只保留GB2312字符集支持的内容
        
        Args:
            text: 原始文本
            
        Returns:
            过滤后的文本
        """
        result = []
        for char in text:
            try:
                # 尝试用GB2312编码，如果失败则跳过该字符
                char.encode('gb2312')
                result.append(char)
            except UnicodeEncodeError:
                # 替换不支持的字符为空格
                result.append(' ')
        return ''.join(result)
    
    def typewriter_print(self, text: str, delay: float = 0.03):
        """
        打字机效果输出文本
        
        Args:
            text: 要输出的文本
            delay: 字符间的延迟时间（秒）
        """
        for char in text:
            sys.stdout.write(char)
            sys.stdout.flush()
            time.sleep(delay)
        print()
    
    def chat(self, prompt: str) -> str:
        """
        发送聊天请求并获取响应
        
        Args:
            prompt: 用户输入
            
        Returns:
            AI的回复
        """
        # 记录用户输入
        self.log_message(f"User input: {prompt}")
        
        # 添加用户消息到历史
        self.conversation_history.append({
            "role": "user",
            "content": prompt
        })
        
        # 准备请求头
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {self.api_key}"
        }
        
        # 准备请求数据
        data = {
            "model": "deepseek-chat",
            "messages": self.conversation_history,
            "temperature": 0.7,
            "max_tokens": 2000
        }
        
        try:
            # 记录API请求开始
            self.log_message(f"Sending request to {self.api_url}")
            
            # 发送请求
            response = requests.post(
                self.api_url,
                headers=headers,
                data=json.dumps(data),
                timeout=60
            )
            
            # 记录响应状态
            self.log_message(f"Response status: {response.status_code}")
            
            if response.status_code == 200:
                result = response.json()
                ai_response = result["choices"][0]["message"]["content"]
                
                # 过滤为GB2312字符集
                ai_response_filtered = self.filter_gb2312(ai_response)
                
                # 记录AI响应
                self.log_message(f"AI response: {ai_response_filtered}")
                
                # 添加AI回复到历史
                self.conversation_history.append({
                    "role": "assistant",
                    "content": ai_response_filtered
                })
                
                return ai_response_filtered
            else:
                error_msg = f"API request failed, status: {response.status_code}, response: {response.text}"
                self.log_message(error_msg, "ERROR")
                return f"API请求失败，状态码: {response.status_code}"
                
        except Exception as e:
            error_msg = f"Request error: {str(e)}"
            self.log_message(error_msg, "ERROR")
            return f"请求出错: {str(e)}"
    
    def interactive_chat(self, max_turns: int = -1):
        """
        交互式聊天模式
        
        Args:
            max_turns: 最大对话轮次，负数表示无限直到输入/exit
        """
        turn_count = 0
        
        # 记录会话开始
        self.log_message("=" * 50)
        self.log_message("Chat session started")
        self.log_message(f"AI Name: {self.ai_name}")
        self.log_message(f"Max turns: {max_turns if max_turns > 0 else 'unlimited'}")
        self.log_message("=" * 50)
        
        print("=" * 50)
        print("输入 /exit 退出程序")
        print("输入 /clear 清空对话历史")
        print("=" * 50)
        
        while True:
            # 检查是否达到最大轮次（仅当max_turns为正数时）
            if max_turns > 0 and turn_count >= max_turns:
                end_msg = f"Reached maximum dialogue turns ({max_turns}), exiting."
                print(f"\n{end_msg}")
                self.log_message(end_msg)
                break
            
            try:
                # 获取用户输入
                user_input = input(f"\n[{turn_count + 1}] 你: ")
                
                # 检查退出命令
                if user_input.strip().lower() == "/exit":
                    exit_msg = "User requested exit."
                    print("退出程序...")
                    self.log_message(exit_msg)
                    break
                
                # 检查清空历史命令
                if user_input.strip().lower() == "/clear":
                    # 保留系统提示词（如果有）
                    system_messages = [msg for msg in self.conversation_history 
                                      if msg["role"] == "system"]
                    self.conversation_history = system_messages
                    clear_msg = "Dialogue history cleared."
                    print("对话历史已清空！")
                    self.log_message(clear_msg)
                    turn_count = 0
                    continue
                
                # 检查日志文件位置命令
                if user_input.strip().lower() == "/log":
                    if self.log_file:
                        log_msg = f"Log file location: {self.log_file}"
                        print(log_msg)
                        self.log_message(log_msg)
                    else:
                        no_log_msg = "Log file not configured."
                        print(no_log_msg)
                        self.log_message(no_log_msg)
                    continue
                
                # 发送请求并获取响应
                print(f"\n[{turn_count + 1}] {self.ai_name}: ", end="")
                response = self.chat(user_input)
                
                # 打字机效果输出
                self.typewriter_print(response)
                
                turn_count += 1
                
            except KeyboardInterrupt:
                interrupt_msg = "Program interrupted by user."
                print("\n\n程序被用户中断，退出...")
                self.log_message(interrupt_msg)
                break
            except Exception as e:
                error_msg = f"Unexpected error: {str(e)}"
                print(f"\n发生错误: {str(e)}")
                self.log_message(error_msg, "ERROR")

def read_prompt_file(filepath: str) -> str:
    """
    读取提示词文件（GB2312编码）
    
    Args:
        filepath: 文件路径
        
    Returns:
        文件内容
    """
    try:
        with open(filepath, 'r', encoding='gb2312') as f:
            return f.read().strip()
    except Exception as e:
        error_msg = f"Failed to read prompt file: {str(e)}"
        print(error_msg)
        return ""

def main():
    """
    主函数，处理命令行参数
    """
    # 检查参数数量
    if len(sys.argv) != 6:
        print("用法: python chatbot.py <次数> <提示词文件路径> <API密钥> <AI名称> <日志文件路径>")
        print("  次数: 负数 - 无限循环直到输入/exit, 0 - 使用提示词文件作为输入, 正数 - 对话次数")
        print("  提示词文件路径: GB2312编码的文本文件")
        print("  API密钥: DeepSeek API密钥")
        print("  AI名称: AI显示名称 (例如: '助手', '机器人'等)")
        print("  日志文件路径: GB2312编码的日志文件路径 (留空或设为 'none' 禁用日志)")
        sys.exit(1)
    
    try:
        # 解析参数
        turns = int(sys.argv[1])
        prompt_file = sys.argv[2]
        api_key = sys.argv[3]
        ai_name = sys.argv[4]
        log_file = sys.argv[5]
        
        # 处理日志文件参数
        if log_file.lower() in ["", "none", "null"]:
            log_file = None
            print("日志功能已禁用")
        else:
            print(f"日志将保存到: {log_file}")
        
        # 读取提示词文件
        system_prompt = read_prompt_file(prompt_file)
        
        # 创建聊天机器人实例
        chatbot = GB2312Chatbot(api_key, system_prompt, ai_name, log_file)
        
        # 记录程序启动
        chatbot.log_message("Program started")
        chatbot.log_message(f"Turns parameter: {turns}")
        chatbot.log_message(f"Prompt file: {prompt_file}")
        chatbot.log_message(f"AI name: {ai_name}")
        chatbot.log_message(f"Log file: {log_file if log_file else 'disabled'}")
        
        # 根据turns参数执行不同模式
        if turns == 0:
            # 模式0：使用提示词文件作为输入
            if not system_prompt:
                error_msg = "Prompt file is empty, cannot execute mode 0"
                print(error_msg)
                chatbot.log_message(error_msg, "ERROR")
                return
                
            print("正在处理提示词文件内容...")
            print(f"{ai_name}回复: ", end="")
            response = chatbot.chat(system_prompt)
            chatbot.typewriter_print(response)
            
        elif turns > 0:
            # 模式正数：限制对话次数
            print(f"将进行 {turns} 轮对话")
            chatbot.interactive_chat(max_turns=turns)
            
        else:
            # 模式负数：无限对话直到输入/exit
            chatbot.interactive_chat(max_turns=-1)
            
        # 记录程序结束
        chatbot.log_message("Program ended normally")
        
    except ValueError as e:
        error_msg = f"Parameter error: {str(e)}"
        print(error_msg)
        if 'chatbot' in locals():
            chatbot.log_message(error_msg, "ERROR")
        sys.exit(1)
    except Exception as e:
        error_msg = f"Program execution error: {str(e)}"
        print(error_msg)
        if 'chatbot' in locals():
            chatbot.log_message(error_msg, "ERROR")
        sys.exit(1)

if __name__ == "__main__":
    main()