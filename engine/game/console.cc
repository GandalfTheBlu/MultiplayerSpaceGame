#include "config.h"
#include "console.h"
#include "imgui.h"

Console::Console(const char* _label, size_t inputBufferSize)
{
	label = _label;
	buffSize = inputBufferSize;
	buffer = new char[buffSize];

	ClearBuffer();
}

Console::~Console()
{
	delete[] buffer;
}

void Console::SetCommand(const std::string& commandName, ConsoleCommand commandFunction)
{
	commands[commandName] = commandFunction;
}

void Console::Draw()
{
    ImGui::Begin(label);

    ImGui::InputText("input", buffer, buffSize);
    if (ImGui::Button("enter command"))
    {
		ReadCommand();
    }

    ImGui::End();
}

void Console::ClearBuffer()
{
	for (size_t i = 0; i < buffSize; i++)
		buffer[i] = '\0';
}

void Console::ReadCommand()
{
	bool hasName = false;
	std::string commandName;
	std::string commandArg;

	for (size_t i = 0; i < buffSize && buffer[i] != '\0'; i++)
	{
		char c = buffer[i];
		if (!hasName && c == ' ')
		{
			hasName = true;
		}
		else if (!hasName)
		{
			commandName += c;
		}
		else
		{
			commandArg += c;
		}
	}

	if (commands.count(commandName) == 0)
		printf("\n[WARNING] invalid command '%s'.\n", commandName.c_str());
	else
		commands[commandName](commandArg);

	ClearBuffer();
}