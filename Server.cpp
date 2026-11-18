
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

struct Question
{
    string text;
    string options[4];
    char correct;
};

class Quiz
{
private:
    Question questions[100];
    int order[100];

    int count = 0;
    int current = 0;
    int score = 0;

    bool finished = false;
    bool saved = false;

    string playerName = "Player";

public:
    Quiz()
    {
        loadQuestions();
        shuffleQuestions();
    }

    void loadQuestions()
    {
        ifstream file("questions.txt");

        if (!file.is_open())
        {
            cout << "Cannot open questions.txt" << endl;
            return;
        }

        string line;

        while (getline(file, line) && count < 100)
        {
            stringstream ss(line);
            string answer;

            getline(ss, questions[count].text, '|');

            for (int i = 0; i < 4; i++)
                getline(ss, questions[count].options[i], '|');

            getline(ss, answer);

            if (!answer.empty())
            {
                questions[count].correct = answer[0];

                if (questions[count].correct >= 'a' &&
                    questions[count].correct <= 'z')
                {
                    questions[count].correct -= 32;
                }

                count++;
            }
        }

        cout << "Questions loaded: " << count << endl;
    }

    void shuffleQuestions()
    {
        for (int i = 0; i < count; i++)
            order[i] = i;

        for (int i = count - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);

            int temp = order[i];
            order[i] = order[j];
            order[j] = temp;
        }
    }

    string showQuestion()
    {
        if (current >= count)
            return "Quiz finished!";

        int index = order[current];

        string message = "Question ";
        message += to_string(current + 1) + "/";
        message += to_string(count) + ": ";
        message += questions[index].text;

        message += " | A) " + questions[index].options[0];
        message += " | B) " + questions[index].options[1];
        message += " | C) " + questions[index].options[2];
        message += " | D) " + questions[index].options[3];

        message += " | Enter your answer.";

        return message;
    }

    void restart()
    {
        current = 0;
        score = 0;
        finished = false;
        saved = false;

        shuffleQuestions();
    }

    void saveRecord()
    {
        if (saved)
            return;

        ofstream file("records.txt", ios::app);

        if (file.is_open())
        {
            file << playerName << " " << score << endl;
            saved = true;
        }
    }

    string showTop()
    {
        ifstream file("records.txt");

        if (!file.is_open())
            return "No records yet.";

        string result = "TOP records: ";
        string name;
        int points;
        int number = 1;

        while (file >> name >> points)
        {
            if (number > 1)
                result += " | ";

            result += name + " - " + to_string(points);
            number++;
        }

        if (number == 1)
            return "No records yet.";

        return result;
    }

    string handle(string command)
    {
        string upper = command;

        for (int i = 0; i < upper.length(); i++)
        {
            if (upper[i] >= 'a' && upper[i] <= 'z')
                upper[i] -= 32;
        }

        if (upper == "SCORE")
            return "Your score: " + to_string(score);

        if (upper == "TOP")
            return showTop();

        if (upper.find("NAME ") == 0)
        {
            playerName = command.substr(5);

            if (playerName.empty())
                return "Error. Enter your name.";

            return "Name set: " + playerName;
        }

        if (upper == "RESTART")
        {
            restart();
            return "New game! " + showQuestion();
        }

        if (upper == "QUIT")
            return "Goodbye!";

        if (count == 0)
            return "No questions loaded.";

        if (finished)
            return "Quiz finished. Enter RESTART to play again.";

        if (upper == "SKIP")
        {
            current++;

            if (current == count)
            {
                finished = true;
                saveRecord();

                return "Quiz finished! Your score: "
                    + to_string(score) + "/" + to_string(count);
            }

            return "Question skipped. Next question: " + showQuestion();
        }

        if (upper == "A" || upper == "B" ||
            upper == "C" || upper == "D")
        {
            int index = order[current];
            bool right = (upper[0] == questions[index].correct);

            if (right)
                score++;

            current++;

            if (current == count)
            {
                finished = true;
                saveRecord();

                if (right)
                    return "Right! Quiz finished! Your score: "
                    + to_string(score) + "/" + to_string(count);

                return "Wrong! Quiz finished! Your score: "
                    + to_string(score) + "/" + to_string(count);
            }

            if (right)
                return "Right! Next question: " + showQuestion();

            return "Wrong! Next question: " + showQuestion();
        }

        return "Error. Enter A, B, C, D or a command.";
    }

    string welcome()
    {
        if (count == 0)
            return "No questions loaded!";

        return "Welcome to the quiz! " + showQuestion();
    }
};

void sendLine(SOCKET client, string message)
{
    message += "\n";

    int sent = 0;

    while (sent < message.length())
    {
        int result = send(client, message.c_str() + sent,
            message.length() - sent, 0);

        if (result <= 0)
            return;

        sent += result;
    }
}

string recvLine(SOCKET client)
{
    string message;
    char ch;

    while (true)
    {
        int result = recv(client, &ch, 1, 0);

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

    srand((unsigned int)time(0));

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cout << "WSAStartup error" << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET)
    {
        cout << "Socket error" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddress,
        sizeof(serverAddress)) == SOCKET_ERROR)
    {
        cout << "Bind error" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR)
    {
        cout << "Listen error" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server is waiting for a client..." << endl;

    sockaddr_in clientAddress = {};
    int clientLength = sizeof(clientAddress);

    SOCKET client = accept(serverSocket,
        (sockaddr*)&clientAddress, &clientLength);

    if (client == INVALID_SOCKET)
    {
        cout << "Accept error" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Client connected!" << endl;

    Quiz quiz;

    sendLine(client, quiz.welcome());

    while (true)
    {
        string command = recvLine(client);

        if (command.empty())
            break;

        string response = quiz.handle(command);

        sendLine(client, response);

        cout << "Command: " << command << endl;
        cout << "Response: " << response << endl;

        if (response == "Bye Bye Russia!")
            break;
    }

    closesocket(client);
    closesocket(serverSocket);

    WSACleanup();

    return 0;
}
