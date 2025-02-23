#ifndef OFFSET_FETCHER_H
#define OFFSET_FETCHER_H

#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <sstream>

#pragma comment(lib, "wininet.lib")

class OffsetFetcher {
public:
    static std::string FetchURLContent(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("OffsetFetcher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            return "";
        }

        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "";
        }

        char buffer[4096];
        DWORD bytesRead;
        std::string content;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            content += buffer;
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return content;
    }

    static std::string GetLineAt(const std::string& content, int lineNumber) {
        std::istringstream stream(content);
        std::string line;
        int currentLine = 1;

        while (std::getline(stream, line)) {
            if (currentLine == lineNumber) {

                size_t commentPos = line.find("//");
                if (commentPos != std::string::npos) {
                    line = line.substr(0, commentPos);
                }

                size_t firstNonSpace = line.find_first_not_of(" \t");
                if (firstNonSpace != std::string::npos) {
                    line = line.substr(firstNonSpace);
                }
                return line;
            }
            currentLine++;
        }

        return "";
    }

    static uintptr_t GetOffsetFromOffsets(int lineNumber) {
        const std::string urlOffsetsHpp = "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/offsets.hpp";
        std::string content = FetchURLContent(urlOffsetsHpp);
        if (content.empty()) {
           
        }
        std::string line = GetLineAt(content, lineNumber);
        return ParseOffsetFromLine(line);
    }

    static uintptr_t GetOffsetFromClient(int lineNumber) {
        const std::string urlClientDllHpp = "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/client_dll.hpp";
        std::string content = FetchURLContent(urlClientDllHpp);
        if (content.empty()) {
           
        }
        std::string line = GetLineAt(content, lineNumber);
        return ParseOffsetFromLine(line);
    }

private:
    static uintptr_t ParseOffsetFromLine(const std::string& line) {
        size_t offsetPos = line.find("0x");
        if (offsetPos != std::string::npos) {
            return std::stoul(line.substr(offsetPos), nullptr, 16);
        }
       
    }
};

#endif 