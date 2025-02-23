#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 用于存储文件名和文件内容的映射
std::unordered_map<std::string, std::string> fileStorage;

// 处理客户端请求
void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // 接收客户端请求
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Failed to receive data from client." << std::endl;
        close(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';

    std::string command(buffer);

    if (command == "UPLOAD") {
        // 上传文件的过程
        char filename[BUFFER_SIZE];
        char fileContent[BUFFER_SIZE];

        // 接收文件名
        bytesReceived = recv(clientSocket, filename, sizeof(filename), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Failed to receive filename." << std::endl;
            close(clientSocket);
            return;
        }
        filename[bytesReceived] = '\0';

        // 接收文件内容
        bytesReceived = recv(clientSocket, fileContent, sizeof(fileContent), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Failed to receive file content." << std::endl;
            close(clientSocket);
            return;
        }
        fileContent[bytesReceived] = '\0';

        // 保存文件到文件存储
        fileStorage[filename] = std::string(fileContent);
        std::cout << "File uploaded: " << filename << std::endl;
        send(clientSocket, "UPLOAD SUCCESS", 15, 0);
    }
    else if (command == "LIST") {
        // 列出服务器上存储的文件
        std::string fileList;
        for (const auto& entry : fileStorage) {
            fileList += entry.first + "\n";
        }

        send(clientSocket, fileList.c_str(), fileList.size(), 0);
    }
    else if (command == "DELETE") {
        // 删除文件
        char filename[BUFFER_SIZE];
        bytesReceived = recv(clientSocket, filename, sizeof(filename), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Failed to receive filename for deletion." << std::endl;
            close(clientSocket);
            return;
        }
        filename[bytesReceived] = '\0';

        if (fileStorage.find(filename) != fileStorage.end()) {
            fileStorage.erase(filename);
            std::cout << "File deleted: " << filename << std::endl;
            send(clientSocket, "DELETE SUCCESS", 14, 0);
        } else {
            send(clientSocket, "FILE NOT FOUND", 15, 0);
        }
    }
    else if (command == "DOWNLOAD") {
        // 下载文件
        char filename[BUFFER_SIZE];
        bytesReceived = recv(clientSocket, filename, sizeof(filename), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Failed to receive filename for download." << std::endl;
            close(clientSocket);
            return;
        }
        filename[bytesReceived] = '\0';

        if (fileStorage.find(filename) != fileStorage.end()) {
            send(clientSocket, fileStorage[filename].c_str(), fileStorage[filename].size(), 0);
        } else {
            send(clientSocket, "FILE NOT FOUND", 15, 0);
        }
    }
    else {
        send(clientSocket, "UNKNOWN COMMAND", 15, 0);
    }

    close(clientSocket);
}

int main() {
    // 创建服务端套接字
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create server socket." << std::endl;
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // 绑定套接字到指定的端口
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Failed to bind socket." << std::endl;
        close(serverSocket);
        return -1;
    }

    // 监听连接
    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(serverSocket);
        return -1;
    }

    std::cout << "Server is running and waiting for connections..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        // 接受客户端连接
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::cout << "Client connected: " << inet_ntoa(clientAddr.sin_addr) << std::endl;

        // 处理客户端请求
        handleClient(clientSocket);
    }

    close(serverSocket);
    return 0;
}
