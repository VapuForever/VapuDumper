#include <io.h>
#include <stdio.h>
#include <windows.h>

BOOL running = TRUE;

#define VAPU_DLL "Vapu-Native.dll"

#define OUTPUT "dumped/"

void prepareDir(char* path) {
    int len = strlen(path);
    int last = 0;
    for(int i=0;i<len;i++) {
        char c = *(path+i);
        if(c == '/')
            c = *(path+i) = '\\';
        if(c == '\\')
            last = i;
    }

    char* cmd = calloc(last + 22, 1);
    memcpy(cmd, "mkdir ", 6);
    memcpy(cmd + 6, path, last);
    strcat(cmd, " 1> nul 2> nul");
    system(cmd);
}

DWORD WINAPI LoadThread(LPVOID parameter) {
    if(LoadLibraryA(VAPU_DLL) == NULL) {
        running = FALSE;
        MessageBoxA(NULL, "Vapu-Native.dll not found!", "Error", MB_ICONHAND);
    }
    return 0;
}

int main() {
    HANDLE hThread = CreateThread(NULL, 4096, &LoadThread, NULL, 0, NULL);
    if(hThread == NULL) {
        printf("Create thread failed! GLE is %d\n", GetLastError());
    }
    Sleep(1000);
    HANDLE hModule = GetModuleHandle(VAPU_DLL);
    printf("Handle: 0x%p\n", hModule);
    if(!hModule) {
        ExitProcess(-1);
    }
    char* magic = calloc(5, 1);
    size_t offset = 0;
    while(ReadProcessMemory(GetCurrentProcess(), hModule + offset, magic, 4, NULL)) {
        if(strcmp(magic, "\xCA\xFE\xBA\xBE") == 0) {
            printf("Offset is %d\nAddress is 0x%p\n", offset, hModule + offset);
            break;
        } else {
            offset++;
        }
    }
    free(magic);
    offset -= 128;

    int entries;
    if(!ReadProcessMemory(GetCurrentProcess(), hModule + offset, &entries, 4, NULL)) {
        printf("Read process memory failed! GLE is %d\n", GetLastError());
        goto exit;
    }
    printf("%d entries.\n", entries);
    offset += 4;
    char* name = calloc(120, 1);
    int length;
    char* path = calloc(MAX_PATH, 1);
    int len = strlen(OUTPUT);
    memcpy(path, OUTPUT, len);

    for(int i=0;i<entries;i++) {
        if(!ReadProcessMemory(GetCurrentProcess(), hModule + offset, name, 120, NULL)) {
            printf("Read process memory failed! GLE is %d\n", GetLastError());
            free(name);
            goto exit;
        }
        offset += 120;
        if(!ReadProcessMemory(GetCurrentProcess(), hModule + offset, &length, 4, NULL)) {
            printf("Read process memory failed! GLE is %d\n", GetLastError());
            free(name);
            goto exit;
        }
        offset += 4;
        printf("File: %s size: %d \n", name, length);
        memcpy(path + len, name, 120);
        strcat(path, ".class");
        prepareDir(path);
        char* data = calloc(length, 1);
        if(!ReadProcessMemory(GetCurrentProcess(), hModule + offset, data, length, NULL)) {
            printf("Read process memory failed! GLE is %d\n", GetLastError());
            free(name);
            free(data);
            goto exit;
        }
        offset += length;
        printf("Output to %s\n", path);
        FILE* fp = fopen(path, "wb");
        if(fp == NULL) {
            printf("Open file failed! GLE is %d\n", GetLastError());
            free(name);
            free(data);
            goto exit;
        }
        if(fwrite(data, length, 1, fp) == 0) {
            printf("Open file failed! GLE is %d\n", GetLastError());
            free(name);
            free(data);
            goto exit;
        }
        fclose(fp);
        free(data);
    }
    free(name);
    printf("Done!");
    exit:
    ExitProcess(0);
}