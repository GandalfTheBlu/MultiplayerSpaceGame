#pragma once
#include <string>
#include <unordered_map>
#include <functional>

typedef std::function<void(const std::string&)> ConsoleCommand;

class Console
{
private:
	const char* label;
	char* buffer;
	size_t buffSize;
	std::unordered_map<std::string, ConsoleCommand> commands;

public:
	Console(const char* _label, size_t inputBufferSize);
	~Console();

	void SetCommand(const std::string& commandName, ConsoleCommand commandFunction);
	void Draw();

private:
	void ClearBuffer();
	void ReadCommand();
};