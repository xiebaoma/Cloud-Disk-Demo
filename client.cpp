#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1" // 服务端的IP地址
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// 显示菜单
void showMenu()
{
    std::cout << "====================================" << std::endl;
    std::cout << "1. Upload File" << std::endl;
    std::cout << "2. List Files" << std::endl;
    std::cout << "3. Download File" << std::endl;
    std::cout << "4. Delete File" << std::endl;
    std::cout << "5. Exit" << std::endl;
    std::cout << "====================================" << std::endl;
}

int main()
{
    // 创建客户端套接字
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        std::cerr << "Failed to create client socket." << std::endl;
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid server IP address." << std::endl;
        close(clientSocket);
        return -1;
    }

    // 连接到服务器
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << "Connection to server failed." << std::endl;
        close(clientSocket);
        return -1;
    }

    std::cout << "Connected to server at " << SERVER_IP << ":" << SERVER_PORT << std::endl;

    while (true)
    {
        showMenu();

        int choice;
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        std::cin.ignore(); // 忽略输入缓冲区的换行符

        char buffer[BUFFER_SIZE];
        std::string filename, fileContent;

        switch (choice)
        {
        case 1: // 上传文件
            std::cout << "Enter the full path of the file to upload: ";
            std::getline(std::cin, filename);

            // 读取文件内容
            std::ifstream file(filename, std::ios::in | std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Failed to open the file." << std::endl;
                break;
            }

            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

            // 向服务端发送上传命令
            send(clientSocket, "UPLOAD", 6, 0);

            // 发送文件名
            send(clientSocket, filename.c_str(), filename.size(), 0);

            // 发送文件内容
            send(clientSocket, content.c_str(), content.size(), 0);

            // 接收服务端反馈
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0';
                std::cout << "Server response: " << buffer << std::endl;
            }
            file.close();
            break;

        case 2: // 列出文件
            send(clientSocket, "LIST", 4, 0);

            // 接收文件列表
            int bytesReceivedList = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceivedList > 0)
            {
                buffer[bytesReceivedList] = '\0';
                std::cout << "Files on server:\n"
                          << buffer << std::endl;
            }
            break;

        case 3: // 下载文件
            std::cout << "Enter the name of the file to download: ";
            std::getline(std::cin, filename);

            // 向服务端发送下载命令
            send(clientSocket, "DOWNLOAD", 8, 0);

            // 发送文件名
            send(clientSocket, filename.c_str(), filename.size(), 0);

            // 接收文件内容
            int bytesReceivedDownload = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceivedDownload > 0)
            {
                buffer[bytesReceivedDownload] = '\0';
                if (std::string(buffer) == "FILE NOT FOUND")
                {
                    std::cout << "File not found on the server." << std::endl;
                }
                else
                {
                    // 保存下载的文件
                    std::ofstream outFile("downloaded_" + filename, std::ios::out | std::ios::binary);
                    outFile.write(buffer, bytesReceivedDownload);
                    outFile.close();
                    std::cout << "File downloaded as 'downloaded_" << filename << "'." << std::endl;
                }
            }
            break;

        case 4: // 删除文件
            std::cout << "Enter the name of the file to delete: ";
            std::getline(std::cin, filename);

            // 向服务端发送删除命令
            send(clientSocket, "DELETE", 6, 0);

            // 发送文件名
            send(clientSocket, filename.c_str(), filename.size(), 0);

            // 接收服务端反馈
            int bytesReceivedDelete = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceivedDelete > 0)
            {
                buffer[bytesReceivedDelete] = '\0';
                std::cout << "Server response: " << buffer << std::endl;
            }
            break;

        case 5: // 退出
            std::cout << "Exiting..." << std::endl;
            close(clientSocket);
            return 0;

        default:
            std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }

    return 0;
}
