#pragma once

#include "smallvec.h"
#include <vector>
#include <algorithm>
#include <filesystem>

struct FileItem
{
    std::string path;
    bool isDirectory;
    std::vector<FileItem> children;
};

std::vector<FileItem> readDirectory(std::string const& dir, bool recursive=true)
{
    std::vector<FileItem> files;

#if _WIN32
    WIN32_FIND_DATA fileData;
    HANDLE handle = FindFirstFile((dir + "\\*").c_str(), &fileData);
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
                files.push_back({ fileData.cFileName, true });
                std::string childDirectory = dir + '/' + fileData.cFileName;
                if (recursive)
                {
                    files.back().children = readDirectory(childDirectory, recursive);
                }
            }
            else
            {
                files.push_back({ fileData.cFileName, false });
            }
        }
    }
    while (FindNextFileA(handle, &fileData) != 0);
    FindClose(handle);
#else
    DIR* dirp = opendir(dir.c_str());
    if (!dirp)
    {
        FATAL_ERROR("Failed to read directory: ", dir);
    }
    dirent* dp;
    while ((dp = readdir(dirp)))
    {
        if (dp->d_name[0] != '.')
        {
            if (dp->d_type == DT_DIR)
            {
				files.push_back({ dp->d_name, true });
				std::string childDirectory = dir + '/' + dp->d_name;
				if (recursive)
				{
					files.back().children = readDirectory(childDirectory, recursive);
				}
            }
            else if (dp->d_type == DT_REG)
            {
                files.push_back({ dp->d_name, false });
            }
        }
    }
    closedir(dirp);
#endif

    std::sort(files.begin(), files.end(), [](FileItem const& a, FileItem const& b) {
        if (a.isDirectory && !b.isDirectory) return true;
        if (!a.isDirectory && b.isDirectory) return false;
        return a.path < b.path;
    });
    return files;
}

std::string chooseFile(bool open, std::string const& fileType,
        SmallVec<const char*> extensions, std::string const& defaultDir)
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
            return std::string(szFile);
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
            return std::string(szFile);
        }
        else
        {
            return {};
        }
    }
#else
    char filename[1024] = { 0 };
    std::string cmd = "zenity";
    std::string fileFilter = fileType + " | ";
    for (auto& ext : extensions)
    {
        fileFilter += ext;
        fileFilter += ' ';
    }
    cmd += " --file-filter \"" + fileFilter + '"';
    if (open)
    {
        cmd += " --title 'Open File' --file-selection --filename ";
    }
    else
    {
        cmd += " --title 'Save File' --file-selection --save --confirm-overwrite --filename ";
    }
    cmd += std::filesystem::absolute(defaultDir).string();
    print(cmd, '\n');
    FILE *f = popen(cmd.c_str(), "r");
    if (!f || !fgets(filename, sizeof(filename) - 1, f))
    {
        error("Unable to create file dialog\n");
        return {};
    }
    pclose(f);
    std::string file(filename);
    if (!file.empty())
    {
        file.pop_back();
    }

    return file;
#endif
}

struct CommandResult
{
    i32 exitCode;
    std::string output;
};

CommandResult runShellCommand(std::string const& command)
{
    print(command, '\n');

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

    if (!CreateProcess(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, 0,
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
    std::string cmd = command + " 2>&1";

    FILE* stream = popen(cmd.c_str(), "r");
    if (!stream)
    {
        return { -1, "" };
    }

    std::string output;
    char* line = NULL;
    size_t memSize = 0;
    ssize_t r;
    while ((r = getline(&line, &memSize, stream)) != -1)
    {
        output += std::string(line, r);
    }
    free(line);

    i32 code = pclose(stream);

    return { code, output };
#endif
}
