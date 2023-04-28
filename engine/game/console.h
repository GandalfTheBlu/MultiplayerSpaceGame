#pragma once
#include <string>
#include <unordered_map>
#include <functional>

namespace Game
{
typedef std::function<void(const std::string&)> ConsoleCommand;

class Console
{
private:
	const char* windowLabel;
	char* inputBuffer;
	size_t inputBufferSize;
	std::unordered_map<std::string, ConsoleCommand> commands;

	char* outputBuffer;
	size_t outputLineSize;
	size_t outputLineCount;

public:
	Console(const char* _windowLabel, size_t _inputBufferSize, size_t _outputLineSize, size_t _outputLineCount);
	~Console();

	void SetCommand(const std::string& commandName, ConsoleCommand commandFunction);
	void AddOutput(const std::string& output);
	void Draw();

private:
	void ClearInputBuffer();
	void ClearOutputBuffer();
	void ReadCommand();
};
}