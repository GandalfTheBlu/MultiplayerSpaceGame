#include "config.h"
#include "console.h"
#include "imgui.h"

namespace Game
{
Console::Console(const char* _windowLabel, size_t _inputBufferSize, size_t _outputLineSize, size_t _outputLineCount)
{
	windowLabel = _windowLabel;
	inputBufferSize = _inputBufferSize;
	inputBuffer = new char[inputBufferSize+1];// + 1 for \0-character

	outputLineSize = _outputLineSize;
	outputLineCount = _outputLineCount;
	outputBuffer = new char[outputLineCount * outputLineSize + 1];// + 1 for \0-character

	ClearInputBuffer();
	ClearOutputBuffer();
}

Console::~Console()
{
	delete[] inputBuffer;
	delete[] outputBuffer;
}

void Console::SetCommand(const std::string& commandName, ConsoleCommand commandFunction)
{
	commands[commandName] = commandFunction;
}

void Console::AddOutput(const std::string& output)
{
	for (size_t i = 1; i < outputLineCount; i++)
	{
		for (size_t j = 0; j+1 < outputLineSize; j++)
		{
			// shift each line up
			size_t prevLineIndex = (i - 1) * outputLineSize + j;
			size_t currLineIndex = i * outputLineSize + j;
			outputBuffer[prevLineIndex] = outputBuffer[currLineIndex];

			// add new output to last line
			if (i + 1 == outputLineCount)
			{
				outputBuffer[currLineIndex] = (j < output.size() ? output[j] : ' ');
			}
		}
	}
}

void Console::Draw()
{
	ImGui::Begin(windowLabel);

	ImGui::Text(outputBuffer);
	
	if (ImGui::InputText("input", inputBuffer, inputBufferSize, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		ReadCommand();
	}
	if (ImGui::Button("enter command"))
	{
		ReadCommand();
	}

	ImGui::End();
}

void Console::ClearInputBuffer()
{
	for (size_t i = 0; i < inputBufferSize+1; i++)
		inputBuffer[i] = '\0';
}

void Console::ClearOutputBuffer()
{
	for (size_t i = 0; i < outputLineCount; i++)
	{
		for (size_t j = 0; j < outputLineSize; j++)
		{
			size_t index = i * outputLineSize + j;
			outputBuffer[index] = (j + 1 < outputLineSize ? ' ' : '\n');
		}
	}

	outputBuffer[outputLineCount * outputLineSize] = '\0';
}

void Console::ReadCommand()
{
	bool hasName = false;
	std::string commandName;
	std::string commandArg;

	for (size_t i = 0; i < inputBufferSize && inputBuffer[i] != '\0'; i++)
	{
		char c = inputBuffer[i];
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

	ClearInputBuffer();
}
}