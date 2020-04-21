#pragma once

#include "smallvec.h"
#include <vector>
#include <algorithm>

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
    HANDLE handle = FindFirstFileA(dir.c_str(), &fileData);
    if (handle == INVALID_HANDLE_VALUE)
    {
        FATAL_ERROR("Failed to read directory: ", dir);
    }
    do
    {
        if (fileData.cFileName[0] != '.')
        {
            if (fileData & FILE_ATTRIBUTE_DIRECTORY)
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
                if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
                {
                    files.push_back({ dp->d_name, true });
                    std::string childDirectory = dir + '/' + dp->d_name;
                    if (recursive)
                    {
                        files.back().children = readDirectory(childDirectory, recursive);
                    }
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

std::string chooseFile(const char* defaultSelection, bool open, std::string const& fileType,
        SmallVec<const char*> extensions)
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
    if (open)
    {
        cmd += " --title 'Open File' --file-selection --filename ";
        cmd += defaultSelection;
        std::string fileFilter = fileType + " | ";
        for (auto& ext : extensions)
        {
            fileFilter += ext;
            fileFilter += ' ';
        }
        cmd += " --file-filter \"" + fileFilter + '"';
    }
    else
    {
        cmd += " --title 'Save File' --file-selection --save --confirm-overwrite --filename ";
        cmd += defaultSelection;
    }
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
