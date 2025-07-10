#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>

// MySQL C API ���
#include <mysql.h>
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "Ws2_32.lib")

enum RequestType : uint8_t {
    LoginReq = 0,
    RegisterReq = 1
};

enum ResponseType : uint8_t {
    SimpleResponse = 0,        // ���� 1����Ʈ ���� (ȸ������, �α��� ����)
    CharacterDataResponse = 1  // ĳ���� ������ ���� ���� (�α��� ����)
};

MYSQL* mysql_conn = nullptr;
std::mutex mysql_mutex;

bool ConnectMySQL() {
    mysql_conn = mysql_init(NULL);
    if (!mysql_conn) {
        printf("MySQL �ʱ�ȭ ����\n");
        return false;
    }

    if (!mysql_real_connect(mysql_conn,
        "localhost",    // ȣ��Ʈ
        "root",         // �����
        "2022180049!gksrnr", // ��й�ȣ
        NULL,           // �����ͺ��̽� (���߿� ����)
        3306,           // ��Ʈ
        NULL,           // ����
        0)) {           // �÷���
        printf("MySQL ���� ����: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        return false;
    }

    printf("MySQL ���� ����!\n");

    if (mysql_query(mysql_conn, "USE game_db")) {
        printf("�����ͺ��̽� ���� ����: %s\n", mysql_error(mysql_conn));
        return false;
    }

    printf("game_db �����ͺ��̽� ��� ��\n");
    return true;
}

bool ProcessRegister(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mysql_mutex);

    // �ߺ� Ȯ��
    std::string check_query = "SELECT id FROM Users WHERE username = '" + username + "'";

    if (mysql_query(mysql_conn, check_query.c_str())) {
        printf("�ߺ� Ȯ�� ���� ����: %s\n", mysql_error(mysql_conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (!result) {
        printf("��� ���� ����: %s\n", mysql_error(mysql_conn));
        return false;
    }

    bool user_exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);

    if (user_exists) {
        printf("ȸ������ ����: ����� '%s' �̹� ����\n", username.c_str());
        return false;
    }

    // �� ����� ���
    std::string insert_query = "INSERT INTO Users (username, password) VALUES ('" +
        username + "', '" + password + "')";

    if (mysql_query(mysql_conn, insert_query.c_str())) {
        printf("����� ��� ����: %s\n", mysql_error(mysql_conn));
        return false;
    }

    // ���� ������ ����� ID ��������
    int user_id = mysql_insert_id(mysql_conn);

    // �⺻ ĳ���� ����
    std::string create_character_query =
        "INSERT INTO Characters (user_id, character_name, player_class, gold) VALUES (" +
        std::to_string(user_id) + ", '" + username + "_Character', 0, 1000)";

    if (mysql_query(mysql_conn, create_character_query.c_str())) {
        printf("�⺻ ĳ���� ���� ����: %s\n", mysql_error(mysql_conn));
        return false;
    }

    printf("ȸ������ ����: %s (�⺻ ĳ���� ������)\n", username.c_str());
    return true;
}

std::string ProcessLogin(const std::string& username, const std::string& password, bool& success) {
    std::lock_guard<std::mutex> lock(mysql_mutex);
    success = false;

    // ����� ����
    std::string login_query = "SELECT id, password FROM Users WHERE username = '" + username + "'";

    if (mysql_query(mysql_conn, login_query.c_str())) {
        printf("�α��� ���� ����: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (!result) {
        printf("��� ���� ����: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        printf("�α��� ����: ����� '%s' ����\n", username.c_str());
        return "";
    }

    int user_id = atoi(row[0]);
    std::string db_password = row[1];
    mysql_free_result(result);

    if (db_password != password) {
        printf("�α��� ����: �߸��� ��й�ȣ (�����: %s)\n", username.c_str());
        return "";
    }

    // ĳ���� ������ �ε�
    std::string character_query =
        "SELECT id, character_name, player_class, level, max_health, max_stamina, max_knowledge, gold "
        "FROM Characters WHERE user_id = " + std::to_string(user_id) + " LIMIT 1";

    if (mysql_query(mysql_conn, character_query.c_str())) {
        printf("ĳ���� ������ �ε� ����: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_RES* char_result = mysql_store_result(mysql_conn);
    if (!char_result) {
        printf("ĳ���� ��� ���� ����: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_ROW char_row = mysql_fetch_row(char_result);
    if (!char_row) {
        mysql_free_result(char_result);
        printf("ĳ���� ������ ���� (�����: %s)\n", username.c_str());
        return "";
    }

    // ������ JSON ���·� ĳ���� ������ ����
    std::stringstream json;
    json << "{";
    json << "\"character_id\":" << char_row[0] << ",";
    json << "\"character_name\":\"" << char_row[1] << "\",";
    json << "\"player_class\":" << char_row[2] << ",";
    json << "\"level\":" << char_row[3] << ",";
    json << "\"max_health\":" << char_row[4] << ",";
    json << "\"max_stamina\":" << char_row[5] << ",";
    json << "\"max_knowledge\":" << char_row[6] << ",";
    json << "\"gold\":" << char_row[7];
    json << "}";

    mysql_free_result(char_result);
    success = true;

    printf("�α��� ����: %s (ĳ����: %s, ����: %s, ���: %s)\n",
        username.c_str(), char_row[1], char_row[2], char_row[7]);

    return json.str();
}

// �� Ŭ���̾�Ʈ�� ���� �����忡�� ó��
void HandleClient(SOCKET clientSocket, int clientId) {
    char ipStr[INET_ADDRSTRLEN] = {};
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(clientSocket, (sockaddr*)&clientAddr, &addrLen);
    InetNtopA(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

    printf("[Ŭ���̾�Ʈ %d] �����: %s:%d\n", clientId, ipStr, ntohs(clientAddr.sin_port));

    const int BUF = 4096;
    std::vector<uint8_t> buf(BUF);

    // Ŭ���̾�Ʈ�� �������� ���
    while (true) {
        int received = recv(clientSocket, (char*)buf.data(), BUF, 0);
        if (received <= 0) {
            printf("[Ŭ���̾�Ʈ %d] ���� ���� (recv: %d)\n", clientId, received);
            break;
        }

        printf("[Ŭ���̾�Ʈ %d] ��û ���� (%d bytes)\n", clientId, received);

        // �������� �Ľ�
        uint8_t req = buf[0];
        int idx = 1;

        auto readLen = [&](uint16_t& L) {
            L = buf[idx] | (buf[idx + 1] << 8);
            idx += 2;
            };

        uint16_t uLen = 0, pLen = 0;
        readLen(uLen);
        std::string user((char*)&buf[idx], uLen);
        idx += uLen;
        readLen(pLen);
        std::string pass((char*)&buf[idx], pLen);

        // ��û ó��
        if (req == RegisterReq) {
            printf("[Ŭ���̾�Ʈ %d] ȸ������ ��û: %s\n", clientId, user.c_str());

            bool ok = ProcessRegister(user, pass);

            // ȸ�������� ���� ��� (1����Ʈ ����)
            uint8_t resp = ok ? 1 : 0;
            int sent = send(clientSocket, (char*)&resp, 1, 0);
            printf("[Ŭ���̾�Ʈ %d] ȸ������ ����: %s (%d bytes)\n",
                clientId, ok ? "����" : "����", sent);
        }
        else if (req == LoginReq) {
            printf("[Ŭ���̾�Ʈ %d] �α��� ��û: %s\n", clientId, user.c_str());

            bool success = false;
            std::string characterData = ProcessLogin(user, pass, success);

            if (success && !characterData.empty()) {
                // �α��� ����: [����Ÿ��][������ũ��][JSON������]
                uint8_t respType = CharacterDataResponse;
                uint32_t dataLen = characterData.length();

                // ���� ��Ŷ ����
                std::vector<uint8_t> response;
                response.push_back(respType);  // 1����Ʈ: ���� Ÿ��

                // 4����Ʈ: ������ ũ�� (��Ʋ �����)
                response.push_back((dataLen >> 0) & 0xFF);
                response.push_back((dataLen >> 8) & 0xFF);
                response.push_back((dataLen >> 16) & 0xFF);
                response.push_back((dataLen >> 24) & 0xFF);

                // JSON ������
                response.insert(response.end(), characterData.begin(), characterData.end());

                int sent = send(clientSocket, (char*)response.data(), response.size(), 0);
                printf("[Ŭ���̾�Ʈ %d] �α��� ����, ĳ���� ������ ���� (%d bytes)\n",
                    clientId, sent);
                printf("[Ŭ���̾�Ʈ %d] JSON: %s\n", clientId, characterData.c_str());
            }
            else {
                // �α��� ����: ���� ��� (0)
                uint8_t resp = 0;
                int sent = send(clientSocket, (char*)&resp, 1, 0);
                printf("[Ŭ���̾�Ʈ %d] �α��� ���� (%d bytes)\n", clientId, sent);
            }
        }
    }

    closesocket(clientSocket);
    printf("[Ŭ���̾�Ʈ %d] ���� ���� �Ϸ�\n", clientId);
}

int main() {
    printf("=== ĳ���� ������ ���� MySQL Echo Server ���� ===\n");

    // MySQL ����
    if (!ConnectMySQL()) {
        printf("MySQL ���� ���з� ������ �����մϴ�.\n");
        system("pause");
        return 1;
    }

    // Winsock �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup ����\n");
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        printf("���� ���� ����\n");
        return 1;
    }

    // SO_REUSEADDR ���� (�ּ� ���� ���)
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(3000);

    if (bind(listenSock, (sockaddr*)&service, sizeof(service)) == SOCKET_ERROR) {
        printf("bind ����: %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen ����: %d\n", WSAGetLastError());
        return 1;
    }

    printf("���� ���� - ��Ʈ 3000���� ĳ���� ������ ���� ��� ��...\n");

    int clientIdCounter = 1;
    std::vector<std::thread> clientThreads;

    while (true) {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET client = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);

        if (client == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }

        // �� Ŭ���̾�Ʈ�� ���� �����忡�� ó��
        int currentClientId = clientIdCounter++;
        clientThreads.emplace_back(HandleClient, client, currentClientId);
        clientThreads.back().detach();  // ������ ���� ����
    }

    // ����
    if (mysql_conn) mysql_close(mysql_conn);
    closesocket(listenSock);
    WSACleanup();

    return 0;
}