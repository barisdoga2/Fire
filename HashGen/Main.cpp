#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")

std::string ToHex(const BYTE* data, DWORD len)
{
    std::string hex;
    hex.reserve(len * 2);

    char buf[3];
    for (DWORD i = 0; i < len; i++)
    {
        sprintf_s(buf, "%02x", data[i]);
        hex.append(buf);
    }

    return hex;
}

std::string ComputeMD5(const std::string& filePath)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE buffer[4096];
    DWORD bytesRead = 0;
    BYTE rgbHash[16];
    DWORD cbHash = 16;

    HANDLE file = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE)
        return "";

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return "";

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
        return "";

    while (ReadFile(file, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
    {
        CryptHashData(hHash, buffer, bytesRead, 0);
    }

    CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);

    CloseHandle(file);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return ToHex(rgbHash, cbHash);
}

int main()
{
    std::string basePath = std::filesystem::current_path().string();
    std::string outputFile = "index.html";

    std::ofstream out(outputFile);
    if (!out.is_open())
    {
        std::cout << "Cannot write index.html\n";
        return 1;
    }

    out << "<!DOCTYPE html>\n<html>\n<body>\n";

    for (auto& entry : std::filesystem::recursive_directory_iterator(basePath))
    {
        if (!entry.is_regular_file())
            continue;

        std::string full = entry.path().string();
        std::string filename = entry.path().filename().string();

        // Skip index.html and hash_gen.exe
        if (filename == "hash_gen.exe" || filename.find(".php") != std::string::npos || filename.find(".html") != std::string::npos)
            continue;

        // Compute relative path
        std::string rel = std::filesystem::relative(entry.path(), basePath).string();

        // Convert Windows slashes to web slashes
        for (char& c : rel)
            if (c == '\\') c = '/';

        // Compute MD5
        std::string md5 = ComputeMD5(full);

        std::cout << rel << " : " << md5 << "\n";
        out << "<p>" << rel << ":" << md5 << "</p>\n";
    }

    out << "</body>\n</html>\n";
    out.close();

    std::cout << "\nindex.html has been created.\n";

    return 0;
}
