#pragma once

#define FPS (1000.0 / 144.0)
#define UPS (1000.0 / 24.0)

#include <iostream>
#include <fstream>
#include <string>
#include <ostream>

inline void SaveConfig(bool remember, const std::string& user, const std::string& passHash)
{
    std::ofstream f("config.ini", std::ios::trunc);
    f << "remember=" << (remember ? "1" : "0") << "\n";
    f << "username=" << user << "\n";
    f << "password=" << passHash << "\n";
}

inline void LoadConfig(bool& remember, char(&user)[16], char(&pass)[32])
{
    std::ifstream f("config.ini");
    if (!f.is_open())
        return;

    std::string line;
    while (std::getline(f, line))
    {
        if (line.rfind("remember=", 0) == 0)
            remember = (line.substr(9) == "1");
        else if (line.rfind("username=", 0) == 0)
            strcpy_s(user, sizeof(user), line.substr(9).c_str());
        else if (line.rfind("password=", 0) == 0)
            strcpy_s(pass, sizeof(pass), line.substr(9).c_str());
    }
}
