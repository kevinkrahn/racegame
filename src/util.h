#pragma once

#include "misc.h"
#include "buffer.h"

#if _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <dirent.h>
#endif

struct FileItem
{
    const char* path;
    bool isDirectory;
    Array<FileItem> children;
};

Array<FileItem> readDirectory(const char* dir, bool recursive=true)
{
    Array<FileItem> files;

#if _WIN32
    WIN32_FIND_DATA fileData;
    HANDLE handle = FindFirstFile(tmpStr("%s\\*", dir), &fileData);
    if (handle == INVALID_HANDLE_VALUE)
    {
        FATAL_ERROR("Failed to read directory: ", dir);
    }
    do
    {
        if (fileData.cFileName[0] != '.')
        {
            if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                files.push({ tmpStr("%s", fileData.cFileName), true });
                const char* childDirectory = tmpStr("%s/%s", dir, fileData.cFileName);
                if (recursive)
                {
                    files.back().children = readDirectory(childDirectory, recursive);
                }
            }
            else
            {
                files.push({ tmpStr("%s", fileData.cFileName), false });
            }
        }
    }
    while (FindNextFileA(handle, &fileData) != 0);
    FindClose(handle);
#else
    DIR* dirp = opendir(dir);
    if (!dirp)
    {
        FATAL_ERROR("Failed to read directory: %s", dir);
    }
    dirent* dp;
    while ((dp = readdir(dirp)))
    {
        if (dp->d_name[0] != '.')
        {
            const char* name = tmpStr("%s", dp->d_name);
            if (dp->d_type == DT_DIR)
            {
				files.push({ name, true });
				const char* childDirectory = tmpStr("%s/%s", dir, dp->d_name);
				if (recursive)
				{
					files.back().children = readDirectory(childDirectory, recursive);
				}
            }
            else if (dp->d_type == DT_REG)
            {
                files.push({ name, false });
            }
        }
    }
    closedir(dirp);
#endif

    files.sort([](auto& a, auto& b) {
        if (a.isDirectory && !b.isDirectory) return true;
        if (!a.isDirectory && b.isDirectory) return false;
        return strcmp(a.path, b.path) < 0;
    });
    return files;
}

const char* chooseFile(bool open, const char* fileType,
        SmallArray<const char*> extensions, const char* defaultDir)
{
#if _WIN32
    char szFile[260];

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    // TODO: Implement file filter
    ofn.lpstrFilter = "All\0*.*\0Tracks\0*.dat\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    // TODO: set this
    ofn.lpstrInitialDir = NULL;
    if (open)
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    }

    if (open)
    {
        if (GetOpenFileName(&ofn) == TRUE)
        {
            return tmpStr("%s", szFile);
        }
        else
        {
            return {};
        }
    }
    else
    {
        if (GetSaveFileName(&ofn) == TRUE)
        {
            return tmpStr("%s", szFile);
        }
        else
        {
            return {};
        }
    }
#else
    StrBuf buf;
    buf.writef("zenity --file-filter \"%s |", fileType);
    for (auto& ext : extensions)
    {
        buf.write(" ");
        buf.write(ext);
    }
    buf.write("\"");
    if (open)
    {
        buf.write(" --title 'Open File' --file-selection");
    }
    else
    {
        buf.write(" --title 'Save File' --file-selection --save --confirm-overwrite");
    }
    if (defaultDir)
    {
        buf.write(" --filename ");
        buf.write(defaultDir);
    }
    println("%s", buf.data());
    FILE *f = popen(buf.data(), "r");
    char* filename = g_tmpMem.bump<char>(1024);
    memset(filename, 0, 1024);
    if (!f || !fgets(filename, 1024 - 1, f))
    {
        error("Unable to create file dialog");
        return {};
    }
    pclose(f);
    if (filename[0] != 0)
    {
        // strip newline
        auto len = strlen(filename)-1;
        if (filename[len] == '\n')
        {
            filename[len] = '\0';
        }
        println("File chosen: %s", filename);
        return filename;
    }
    return nullptr;
#endif
}

struct CommandResult
{
    i32 exitCode;
    const char* output;
};

CommandResult runShellCommand(const char* command)
{
    println("%s", command);

#if _WIN32
    SECURITY_ATTRIBUTES attr;
    attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    attr.bInheritHandle = TRUE;
    attr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdoutRead, hChildStdoutWrite;
    if (!CreatePipe(&hChildStdoutRead, &hChildStdoutWrite, &attr, 0))
    {
        return { -1000 };
    }

    HANDLE hChildStdinRead, hChildStdinWrite;
    if (!CreatePipe(&hChildStdinRead, &hChildStdinWrite, &attr, 0))
    {
        return { -1002 };
    }

    PROCESS_INFORMATION procInfo = {};

    STARTUPINFO startInfo = {};
    startInfo.cb = sizeof(startInfo);
    startInfo.hStdError = hChildStdoutWrite;
    startInfo.hStdOutput = hChildStdoutWrite;
    startInfo.hStdInput = hChildStdinRead;
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, (LPSTR)command, NULL, NULL, TRUE, 0,
                NULL, NULL, &startInfo, &procInfo))
    {
        return { -1004 };
    }

    char buf[4096];
    DWORD read = 0;
    ReadFile(hChildStdoutRead, buf, sizeof(buf), &read, NULL);

    DWORD exitCode;
    GetExitCodeProcess(procInfo.hProcess, &exitCode);
    CommandResult result;
    result.exitCode = exitCode;
    result.output = buf;

    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);
    CloseHandle(hChildStdoutRead);
    CloseHandle(hChildStdoutWrite);
    CloseHandle(hChildStdinRead);
    CloseHandle(hChildStdinWrite);

    return result;
#else
    FILE* stream = popen(tmpStr("%s 2>&1", command), "r");
    if (!stream)
    {
        return { -1, "" };
    }

    char* commandOutput = g_tmpMem.get<char*>();
    char* writePos = commandOutput;
    u32 totalSize = 0;
    while (fgets(writePos, 1024, stream))
    {
        totalSize += 1024;
        writePos = commandOutput + totalSize;
    }
    i32 code = pclose(stream);

    return { code, commandOutput };
#endif
}

Buffer readFileBytes(const char* filename)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "r+b");
    if (!file)
    {
        FATAL_ERROR("File ", filename, " does not exist.");
    }
    size_t size = SDL_RWsize(file);
    Buffer buffer(size);
    SDL_RWread(file, buffer.data.get(), size, 1);
    SDL_RWclose(file);
    return buffer;
}

StrBuf readFileString(const char* filename)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "r+b");
    if (!file)
    {
        FATAL_ERROR("File ", filename, " does not exist.");
    }
    size_t size = SDL_RWsize(file);
    StrBuf buffer(size);
    SDL_RWread(file, buffer.data(), size, 1);
    SDL_RWclose(file);
    return buffer;
}

void writeFile(const char* filename, void* data, size_t len)
{
    SDL_RWops* file = SDL_RWFromFile(filename, "w+b");
    if (!file)
    {
        FATAL_ERROR("Failed to open file for writing: ", filename);
    }
    if (SDL_RWwrite(file, data, 1, len) != len)
    {
        FATAL_ERROR("Failed to complete file write: ", filename);
    }
    SDL_RWclose(file);
}
