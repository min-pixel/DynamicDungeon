#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>

// MySQL C API 사용
#include <mysql.h>
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "Ws2_32.lib")

enum RequestType : uint8_t {
    LoginReq = 0,
    RegisterReq = 1
};

enum ResponseType : uint8_t {
    SimpleResponse = 0,        // 기존 1바이트 응답 (회원가입, 로그인 실패)
    CharacterDataResponse = 1  // 캐릭터 데이터 포함 응답 (로그인 성공)
};

MYSQL* mysql_conn = nullptr;
std::mutex mysql_mutex;

bool ConnectMySQL() {
    mysql_conn = mysql_init(NULL);
    if (!mysql_conn) {
        printf("MySQL 초기화 실패\n");
        return false;
    }

    if (!mysql_real_connect(mysql_conn,
        "localhost",    // 호스트
        "root",         // 사용자
        "2022180049!gksrnr", // 비밀번호
        NULL,           // 데이터베이스 (나중에 선택)
        3306,           // 포트
        NULL,           // 소켓
        0)) {           // 플래그
        printf("MySQL 연결 실패: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        return false;
    }

    printf("MySQL 연결 성공!\n");

    if (mysql_query(mysql_conn, "USE game_db")) {
        printf("데이터베이스 선택 실패: %s\n", mysql_error(mysql_conn));
        return false;
    }

    printf("game_db 데이터베이스 사용 중\n");
    return true;
}

bool ProcessRegister(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mysql_mutex);

    // 중복 확인
    std::string check_query = "SELECT id FROM Users WHERE username = '" + username + "'";

    if (mysql_query(mysql_conn, check_query.c_str())) {
        printf("중복 확인 쿼리 실패: %s\n", mysql_error(mysql_conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (!result) {
        printf("결과 저장 실패: %s\n", mysql_error(mysql_conn));
        return false;
    }

    bool user_exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);

    if (user_exists) {
        printf("회원가입 실패: 사용자 '%s' 이미 존재\n", username.c_str());
        return false;
    }

    // 새 사용자 등록
    std::string insert_query = "INSERT INTO Users (username, password) VALUES ('" +
        username + "', '" + password + "')";

    if (mysql_query(mysql_conn, insert_query.c_str())) {
        printf("사용자 등록 실패: %s\n", mysql_error(mysql_conn));
        return false;
    }

    // 새로 생성된 사용자 ID 가져오기
    int user_id = mysql_insert_id(mysql_conn);

    // 기본 캐릭터 생성
    std::string create_character_query =
        "INSERT INTO Characters (user_id, character_name, player_class, gold) VALUES (" +
        std::to_string(user_id) + ", '" + username + "_Character', 0, 1000)";

    if (mysql_query(mysql_conn, create_character_query.c_str())) {
        printf("기본 캐릭터 생성 실패: %s\n", mysql_error(mysql_conn));
        return false;
    }

    printf("회원가입 성공: %s (기본 캐릭터 생성됨)\n", username.c_str());
    return true;
}

std::string ProcessLogin(const std::string& username, const std::string& password, bool& success) {
    std::lock_guard<std::mutex> lock(mysql_mutex);
    success = false;

    // 사용자 인증
    std::string login_query = "SELECT id, password FROM Users WHERE username = '" + username + "'";

    if (mysql_query(mysql_conn, login_query.c_str())) {
        printf("로그인 쿼리 실패: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_RES* result = mysql_store_result(mysql_conn);
    if (!result) {
        printf("결과 저장 실패: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        printf("로그인 실패: 사용자 '%s' 없음\n", username.c_str());
        return "";
    }

    int user_id = atoi(row[0]);
    std::string db_password = row[1];
    mysql_free_result(result);

    if (db_password != password) {
        printf("로그인 실패: 잘못된 비밀번호 (사용자: %s)\n", username.c_str());
        return "";
    }

    // 캐릭터 데이터 로드
    std::string character_query =
        "SELECT id, character_name, player_class, level, max_health, max_stamina, max_knowledge, gold "
        "FROM Characters WHERE user_id = " + std::to_string(user_id) + " LIMIT 1";

    if (mysql_query(mysql_conn, character_query.c_str())) {
        printf("캐릭터 데이터 로드 실패: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_RES* char_result = mysql_store_result(mysql_conn);
    if (!char_result) {
        printf("캐릭터 결과 저장 실패: %s\n", mysql_error(mysql_conn));
        return "";
    }

    MYSQL_ROW char_row = mysql_fetch_row(char_result);
    if (!char_row) {
        mysql_free_result(char_result);
        printf("캐릭터 데이터 없음 (사용자: %s)\n", username.c_str());
        return "";
    }

    // 간단한 JSON 형태로 캐릭터 데이터 구성
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

    printf("로그인 성공: %s (캐릭터: %s, 직업: %s, 골드: %s)\n",
        username.c_str(), char_row[1], char_row[2], char_row[7]);

    return json.str();
}

// 각 클라이언트를 별도 스레드에서 처리
void HandleClient(SOCKET clientSocket, int clientId) {
    char ipStr[INET_ADDRSTRLEN] = {};
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(clientSocket, (sockaddr*)&clientAddr, &addrLen);
    InetNtopA(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

    printf("[클라이언트 %d] 연결됨: %s:%d\n", clientId, ipStr, ntohs(clientAddr.sin_port));

    const int BUF = 4096;
    std::vector<uint8_t> buf(BUF);

    // 클라이언트와 지속적인 통신
    while (true) {
        int received = recv(clientSocket, (char*)buf.data(), BUF, 0);
        if (received <= 0) {
            printf("[클라이언트 %d] 연결 종료 (recv: %d)\n", clientId, received);
            break;
        }

        printf("[클라이언트 %d] 요청 받음 (%d bytes)\n", clientId, received);

        // 프로토콜 파싱
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

        // 요청 처리
        if (req == RegisterReq) {
            printf("[클라이언트 %d] 회원가입 요청: %s\n", clientId, user.c_str());

            bool ok = ProcessRegister(user, pass);

            // 회원가입은 기존 방식 (1바이트 응답)
            uint8_t resp = ok ? 1 : 0;
            int sent = send(clientSocket, (char*)&resp, 1, 0);
            printf("[클라이언트 %d] 회원가입 응답: %s (%d bytes)\n",
                clientId, ok ? "성공" : "실패", sent);
        }
        else if (req == LoginReq) {
            printf("[클라이언트 %d] 로그인 요청: %s\n", clientId, user.c_str());

            bool success = false;
            std::string characterData = ProcessLogin(user, pass, success);

            if (success && !characterData.empty()) {
                // 로그인 성공: [응답타입][데이터크기][JSON데이터]
                uint8_t respType = CharacterDataResponse;
                uint32_t dataLen = characterData.length();

                // 응답 패킷 구성
                std::vector<uint8_t> response;
                response.push_back(respType);  // 1바이트: 응답 타입

                // 4바이트: 데이터 크기 (리틀 엔디안)
                response.push_back((dataLen >> 0) & 0xFF);
                response.push_back((dataLen >> 8) & 0xFF);
                response.push_back((dataLen >> 16) & 0xFF);
                response.push_back((dataLen >> 24) & 0xFF);

                // JSON 데이터
                response.insert(response.end(), characterData.begin(), characterData.end());

                int sent = send(clientSocket, (char*)response.data(), response.size(), 0);
                printf("[클라이언트 %d] 로그인 성공, 캐릭터 데이터 전송 (%d bytes)\n",
                    clientId, sent);
                printf("[클라이언트 %d] JSON: %s\n", clientId, characterData.c_str());
            }
            else {
                // 로그인 실패: 기존 방식 (0)
                uint8_t resp = 0;
                int sent = send(clientSocket, (char*)&resp, 1, 0);
                printf("[클라이언트 %d] 로그인 실패 (%d bytes)\n", clientId, sent);
            }
        }
    }

    closesocket(clientSocket);
    printf("[클라이언트 %d] 연결 해제 완료\n", clientId);
}

int main() {
    printf("=== 캐릭터 데이터 지원 MySQL Echo Server 시작 ===\n");

    // MySQL 연결
    if (!ConnectMySQL()) {
        printf("MySQL 연결 실패로 서버를 종료합니다.\n");
        system("pause");
        return 1;
    }

    // Winsock 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup 실패\n");
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        printf("소켓 생성 실패\n");
        return 1;
    }

    // SO_REUSEADDR 설정 (주소 재사용 허용)
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(3000);

    if (bind(listenSock, (sockaddr*)&service, sizeof(service)) == SOCKET_ERROR) {
        printf("bind 실패: %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen 실패: %d\n", WSAGetLastError());
        return 1;
    }

    printf("서버 시작 - 포트 3000에서 캐릭터 데이터 서비스 대기 중...\n");

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

        // 새 클라이언트를 별도 스레드에서 처리
        int currentClientId = clientIdCounter++;
        clientThreads.emplace_back(HandleClient, client, currentClientId);
        clientThreads.back().detach();  // 스레드 독립 실행
    }

    // 정리
    if (mysql_conn) mysql_close(mysql_conn);
    closesocket(listenSock);
    WSACleanup();

    return 0;
}