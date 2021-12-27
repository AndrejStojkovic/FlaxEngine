// Copyright (c) 2012-2019 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacFileSystem.h"
#include "Engine/Platform/File.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

const DateTime UnixEpoch(1970, 1, 1);

bool MacFileSystem::CreateDirectory(const StringView& path)
{
    const StringAsANSI<> pathAnsi(*path, path.Length());

    // Skip if already exists
    struct stat fileInfo;
    if (stat(pathAnsi.Get(), &fileInfo) != -1 && S_ISDIR(fileInfo.st_mode))
    {
        return false;
    }

    // Recursively do it all again for the parent directory, if any
    const int32 slashIndex = path.FindLast('/');
    if (slashIndex > 1)
    {
        if (CreateDirectory(path.Substring(0, slashIndex)))
        {
            return true;
        }
    }

    // Create the last directory on the path (the recursive calls will have taken care of the parent directories by now)
    return mkdir(pathAnsi.Get(), 0755) != 0 && errno != EEXIST;
}

bool DeletePathTree(const char* path)
{
    size_t pathLength;
    DIR* dir;
    struct stat statPath, statEntry;
    struct dirent* entry;

    // Stat for the path
    stat(path, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char full_path[256];
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(full_path));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // Stat for the entry
        stat(full_path, &statEntry);

        // Recursively remove a nested directory
        if (S_ISDIR(statEntry.st_mode) != 0)
        {
            if (DeletePathTree(full_path))
                return true;
            continue;
        }

        // Remove a file object
        if (unlink(full_path) != 0)
            return true;
    }

    // Remove the devastated directory and close the object of it
    if (rmdir(path) != 0)
        return true;

    closedir(dir);

    return false;
}

bool MacFileSystem::DeleteDirectory(const String& path, bool deleteContents)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (deleteContents)
        return DeletePathTree(pathANSI.Get());
    return rmdir(pathANSI.Get()) != 0;
}

bool MacFileSystem::DirectoryExists(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISDIR(fileInfo.st_mode);
    }
    return false;
}

bool MacFileSystem::DirectoryGetFiles(Array<String>& results, const String& path, const Char* searchPattern, DirectorySearchOption option)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    const StringAsANSI<> searchPatternANSI(searchPattern);

    // Check if use only top directory
    if (option == DirectorySearchOption::TopDirectoryOnly)
        return getFilesFromDirectoryTop(results, pathANSI.Get(), searchPatternANSI.Get());
    return getFilesFromDirectoryAll(results, pathANSI.Get(), searchPatternANSI.Get());
}

bool MacFileSystem::GetChildDirectories(Array<String>& results, const String& directory)
{
    size_t pathLength;
    DIR* dir;
    struct stat statPath, statEntry;
    struct dirent* entry;
    const StringAsANSI<> pathANSI(*directory, directory.Length());
    const char* path = pathANSI.Get();

    // Stat for the path
    stat(path, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char full_path[256];
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(full_path));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // Stat for the entry
        stat(full_path, &statEntry);

        // Check for directory
        if (S_ISDIR(statEntry.st_mode) != 0)
        {
            // Add directory
            results.Add(String(full_path));
        }
    }

    closedir(dir);

    return false;
}

bool MacFileSystem::FileExists(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        return S_ISREG(fileInfo.st_mode);
    }
    return false;
}

bool MacFileSystem::DeleteFile(const StringView& path)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    return unlink(pathANSI.Get()) == 0;
}

uint64 MacFileSystem::GetFileSize(const StringView& path)
{
    struct stat fileInfo;
    fileInfo.st_size = -1;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        // Check for directories
        if (S_ISDIR(fileInfo.st_mode))
        {
            fileInfo.st_size = -1;
        }
    }
    return fileInfo.st_size;
}

bool MacFileSystem::IsReadOnly(const StringView& path)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (access(pathANSI.Get(), W_OK) == -1)
    {
        return errno == EACCES;
    }
    return false;
}

bool MacFileSystem::SetReadOnly(const StringView& path, bool isReadOnly)
{
    const StringAsANSI<> pathANSI(*path, path.Length());
    struct stat fileInfo;
    if (stat(pathANSI.Get(), &fileInfo) != -1)
    {
        if (isReadOnly)
        {
            fileInfo.st_mode &= ~S_IWUSR;
        }
        else
        {
            fileInfo.st_mode |= S_IWUSR;
        }
        return chmod(pathANSI.Get(), fileInfo.st_mode) == 0;
    }
    return false;
}

bool MacFileSystem::MoveFile(const StringView& dst, const StringView& src, bool overwrite)
{
    if (!overwrite && FileExists(dst))
    {
        // Already exists
        return true;
    }

    if (overwrite)
    {
        unlink(StringAsANSI<>(*dst, dst.Length()).Get());
    }
    if (rename(StringAsANSI<>(*src, src.Length()).Get(), StringAsANSI<>(*dst, dst.Length()).Get()) != 0)
    {
        if (errno == EXDEV)
        {
            if (!CopyFile(dst, src))
            {
                unlink(StringAsANSI<>(*src, src.Length()).Get());
                return false;
            }
        }
        return true;
    }
    return false;
}

bool MacFileSystem::CopyFile(const StringView& dst, const StringView& src)
{
    const StringAsANSI<> srcANSI(*src, src.Length());
    const StringAsANSI<> dstANSI(*dst, dst.Length());

    int srcFile, dstFile;
    char buffer[4096];
    ssize_t readSize;
    int cachedError;

    srcFile = open(srcANSI.Get(), O_RDONLY);
    if (srcFile < 0)
        return true;
    dstFile = open(dstANSI.Get(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (dstFile < 0)
        goto out_error;

    // first try the kernel method
    struct stat statBuf;
    fstat(srcFile, &statBuf);
    readSize = 1;
    while (readSize > 0)
    {
        readSize = sendfile(dstFile, srcFile, 0, statBuf.st_size);
    }
    // sendfile could fail for example if the input file is not nmap'able
    // in this case we fall back to the read/write loop
    if (readSize < 0)
    {
        while (readSize = read(srcFile, buffer, sizeof(buffer)), readSize > 0)
        {
            char* ptr = buffer;
            ssize_t writeSize;

            do
            {
                writeSize = write(dstFile, ptr, readSize);
                if (writeSize >= 0)
                {
                    readSize -= writeSize;
                    ptr += writeSize;
                }
                else if (errno != EINTR)
                {
                    goto out_error;
                }
            } while (readSize > 0);
        }
    }

    if (readSize == 0)
    {
        if (close(dstFile) < 0)
        {
            dstFile = -1;
            goto out_error;
        }
        close(srcFile);

        // Success
        return false;
    }

out_error:
    cachedError = errno;
    close(srcFile);
    if (dstFile >= 0)
        close(dstFile);
    errno = cachedError;
    return true;
}

bool MacFileSystem::getFilesFromDirectoryTop(Array<String>& results, const char* path, const char* searchPattern)
{
    size_t pathLength;
    struct stat statPath, statEntry;
    struct dirent* entry;

    // Stat for the path
    stat(path, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    DIR* dir = opendir(path);
    if (dir == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char fullPath[256];
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(fullPath));
        strcpy(fullPath, path);
        strcat(fullPath, "/");
        strcat(fullPath, entry->d_name);

        // Stat for the entry
        stat(fullPath, &statEntry);

        // Check for file
        if (S_ISREG(statEntry.st_mode) != 0)
        {
            // Validate with filter
            const int32 fullPathLength = StringUtils::Length(fullPath);
            const int32 searchPatternLength = StringUtils::Length(searchPattern);
            if (searchPatternLength == 0 || StringUtils::Compare(searchPattern, "*") == 0)
            {
                // All files
            }
            else if (searchPattern[0] == '*' && searchPatternLength < fullPathLength && StringUtils::Compare(fullPath + fullPathLength - searchPatternLength + 1, searchPattern + 1, searchPatternLength - 1) == 0)
            {
                // Path ending
            }
            else
            {
                // TODO: implement all cases in a generic way
                continue;
            }

            // Add file
            results.Add(String(fullPath));
        }
    }

    closedir(dir);

    return false;
}

bool MacFileSystem::getFilesFromDirectoryAll(Array<String>& results, const char* path, const char* searchPattern)
{
    // Find all files in this directory
    getFilesFromDirectoryTop(results, path, searchPattern);

    size_t pathLength;
    DIR* dir;
    struct stat statPath, statEntry;
    struct dirent* entry;

    // Stat for the path
    stat(path, &statPath);

    // If path does not exists or is not dir - exit with status -1
    if (S_ISDIR(statPath.st_mode) == 0)
    {
        // Is not directory
        return true;
    }

    // If not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL)
    {
        // Cannot open directory
        return true;
    }

    // The length of the path
    pathLength = strlen(path);

    // Iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determinate a full path of an entry
        char full_path[256];
        ASSERT(pathLength + strlen(entry->d_name) < ARRAY_COUNT(full_path));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // Stat for the entry
        stat(full_path, &statEntry);

        // Check for directory
        if (S_ISDIR(statEntry.st_mode) != 0)
        {
            if (getFilesFromDirectoryAll(results, full_path, searchPattern))
            {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);

    return false;
}

DateTime MacFileSystem::GetFileLastEditTime(const StringView& path)
{
    struct stat fileInfo;
    const StringAsANSI<> pathANSI(*path, path.Length());
    if (stat(pathANSI.Get(), &fileInfo) == -1)
    {
        return DateTime::MinValue();
    }

    const TimeSpan timeSinceEpoch(0, 0, fileInfo.st_mtime);
    return UnixEpoch + timeSinceEpoch;
}

void MacFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    MISSING_CODE("MacFileSystem::GetSpecialFolderPath");
    return; // TODO: filesystem on Mac
}

#endif