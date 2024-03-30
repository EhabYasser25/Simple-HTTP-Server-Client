#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <atomic>
#include <cmath>
#include <sstream>
#include <fstream>

using namespace std;

#define MAX_BUFF_LEN 4096

atomic_int connectionCount;

const string ROOT_PATH = "../";

void handleGet(SOCKET clientSocket, string& request) {
    istringstream iss(request);
    string method, filePath;
    iss >> method >> filePath;

    filePath = ROOT_PATH + filePath;

    ifstream file(filePath, ios::binary);
    if (file.is_open()) {
        ostringstream ss;
        ss << file.rdbuf();
        string fileContent = ss.str();
        cout << "Found" << endl << fileContent << endl << endl << endl;
        send(clientSocket, fileContent.c_str(), fileContent.size(), 0);
    } else {
        string errorMessage = "HTTP/1.1 404 Not Found\r\n\r\n";
        cout << "Not Found" << endl << errorMessage << endl;
        send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
    }
}


void handlePost(SOCKET clientSocket, string& request) {
    istringstream iss(request);
    string method, filePath;
    iss >> method >> filePath;

    ifstream file(filePath);

    if (!file) {
        string okMessage = "HTTP/1.1 200 OK\r\n\r\n";
        cout << "OK Sent" << endl << okMessage;
        send(clientSocket, okMessage.c_str(), okMessage.size(), 0);
    } else {
        string errorMessage = "HTTP/1.1 404 Not Found\r\n\r\n";
        cout << "Not Found" << endl << errorMessage;
        send(clientSocket, errorMessage.c_str(), errorMessage.size(), 0);
    }

    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, 4096, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        cout << "Received file:" << endl << buffer << endl << endl << endl;
        filePath = ROOT_PATH + filePath;
        ofstream outputFile(filePath);
        outputFile.write(buffer, bytesReceived);
    }
}

void clientHandler(SOCKET clientSocket, int clientSeq) {
    char buffer[MAX_BUFF_LEN];
    int recResult, sendResult;
    int currConnectionCount;
    // Setup timeout structure
    struct timeval timeout;
    // Setup structure encapsulating the socket we're waiting on
    struct fd_set fds;
    FD_ZERO(&fds);
    FD_SET(clientSocket, &fds);

    do {
        currConnectionCount = connectionCount.load();
        timeout.tv_sec = 15/currConnectionCount;
        timeout.tv_usec = trunc((15.0f/currConnectionCount - (int)(15/currConnectionCount)) * 1000000);
        switch ( select(0, &fds, 0, 0, &timeout) ) {

            case 0:
                cerr << "Connection timed out with client " << clientSeq << ", closing connection..." << endl;
                connectionCount--;
                return;

            case -1:
                cerr << "Connection with client " << clientSeq << " had an error: " << WSAGetLastError() << ". Closing connection..";
                connectionCount--;
                return;

            default:
                recResult = recv(clientSocket, buffer, MAX_BUFF_LEN, 0);
                if (recResult > 0) {
                    buffer[recResult] = '\0';
                    string request(buffer);

                    cout << "Received request:" << endl << request << endl;

                    istringstream iss(request);
                    string method;
                    iss >> method;

                    if (method == "GET")
                        handleGet(clientSocket, request);
                    else if (method == "POST")
                        handlePost(clientSocket, request);
                } else if (recResult == 0) {
                    printf("Connection closing with client %d...\n", clientSeq);
                } else {
                    printf("Receiving from client %d failed: %d\n", clientSeq, WSAGetLastError());
                    closesocket(clientSocket);
                    WSACleanup();
                }
                break;
        }
    } while (recResult > 0);

    connectionCount--;
}

int main(int argc, char** argv) {

    // initialize the winsock library
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) { // Used to use WS2_32.dll file in Windows
        cout << "WSA Startup failed";
        return 1;
    }

    struct addrinfo *hostinfo = NULL, *ptr = NULL, hints;

    // Provide hints for the getaddrinfo function to get the host address info
    ZeroMemory(&hints, sizeof(hints)); // Clear memory of the hints addrinfo structure before filling it out
    hints.ai_family = AF_INET; // Specify the IPv4 address family
    hints.ai_socktype = SOCK_STREAM; // Reliable two-way socket which uses TCP
    hints.ai_protocol = IPPROTO_TCP; // Use TCP as the protocol
    hints.ai_flags = AI_PASSIVE; // To hint that we will use the address info in a bind() call

    // Get the address info of the host to use it for port creation and binding
    if ( getaddrinfo(NULL, "80", &hints, &hostinfo) ) { // Check for an error
        cerr << "Failed to get host address info: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // Get the server SOCKET object
    SOCKET listener =  INVALID_SOCKET;
    listener = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
    // Check if the listener socket was set or not and exit on error
    if (listener == INVALID_SOCKET) {
        cerr << "Failed to open a socket for server to listen on: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // Bind the socket to the host address
    if ( bind(listener, hostinfo->ai_addr, (int) hostinfo->ai_addrlen) ) { // Check for errors
        cerr << "Failed to bind listener socket to host address: " << WSAGetLastError() << endl;
        freeaddrinfo(hostinfo);
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    cout << "Server has started listening on port " << ((argc == 2) ? argv[1] : "80") << ".. " << endl;
    // Clear memory of the host address info
    freeaddrinfo(hostinfo);

    // Start a while loop to listen for connections and accept them

    int clientSeq = 0;

    while (1) {

        // Start listening on the server socket
        if ( listen(listener, SOMAXCONN) == SOCKET_ERROR ) { // Check for errors
            cerr << "Failed to listen to connections: " << WSAGetLastError() << endl;
            WSACleanup();
            return 1;
        }

        // Accept client connections
        SOCKET clientSock = INVALID_SOCKET;
        clientSock = accept(listener, NULL, NULL);
        if (clientSock == INVALID_SOCKET) {
            cerr << "Failed to accept client connection: " << WSAGetLastError() << endl;
            continue;
        }

        connectionCount++;
        clientSeq++;
        thread clientThread(clientHandler, clientSock, clientSeq);
        clientThread.detach();
    }
}