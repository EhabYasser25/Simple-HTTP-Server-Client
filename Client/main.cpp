#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>

using namespace std;

const string ROOT_PATH = "../";

void handleGetRequest(SOCKET clientSocket, string& filePath) {
    string filename = ROOT_PATH + filesystem::path(filePath).filename().string();

    string getRequestMessage = "GET " + filePath + " HTTP/1.1\r\n\r\n";
    send(clientSocket, getRequestMessage.c_str(), getRequestMessage.size(), 0);

    cout << "Request message: " << endl << getRequestMessage;
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, 4096, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        if (strcmp(buffer, "HTTP/1.1 404 Not Found\r\n\r\n") == 0) {
            cout << "File is not found" << endl << endl << endl;
        } else {
            cout << "Received file:" << endl << buffer << endl << endl << endl;
            ofstream outputFile(filename);
            outputFile.write(buffer, bytesReceived);
        }
    }
}

void handlePostRequest(SOCKET clientSocket, string& filePath) {
    string filename = ROOT_PATH + filesystem::path(filePath).filename().string();

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cout << "Failed to open file: " << filename << endl << endl << endl;
        return;
    }

    string postRequestMessage = "POST " + filePath + " HTTP/1.1\r\n\r\n";

    cout << "Request message: " << endl << postRequestMessage;

    send(clientSocket, postRequestMessage.c_str(), postRequestMessage.size(), 0);

    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, 4096, 0);
    buffer[bytesReceived] = '\0';

    if (strcmp(buffer, "HTTP/1.1 200 OK\r\n\r\n") == 0) {
        ostringstream ss;
        ss << file.rdbuf();
        string fileContent = ss.str();
        send(clientSocket, fileContent.c_str(), fileContent.size(), 0);
    }

    cout << "Received message:" << endl << buffer << endl << endl << endl;
}

SOCKET startConnection(char* hostname, u_short portnumber) {

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(portnumber);
    inet_pton(AF_INET, hostname, &serverAddress.sin_addr);

    int connectResult = connect(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    if (connectResult == SOCKET_ERROR) {
        cerr << "Connection failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    return serverSocket;
}

SOCKET readCommands(SOCKET serverSocket) {
    cout << "Starting client.." << endl << endl << endl << endl;

    string commandsFile = "commands.txt";
    ifstream inputFile(ROOT_PATH + commandsFile);
    if (!inputFile.is_open()) {
        cerr << "Error opening file: " << commandsFile << endl;
        return -1;
    }

    string line;
    while (getline(inputFile, line)) { // Loop over each command
        cout << line << endl << endl; // Output the command
        istringstream iss(line); // Initialize a stream object for the command
        string command, filePath, portnumstr, hostname;
        u_short portnum;
        iss >> command >> filePath >> hostname;

        if (iss) { // If the stream is not empty, port number is provided
            iss >> portnumstr;
            portnum = stoi(portnumstr);
        } else { // If the stream is empty, we will use the default 80 port number
            portnum = 80;
        }

        if (command == "get")
            handleGetRequest(serverSocket, filePath);
        else if (command == "post")
            handlePostRequest(serverSocket, filePath);
    }

    inputFile.close();
    return serverSocket;
}

void endConnection(SOCKET clientSocket) {
    int iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
    }
}

int main(int argc, char** argv) {
    char *hostname = "127.0.0.1";
    int portnumber = 80;
    if (argc == 3) {
        hostname = argv[1];
        portnumber = stoi(argv[2]);
    }

    WSADATA wsaData;
    int startUp = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (startUp != 0) {
        cerr << "WSAStartup failed: " << startUp << endl;
        return -1;
    }

    SOCKET serverSocket = INVALID_SOCKET;

    if (serverSocket == INVALID_SOCKET)
        serverSocket = startConnection(hostname, portnumber);

    if (serverSocket == -1)
        exit(1);


    readCommands(serverSocket);
    endConnection(serverSocket);
}
