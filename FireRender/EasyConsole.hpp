#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "EasyUtils.hpp"



class EasyConsole : public std::streambuf {
private:
    static inline std::streambuf* oldCoutBuf{};
    static inline const auto ttl = std::chrono::seconds(3U);

public:
    struct ConsoleLine
    {
        std::string text;
        std::chrono::steady_clock::time_point time;

        ConsoleLine(std::string text, std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now()) : text(TimeNow_HHMMSS() + " - " + text), time(time)
        {
            
        }
        
        static std::string GetAsConsoleLog(std::string log)
        {
            return TimeNow_HHMMSS() + " - " + log;
        }
    };

private:
    std::mutex mtx;
    std::string currentLine;
    std::vector<ConsoleLine> lines;
    char consoleInput[512U]{};
    bool autoScroll{true};

public:
    EasyConsole(const std::vector<std::string>& lines = {} )
    {
        for(const std::string line : lines)
            AddLines(lines);

        if(!oldCoutBuf)
            oldCoutBuf = std::cout.rdbuf(this);
        else
            std::cout.rdbuf(this);
    }

    ~EasyConsole()
    {
        if(oldCoutBuf)
            std::cout.rdbuf(oldCoutBuf);
    }

    void AddLine(const std::string& line, std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now())
    {
        if (line.length() > 0U)
        {
            auto& ref = lines.emplace_back(line, time);
            oldCoutBuf->sputn(ref.text.data(), ref.text.length());
            oldCoutBuf->sputc('\n');
        }
    }

    void AddLines(const std::vector<std::string>& linesIn)
    {
        for (const std::string& line : linesIn)
            AddLine(line);
    }

    void PruneOldLogs()
    {
        const auto now = std::chrono::steady_clock::now();
        while (!lines.empty())
        {
            if (now - lines.front().time > ttl)
                lines.erase(lines.begin());
            else
                break;
        }
    }

    bool AutoScroll() const
    {
        return autoScroll;
    }
        
    const std::vector<ConsoleLine>& Lines() const
    {
        return lines;
    }

protected:
    int overflow(int c) override
    {
        if (c == EOF)
            return EOF;

        std::lock_guard<std::mutex> lock(mtx);
        if (c == '\n')
        {
            if (!currentLine.empty())
            {
                AddLine(currentLine);
                currentLine.clear();
            }
        }
        else
        {
            currentLine.push_back(static_cast<char>(c));
        }
        return c;
    }

    std::streamsize xsputn(const char* s, std::streamsize count) override
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (std::streamsize i = 0; i < count; ++i)
        {
            char c = s[i];
            if (c == '\n')
            {
                AddLine(currentLine);
                currentLine.clear();
            }
            else
            {
                currentLine.push_back(c);
            }
        }
        return count;
    }
};
