#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 65536

// 1. TOPIC: STRUCTURES
typedef struct {
    char name[50];
    char id[20];
    int familySize;
    float monthlyIncome;
    int isServed; // 0 = No, 1 = Yes
} Citizen;

typedef struct {
    int riceStock;     // in kg
    int wheatStock;    // in kg
    int fertilizerPackets;
} Inventory;

const char* DATA_FILE = "citizens.txt";
const char* INV_FILE = "inventory.txt";

// Initialize Inventory
void initializeInventory() {
    FILE *file = fopen(INV_FILE, "r");
    if (file == NULL) {
        file = fopen(INV_FILE, "wb");
        if (file) {
            Inventory defaultInv = {500, 300, 100};
            fwrite(&defaultInv, sizeof(Inventory), 1, file);
            fclose(file);
        }
    } else {
        fclose(file);
    }
}

// URL-decode helper
void urlDecode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Simple JSON extraction helper for string fields
int getJsonStringField(const char *json, const char *key, char *out, int maxLen) {
    char searchPattern[128];
    snprintf(searchPattern, sizeof(searchPattern), "\"%s\"", key);
    char *pos = strstr(json, searchPattern);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    while (*pos == ':' || *pos == ' ' || *pos == '\t' || *pos == '\"') {
        if (*pos == '\"') { pos++; break; }
        pos++;
    }
    int i = 0;
    while (*pos && *pos != '\"' && i < maxLen - 1) {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return 1;
}

// Simple JSON extraction helper for number fields
int getJsonIntField(const char *json, const char *key, int *out) {
    char searchPattern[128];
    snprintf(searchPattern, sizeof(searchPattern), "\"%s\"", key);
    char *pos = strstr(json, searchPattern);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    pos++;
    *out = atoi(pos);
    return 1;
}

int getJsonFloatField(const char *json, const char *key, float *out) {
    char searchPattern[128];
    snprintf(searchPattern, sizeof(searchPattern), "\"%s\"", key);
    char *pos = strstr(json, searchPattern);
    if (!pos) return 0;
    pos = strchr(pos, ':');
    if (!pos) return 0;
    pos++;
    *out = (float)atof(pos);
    return 1;
}

// Business Logics
void handleRegister(const char *body, char *responseBuffer) {
    Citizen c;
    memset(&c, 0, sizeof(Citizen));

    char name[100] = {0};
    char id[50] = {0};
    int familySize = 0;
    float monthlyIncome = 0.0f;

    getJsonStringField(body, "name", name, sizeof(name));
    getJsonStringField(body, "id", id, sizeof(id));
    getJsonIntField(body, "familySize", &familySize);
    getJsonFloatField(body, "monthlyIncome", &monthlyIncome);

    if (strlen(name) == 0 || strlen(id) == 0 || familySize <= 0) {
        sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Invalid citizen data provided.\"}");
        return;
    }

    // Check if ID already exists
    FILE *checkFile = fopen(DATA_FILE, "rb");
    if (checkFile) {
        Citizen temp;
        while (fread(&temp, sizeof(Citizen), 1, checkFile)) {
            if (strcmp(temp.id, id) == 0) {
                fclose(checkFile);
                sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Citizen ID %s already registered!\"}", id);
                return;
            }
        }
        fclose(checkFile);
    }

    strncpy(c.name, name, sizeof(c.name) - 1);
    strncpy(c.id, id, sizeof(c.id) - 1);
    c.familySize = familySize;
    c.monthlyIncome = monthlyIncome;
    c.isServed = 0;

    FILE *file = fopen(DATA_FILE, "ab");
    if (file == NULL) {
        sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Could not open data store.\"}");
        return;
    }

    fwrite(&c, sizeof(Citizen), 1, file);
    fclose(file);

    sprintf(responseBuffer, "{\"status\":\"success\",\"message\":\"Citizen '%s' registered successfully!\"}", c.name);
}

void handleDistribute(const char *body, char *responseBuffer) {
    char searchId[50] = {0};
    getJsonStringField(body, "id", searchId, sizeof(searchId));

    if (strlen(searchId) == 0) {
        sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Please enter a valid Citizen ID.\"}");
        return;
    }

    Citizen c;
    Inventory inv;
    FILE *cFile = fopen(DATA_FILE, "rb+");
    FILE *iFile = fopen(INV_FILE, "rb+");
    int found = 0;

    if (cFile == NULL || iFile == NULL) {
        if (cFile) fclose(cFile);
        if (iFile) fclose(iFile);
        sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Data records or inventory missing.\"}");
        return;
    }

    if (fread(&inv, sizeof(Inventory), 1, iFile) != 1) {
        fclose(cFile);
        fclose(iFile);
        sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Failed to read inventory.\"}");
        return;
    }

    while (fread(&c, sizeof(Citizen), 1, cFile)) {
        if (strcmp(c.id, searchId) == 0) {
            found = 1;

            if (c.isServed == 1) {
                sprintf(responseBuffer, "{\"status\":\"denied\",\"message\":\"Family '%s' has already received their relief quota for this month.\",\"name\":\"%s\",\"id\":\"%s\",\"income\":%.2f,\"familySize\":%d}",
                        c.name, c.name, c.id, c.monthlyIncome, c.familySize);
                break;
            }

            if (c.monthlyIncome <= 15000.0f) {
                int requiredRice = c.familySize * 5;
                int requiredWheat = c.familySize * 3;

                if (inv.riceStock >= requiredRice && inv.wheatStock >= requiredWheat) {
                    inv.riceStock -= requiredRice;
                    inv.wheatStock -= requiredWheat;
                    c.isServed = 1;

                    fseek(cFile, -(long)sizeof(Citizen), SEEK_CUR);
                    fwrite(&c, sizeof(Citizen), 1, cFile);

                    fseek(iFile, 0, SEEK_SET);
                    fwrite(&inv, sizeof(Inventory), 1, iFile);

                    sprintf(responseBuffer, "{\"status\":\"success\",\"message\":\"Approved & Disbursed!\",\"name\":\"%s\",\"id\":\"%s\",\"income\":%.2f,\"familySize\":%d,\"disbursedRice\":%d,\"disbursedWheat\":%d,\"remainingRice\":%d,\"remainingWheat\":%d}",
                            c.name, c.id, c.monthlyIncome, c.familySize, requiredRice, requiredWheat, inv.riceStock, inv.wheatStock);
                } else {
                    sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Insufficient stock to satisfy full family quota (Needed: %d kg Rice, %d kg Wheat).\",\"riceStock\":%d,\"wheatStock\":%d}",
                            requiredRice, requiredWheat, inv.riceStock, inv.wheatStock);
                }
            } else {
                sprintf(responseBuffer, "{\"status\":\"denied\",\"message\":\"Income (%.2f INR) exceeds BPL threshold (15,000 INR). Relief denied.\",\"name\":\"%s\",\"id\":\"%s\",\"income\":%.2f}",
                        c.monthlyIncome, c.name, c.id, c.monthlyIncome);
            }
            break;
        }
    }

    if (!found) {
        sprintf(responseBuffer, "{\"status\":\"not_found\",\"message\":\"No record exists for ID: %s\"}", searchId);
    }

    fclose(cFile);
    fclose(iFile);
}

void handleViewInventory(char *responseBuffer) {
    Inventory inv = {0, 0, 0};
    FILE *file = fopen(INV_FILE, "rb");
    if (file) {
        fread(&inv, sizeof(Inventory), 1, file);
        fclose(file);
    }
    sprintf(responseBuffer, "{\"status\":\"success\",\"riceStock\":%d,\"wheatStock\":%d,\"fertilizerPackets\":%d}",
            inv.riceStock, inv.wheatStock, inv.fertilizerPackets);
}

void handleUpdateInventory(const char *body, char *responseBuffer) {
    Inventory inv = {0, 0, 0};
    FILE *file = fopen(INV_FILE, "rb+");
    if (!file) {
        sprintf(responseBuffer, "{\"status\":\"error\",\"message\":\"Unable to open inventory file.\"}");
        return;
    }

    fread(&inv, sizeof(Inventory), 1, file);

    int addRice = 0, addWheat = 0, addFert = 0;
    getJsonIntField(body, "rice", &addRice);
    getJsonIntField(body, "wheat", &addWheat);
    getJsonIntField(body, "fertilizer", &addFert);

    inv.riceStock += addRice;
    inv.wheatStock += addWheat;
    inv.fertilizerPackets += addFert;

    fseek(file, 0, SEEK_SET);
    fwrite(&inv, sizeof(Inventory), 1, file);
    fclose(file);

    sprintf(responseBuffer, "{\"status\":\"success\",\"message\":\"Inventory updated successfully!\",\"riceStock\":%d,\"wheatStock\":%d,\"fertilizerPackets\":%d}",
            inv.riceStock, inv.wheatStock, inv.fertilizerPackets);
}

void handleGenerateReport(char *responseBuffer, int maxLen) {
    Citizen c;
    FILE *file = fopen(DATA_FILE, "rb");
    int totalFamilies = 0, servedFamilies = 0;

    int offset = snprintf(responseBuffer, maxLen, "{\"status\":\"success\",\"citizens\":[");

    if (file) {
        int first = 1;
        while (fread(&c, sizeof(Citizen), 1, file)) {
            totalFamilies++;
            if (c.isServed) servedFamilies++;

            if (!first) {
                offset += snprintf(responseBuffer + offset, maxLen - offset, ",");
            }
            first = 0;

            offset += snprintf(responseBuffer + offset, maxLen - offset,
                "{\"name\":\"%s\",\"id\":\"%s\",\"familySize\":%d,\"monthlyIncome\":%.2f,\"isServed\":%d}",
                c.name, c.id, c.familySize, c.monthlyIncome, c.isServed);
        }
        fclose(file);
    }

    snprintf(responseBuffer + offset, maxLen - offset,
        "],\"totalFamilies\":%d,\"servedFamilies\":%d,\"pendingFamilies\":%d}",
        totalFamilies, servedFamilies, totalFamilies - servedFamilies);
}

// Static File Server
void serveStaticFile(SOCKET clientSocket, const char *filePath, const char *contentType) {
    FILE *f = fopen(filePath, "rb");
    if (!f) {
        const char *notFound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nNot Found";
        send(clientSocket, notFound, (int)strlen(notFound), 0);
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(size);
    if (!buf) {
        fclose(f);
        return;
    }
    fread(buf, 1, size, f);
    fclose(f);

    char header[512];
    int headerLen = sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n",
        contentType, size);

    send(clientSocket, header, headerLen, 0);
    send(clientSocket, buf, size, 0);
    free(buf);
}

void sendJsonResponse(SOCKET clientSocket, const char *jsonBody) {
    char header[512];
    int bodyLen = (int)strlen(jsonBody);
    int headerLen = sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n\r\n",
        bodyLen);

    send(clientSocket, header, headerLen, 0);
    send(clientSocket, jsonBody, bodyLen, 0);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[Error] WSAStartup failed.\n");
        return 1;
    }

    initializeInventory();

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("[Error] Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    // Set socket option to reuse address
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("[Error] Bind failed on port %d.\n", PORT);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        printf("[Error] Listen failed.\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("\n=======================================================\n");
    printf("  SMART VILLAGE RELIEF WEB BRIDGE SERVER STARTED\n");
    printf("  Port: http://localhost:%d\n", PORT);
    printf("  Serving HTML, CSS, JS and REST API endpoints\n");
    printf("=======================================================\n\n");

    while (1) {
        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);

        if (clientSocket == INVALID_SOCKET) {
            continue;
        }

        char buffer[BUFFER_SIZE];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            closesocket(clientSocket);
            continue;
        }
        buffer[bytesRead] = '\0';

        // Parse Request Line
        char method[16] = {0};
        char path[256] = {0};
        sscanf(buffer, "%s %s", method, path);

        // Handle CORS Preflight
        if (strcmp(method, "OPTIONS") == 0) {
            const char *corsHeader =
                "HTTP/1.1 204 No Content\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n"
                "Connection: close\r\n\r\n";
            send(clientSocket, corsHeader, (int)strlen(corsHeader), 0);
            closesocket(clientSocket);
            continue;
        }

        // Pointer to request body (if POST)
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) body += 4;
        else body = "";

        char jsonResponse[32768];
        memset(jsonResponse, 0, sizeof(jsonResponse));

        // Route handling
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            serveStaticFile(clientSocket, "index.html", "text/html");
        } else if (strcmp(path, "/style.css") == 0) {
            serveStaticFile(clientSocket, "style.css", "text/css");
        } else if (strcmp(path, "/app.js") == 0) {
            serveStaticFile(clientSocket, "app.js", "application/javascript");
        } else if (strcmp(path, "/api/inventory") == 0 && strcmp(method, "GET") == 0) {
            handleViewInventory(jsonResponse);
            sendJsonResponse(clientSocket, jsonResponse);
        } else if (strcmp(path, "/api/register") == 0 && strcmp(method, "POST") == 0) {
            handleRegister(body, jsonResponse);
            sendJsonResponse(clientSocket, jsonResponse);
        } else if (strcmp(path, "/api/distribute") == 0 && strcmp(method, "POST") == 0) {
            handleDistribute(body, jsonResponse);
            sendJsonResponse(clientSocket, jsonResponse);
        } else if (strcmp(path, "/api/update-inventory") == 0 && strcmp(method, "POST") == 0) {
            handleUpdateInventory(body, jsonResponse);
            sendJsonResponse(clientSocket, jsonResponse);
        } else if (strcmp(path, "/api/report") == 0 && strcmp(method, "GET") == 0) {
            handleGenerateReport(jsonResponse, sizeof(jsonResponse));
            sendJsonResponse(clientSocket, jsonResponse);
        } else {
            const char *notFound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nNot Found";
            send(clientSocket, notFound, (int)strlen(notFound), 0);
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
