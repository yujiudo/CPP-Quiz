
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void sendLine(SOCKET server, string message)
{
    message += "\n";

    int sent = 0;

    while (sent < message.length())
    {
        int result = send(server, message.c_str() + sent,
            message.length() - sent, 0);

        if (result <= 0)
            return;

        sent += result;
    }
}

string recvLine(SOCKET server)
{
    string message;
    char ch;

    while (true)
    {
        int result = recv(server, &ch, 1, 0);

        if (result <= 0)
            return "";

        if (ch == '\n')
            break;

        if (ch != '\r')
            message += ch;
    }

    return message;
}

int main()
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cout << "WSAStartup error" << endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Socket error" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddress,
        sizeof(serverAddress)) == SOCKET_ERROR)
    {
        cout << "Connection error. Server is not running."
            << endl;

        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server!" << endl;

    string message = recvLine(clientSocket);
    cout << message << endl;

    while (true)
    {
        cout << "\nEnter command: ";

        string command;
        getline(cin, command);

        if (command.empty())
            continue;

        sendLine(clientSocket, command);

        string response = recvLine(clientSocket);

        if (response.empty())
        {
            cout << "Server disconnected." << endl;
            break;
        }

        cout << response << endl;

        if (command == "QUIT" || command == "quit" ||
            command == "Quit")
        {
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
