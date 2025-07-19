#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <memory>
#include <atomic>

// MySQL C API 헤더
#include <mysql.h>
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "Ws2_32.lib")

enum RequestType : uint8_t {
    LoginReq = 0,
    RegisterReq = 1,
    SaveDataReq = 2
};

enum ResponseType : uint8_t {
    SimpleResponse = 0,
    CharacterDataResponse = 1,
    SaveDataResponse = 2
};

// MySQL 연결 풀 관리
class MySQLConnectionPool {
private:
    struct Connection {
        MYSQL* conn;
        bool inUse;
        std::thread::id owner;
    };

    std::vector<std::unique_ptr<Connection>> connections;
    std::mutex poolMutex;
    std::string host, user, password, database;
    int port;

public:
    MySQLConnectionPool(const std::string& h, const std::string& u,
        const std::string& p, const std::string& d, int pt)
        : host(h), user(u), password(p), database(d), port(pt) {}

    ~MySQLConnectionPool() {
        std::lock_guard<std::mutex> lock(poolMutex);
        for (auto& conn : connections) {
            if (conn->conn) {
                mysql_close(conn->conn);
            }
        }
    }

    MYSQL* getConnection() {
        std::lock_guard<std::mutex> lock(poolMutex);

        // 현재 스레드가 이미 연결을 가지고 있는지 확인
        auto currentThread = std::this_thread::get_id();
        for (auto& conn : connections) {
            if (conn->owner == currentThread && conn->inUse) {
                return conn->conn;
            }
        }

        // 사용 가능한 연결 찾기
        for (auto& conn : connections) {
            if (!conn->inUse) {
                conn->inUse = true;
                conn->owner = currentThread;
                // 연결 상태 확인
                if (mysql_ping(conn->conn) != 0) {
                    if (!reconnect(conn->conn)) {
                        conn->inUse = false;
                        return nullptr;
                    }
                }
                return conn->conn;
            }
        }

        // 새 연결 생성
        auto newConn = std::make_unique<Connection>();
        newConn->conn = createNewConnection();
        if (!newConn->conn) {
            return nullptr;
        }
        newConn->inUse = true;
        newConn->owner = currentThread;
        MYSQL* result = newConn->conn;
        connections.push_back(std::move(newConn));

        printf("[MySQL Pool] Created new connection for thread %llu (total: %zu)\n",
            std::hash<std::thread::id>{}(currentThread), connections.size());

        return result;
    }

    void releaseConnection() {
        std::lock_guard<std::mutex> lock(poolMutex);
        auto currentThread = std::this_thread::get_id();

        for (auto& conn : connections) {
            if (conn->owner == currentThread && conn->inUse) {
                conn->inUse = false;
                printf("[MySQL Pool] Released connection for thread %llu\n",
                    std::hash<std::thread::id>{}(currentThread));
                break;
            }
        }
    }

private:
    MYSQL* createNewConnection() {
        MYSQL* conn = mysql_init(NULL);
        if (!conn) {
            printf("MySQL 초기화 실패\n");
            return nullptr;
        }

        // 연결 타임아웃 설정
        unsigned int timeout = 10;
        mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
        mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

        // 자동 재연결 설정
        bool reconnect = true;
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

        if (!mysql_real_connect(conn, host.c_str(), user.c_str(),
            password.c_str(), database.c_str(),
            port, NULL, 0)) {
            printf("MySQL 연결 실패: %s\n", mysql_error(conn));
            mysql_close(conn);
            return nullptr;
        }

        // 필수 설정들
        mysql_query(conn, "SET autocommit = 1");
        mysql_query(conn, "SET SESSION transaction_isolation = 'READ-COMMITTED'");
        mysql_query(conn, "SET SESSION wait_timeout = 28800");
        mysql_query(conn, "SET NAMES utf8");

        return conn;
    }

    bool reconnect(MYSQL* conn) {
        mysql_close(conn);

        MYSQL* newConn = mysql_init(NULL);
        if (!newConn) return false;

        // 동일한 설정으로 재연결
        unsigned int timeout = 10;
        mysql_options(newConn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        mysql_options(newConn, MYSQL_OPT_READ_TIMEOUT, &timeout);
        mysql_options(newConn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

        bool reconnect = true;
        mysql_options(newConn, MYSQL_OPT_RECONNECT, &reconnect);

        if (!mysql_real_connect(newConn, host.c_str(), user.c_str(),
            password.c_str(), database.c_str(),
            port, NULL, 0)) {
            mysql_close(newConn);
            return false;
        }

        // 기존 포인터 재사용
        memcpy(conn, newConn, sizeof(MYSQL));
        free(newConn);

        // 필수 설정 재적용
        mysql_query(conn, "SET autocommit = 1");
        mysql_query(conn, "SET SESSION transaction_isolation = 'READ-COMMITTED'");
        mysql_query(conn, "SET SESSION wait_timeout = 28800");
        mysql_query(conn, "SET NAMES utf8");

        return true;
    }
};

// 전역 연결 풀
std::unique_ptr<MySQLConnectionPool> g_connectionPool;
std::atomic<int> g_activeClients{ 0 };

// 안전한 문자열 이스케이프 함수
std::string SafeEscapeString(MYSQL* conn, const std::string& str) {
    if (str.empty()) return "";

    // 충분한 버퍼 크기 할당
    std::vector<char> buffer(str.length() * 2 + 1);
    unsigned long escapedLen = mysql_real_escape_string(
        conn, buffer.data(), str.c_str(), str.length()
    );

    return std::string(buffer.data(), escapedLen);
}

// 안전한 JSON 값 추출 함수
std::string SafeExtractValue(const std::string& jsonData, const std::string& key, bool isString = false) {
    std::string search = "\"" + key + "\": ";
    size_t pos = jsonData.find(search);
    if (pos == std::string::npos) return isString ? "" : "0";

    pos += search.length();
    if (pos >= jsonData.length()) return isString ? "" : "0";

    if (isString) {
        if (jsonData[pos] == '"') {
            pos++;
            if (pos >= jsonData.length()) return "";
        }
        size_t end = jsonData.find("\"", pos);
        if (end != std::string::npos && end > pos) {
            return jsonData.substr(pos, end - pos);
        }
        return "";
    }
    else {
        size_t end = pos;
        while (end < jsonData.length() &&
            (std::isdigit(jsonData[end]) || jsonData[end] == '.' || jsonData[end] == '-')) {
            end++;
        }
        if (end > pos) {
            return jsonData.substr(pos, end - pos);
        }
        return "0";
    }
}

std::string ProcessLogin(MYSQL* conn, const std::string& username, const std::string& password, bool& success) {
    success = false;

    if (!conn) {
        printf("로그인 처리 실패: MySQL 연결 없음\n");
        return "";
    }

    // 사용자 인증 - Prepared Statement 사용
    const char* query = "SELECT id, password FROM Users WHERE username = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        printf("Statement 초기화 실패\n");
        return "";
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("Statement prepare 실패: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return "";
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)username.c_str();
    bind[0].buffer_length = username.length();

    if (mysql_stmt_bind_param(stmt, bind)) {
        printf("Parameter bind 실패\n");
        mysql_stmt_close(stmt);
        return "";
    }

    if (mysql_stmt_execute(stmt)) {
        printf("Statement 실행 실패\n");
        mysql_stmt_close(stmt);
        return "";
    }

    // 결과 처리
    int user_id = 0;
    char db_password[256] = { 0 };
    unsigned long password_length = 0;

    MYSQL_BIND result[2];
    memset(result, 0, sizeof(result));

    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &user_id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = db_password;
    result[1].buffer_length = sizeof(db_password);
    result[1].length = &password_length;

    if (mysql_stmt_bind_result(stmt, result)) {
        printf("Result bind 실패\n");
        mysql_stmt_close(stmt);
        return "";
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        printf("로그인 실패: 사용자 '%s' 없음\n", username.c_str());
        mysql_stmt_close(stmt);
        return "";
    }

    mysql_stmt_close(stmt);

    // 비밀번호 확인
    if (password != std::string(db_password)) {
        printf("로그인 실패: 잘못된 비밀번호 (사용자: %s)\n", username.c_str());
        return "";
    }

    // 캐릭터 데이터 로드 (기존 방식 유지, 하지만 안전하게)
    std::string escaped_id = std::to_string(user_id);
    std::string character_query =
        "SELECT id, character_name, player_class, level, max_health, max_stamina, max_knowledge, gold, "
        "current_health, current_stamina, current_knowledge "
        "FROM Characters WHERE user_id = " + escaped_id + " LIMIT 1";

    if (mysql_query(conn, character_query.c_str())) {
        printf("캐릭터 데이터 로드 실패: %s\n", mysql_error(conn));
        return "";
    }

    MYSQL_RES* char_result = mysql_store_result(conn);
    if (!char_result) {
        printf("캐릭터 결과 저장 실패: %s\n", mysql_error(conn));
        return "";
    }

    MYSQL_ROW char_row = mysql_fetch_row(char_result);
    if (!char_row) {
        mysql_free_result(char_result);
        printf("캐릭터 데이터 없음 (사용자: %s)\n", username.c_str());
        return "";
    }

    // JSON 구성
    std::stringstream json;
    json << "{";
    json << "\"character_id\":" << (char_row[0] ? char_row[0] : "0") << ",";
    json << "\"character_name\":\"" << (char_row[1] ? char_row[1] : "") << "\",";
    json << "\"player_class\":" << (char_row[2] ? char_row[2] : "0") << ",";
    json << "\"level\":" << (char_row[3] ? char_row[3] : "1") << ",";
    json << "\"max_health\":" << (char_row[4] ? char_row[4] : "100") << ",";
    json << "\"max_stamina\":" << (char_row[5] ? char_row[5] : "100") << ",";
    json << "\"max_knowledge\":" << (char_row[6] ? char_row[6] : "100") << ",";
    json << "\"gold\":" << (char_row[7] ? char_row[7] : "0") << ",";
    json << "\"current_health\":" << (char_row[8] ? char_row[8] : char_row[4]) << ",";
    json << "\"current_stamina\":" << (char_row[9] ? char_row[9] : char_row[5]) << ",";
    json << "\"current_knowledge\":" << (char_row[10] ? char_row[10] : char_row[6]);

    int character_id = atoi(char_row[0]);
    mysql_free_result(char_result);

    // 인벤토리 아이템 로드
    std::string inventory_query =
        "SELECT slot_index, item_class, item_name, item_type, grade, count, potion_effect "
        "FROM inventory_items WHERE character_id = " + std::to_string(character_id);

    if (mysql_query(conn, inventory_query.c_str()) == 0) {
        MYSQL_RES* inv_result = mysql_store_result(conn);
        if (inv_result) {
            json << ",\"inventory_items\":[";
            bool first = true;
            MYSQL_ROW inv_row;
            while ((inv_row = mysql_fetch_row(inv_result))) {
                if (!first) json << ",";
                json << "{";
                json << "\"slot_index\":" << (inv_row[0] ? inv_row[0] : "0") << ",";
                json << "\"item_class\":\"" << (inv_row[1] ? inv_row[1] : "") << "\",";
                json << "\"item_name\":\"" << (inv_row[2] ? inv_row[2] : "") << "\",";
                json << "\"item_type\":" << (inv_row[3] ? inv_row[3] : "0") << ",";
                json << "\"grade\":" << (inv_row[4] ? inv_row[4] : "0") << ",";
                json << "\"count\":" << (inv_row[5] ? inv_row[5] : "0") << ",";
                json << "\"potion_effect\":" << (inv_row[6] ? inv_row[6] : "0");
                json << "}";
                first = false;
            }
            json << "]";
            mysql_free_result(inv_result);
        }
    }

    json << "}";
    success = true;

    printf("로그인 성공: %s (Thread: %llu)\n", username.c_str(),
        std::hash<std::thread::id>{}(std::this_thread::get_id()));

    return json.str();
}

bool ProcessSaveData(MYSQL* conn, const std::string& jsonData) {
    if (!conn) {
        printf("데이터 저장 실패: MySQL 연결 없음\n");
        return false;
    }

    printf("데이터 저장 요청 받음 (Thread: %llu)\n",
        std::hash<std::thread::id>{}(std::this_thread::get_id()));

    // 트랜잭션 시작
    if (mysql_query(conn, "START TRANSACTION")) {
        printf("트랜잭션 시작 실패: %s\n", mysql_error(conn));
        return false;
    }

    // 캐릭터 정보 추출 (안전한 함수 사용)
    std::string character_id_str = SafeExtractValue(jsonData, "character_id");
    int character_id = 0;
    try {
        character_id = std::stoi(character_id_str);
    }
    catch (...) {
        printf("잘못된 character_id: %s\n", character_id_str.c_str());
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    std::string character_name = SafeExtractValue(jsonData, "character_name", true);
    std::string player_class = SafeExtractValue(jsonData, "player_class");
    std::string level = SafeExtractValue(jsonData, "level");
    std::string max_health = SafeExtractValue(jsonData, "max_health");
    std::string max_stamina = SafeExtractValue(jsonData, "max_stamina");
    std::string max_knowledge = SafeExtractValue(jsonData, "max_knowledge");
    std::string gold = SafeExtractValue(jsonData, "gold");

    // 안전한 이스케이프
    character_name = SafeEscapeString(conn, character_name);

    // Characters 테이블 업데이트
    std::string update_character_query =
        "UPDATE Characters SET "
        "character_name = '" + character_name + "', "
        "player_class = " + player_class + ", "
        "level = " + level + ", "
        "max_health = " + max_health + ", "
        "max_stamina = " + max_stamina + ", "
        "max_knowledge = " + max_knowledge + ", "
        "gold = " + gold + ", "
        "current_health = " + max_health + ", "
        "current_stamina = " + max_stamina + ", "
        "current_knowledge = " + max_knowledge + " "
        "WHERE id = " + std::to_string(character_id);

    if (mysql_query(conn, update_character_query.c_str())) {
        printf("캐릭터 업데이트 실패: %s\n", mysql_error(conn));
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    // 기존 아이템 삭제
    std::string delete_queries[] = {
        "DELETE FROM inventory_items WHERE character_id = " + std::to_string(character_id),
        "DELETE FROM equipment_items WHERE character_id = " + std::to_string(character_id),
        "DELETE FROM storage_items WHERE character_id = " + std::to_string(character_id)
    };

    for (const auto& query : delete_queries) {
        if (mysql_query(conn, query.c_str())) {
            printf("기존 데이터 삭제 실패: %s\n", mysql_error(conn));
            mysql_query(conn, "ROLLBACK");
            return false;
        }
    }

    // 아이템 저장 함수 (람다 내부에서도 안전한 함수 사용)
    auto saveItemsFromArray = [&](const std::string& arrayName, const std::string& tableName) -> bool {
        std::string searchPattern = "\"" + arrayName + "\": [";
        size_t arrayStart = jsonData.find(searchPattern);
        if (arrayStart == std::string::npos) {
            return true; // 배열이 없는 것은 오류가 아님
        }

        arrayStart += searchPattern.length();
        if (arrayStart >= jsonData.length()) return true;

        // 배열 끝 위치 찾기
        size_t arrayEnd = arrayStart;
        int bracketCount = 1;
        while (arrayEnd < jsonData.length() && bracketCount > 0) {
            if (jsonData[arrayEnd] == '[') bracketCount++;
            else if (jsonData[arrayEnd] == ']') bracketCount--;
            arrayEnd++;
        }

        if (bracketCount != 0 || arrayEnd == arrayStart) {
            return true; // 빈 배열
        }

        std::string arrayContent = jsonData.substr(arrayStart, arrayEnd - arrayStart - 1);

        // 각 아이템 처리
        size_t pos = 0;
        while (pos < arrayContent.length()) {
            size_t objStart = arrayContent.find("{", pos);
            if (objStart == std::string::npos) break;

            size_t objEnd = objStart + 1;
            int braceCount = 1;
            while (objEnd < arrayContent.length() && braceCount > 0) {
                if (arrayContent[objEnd] == '{') braceCount++;
                else if (arrayContent[objEnd] == '}') braceCount--;
                objEnd++;
            }

            if (braceCount != 0) break;

            std::string itemJson = arrayContent.substr(objStart, objEnd - objStart);

            // 안전한 필드 추출
            std::string slot_index = SafeExtractValue(itemJson, "slot_index");
            std::string item_class = SafeExtractValue(itemJson, "item_class", true);
            std::string item_name = SafeExtractValue(itemJson, "item_name", true);
            std::string item_type = SafeExtractValue(itemJson, "item_type");
            std::string grade = SafeExtractValue(itemJson, "grade");
            std::string count = SafeExtractValue(itemJson, "count");
            std::string potion_effect = SafeExtractValue(itemJson, "potion_effect");

            // SQL 인젝션 방지
            item_class = SafeEscapeString(conn, item_class);
            item_name = SafeEscapeString(conn, item_name);

            std::string insertQuery =
                "INSERT INTO " + tableName + " "
                "(character_id, slot_index, item_class, item_name, item_type, grade, count, potion_effect) "
                "VALUES (" +
                std::to_string(character_id) + ", " +
                slot_index + ", '" +
                item_class + "', '" +
                item_name + "', " +
                item_type + ", " +
                grade + ", " +
                count + ", " +
                potion_effect + ")";

            if (mysql_query(conn, insertQuery.c_str())) {
                printf("%s 아이템 삽입 실패: %s\n", tableName.c_str(), mysql_error(conn));
                return false;
            }

            pos = objEnd;
        }

        return true;
        };

    // 각 아이템 타입별 저장
    if (!saveItemsFromArray("inventory_items", "inventory_items") ||
        !saveItemsFromArray("equipment_items", "equipment_items") ||
        !saveItemsFromArray("storage_items", "storage_items")) {
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    // 커밋
    if (mysql_query(conn, "COMMIT")) {
        printf("커밋 실패: %s\n", mysql_error(conn));
        mysql_query(conn, "ROLLBACK");
        return false;
    }

    printf("모든 데이터 저장 및 커밋 완료! (Thread: %llu)\n",
        std::hash<std::thread::id>{}(std::this_thread::get_id()));

    return true;
}

bool ProcessRegister(MYSQL* conn, const std::string& username, const std::string& password) {
    if (!conn) {
        printf("회원가입 처리 실패: MySQL 연결 없음\n");
        return false;
    }

    // 안전한 username 이스케이프
    std::string escaped_username = SafeEscapeString(conn, username);
    std::string escaped_password = SafeEscapeString(conn, password);

    // 중복 확인
    std::string check_query = "SELECT id FROM Users WHERE username = '" + escaped_username + "'";

    if (mysql_query(conn, check_query.c_str())) {
        printf("중복 확인 쿼리 실패: %s\n", mysql_error(conn));
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        printf("결과 저장 실패: %s\n", mysql_error(conn));
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
        escaped_username + "', '" + escaped_password + "')";

    if (mysql_query(conn, insert_query.c_str())) {
        printf("사용자 등록 실패: %s\n", mysql_error(conn));
        return false;
    }

    // 새로 생성된 사용자 ID 가져오기
    int user_id = static_cast<int>(mysql_insert_id(conn));

    // 기본 캐릭터 생성
    std::string create_character_query =
        "INSERT INTO Characters (user_id, character_name, player_class, gold) VALUES (" +
        std::to_string(user_id) + ", '" + escaped_username + "_Character', 0, 1000)";

    if (mysql_query(conn, create_character_query.c_str())) {
        printf("기본 캐릭터 생성 실패: %s\n", mysql_error(conn));
        return false;
    }

    printf("회원가입 성공: %s (기본 캐릭터 생성됨)\n", username.c_str());
    return true;
}

// 클라이언트 처리 함수
void HandleClient(SOCKET clientSocket, int clientId) {
    g_activeClients++;
    printf("[클라이언트 %d] 연결됨 (활성 클라이언트: %d)\n", clientId, g_activeClients.load());

    // 이 스레드용 MySQL 연결 획득
    MYSQL* mysql_conn = g_connectionPool->getConnection();
    if (!mysql_conn) {
        printf("[클라이언트 %d] MySQL 연결 획득 실패\n", clientId);
        closesocket(clientSocket);
        g_activeClients--;
        return;
    }

    const int BUF = 4096;
    std::vector<uint8_t> buf(BUF);

    try {
        while (true) {
            int received = recv(clientSocket, (char*)buf.data(), BUF, 0);
            if (received <= 0) {
                printf("[클라이언트 %d] 연결 종료 (recv: %d)\n", clientId, received);
                break;
            }

            printf("[클라이언트 %d] 요청 받음 (%d bytes)\n", clientId, received);

            // 프로토콜 파싱
            if (received < 1) continue;

            uint8_t req = buf[0];
            int idx = 1;

            if (req == SaveDataReq) {
                // 데이터 저장 요청 처리
                printf("[클라이언트 %d] 데이터 저장 요청\n", clientId);

                uint32_t dataSize = 0;
                if (received >= 5) {
                    dataSize |= static_cast<uint32_t>(buf[1]);
                    dataSize |= static_cast<uint32_t>(buf[2]) << 8;
                    dataSize |= static_cast<uint32_t>(buf[3]) << 16;
                    dataSize |= static_cast<uint32_t>(buf[4]) << 24;
                    idx = 5;
                }

                if (dataSize > 0 && dataSize < BUF && received >= 5 + static_cast<int>(dataSize)) {
                    std::string jsonData((char*)&buf[idx], dataSize);

                    bool saveSuccess = ProcessSaveData(mysql_conn, jsonData);

                    uint8_t respData[2];
                    respData[0] = SaveDataResponse;
                    respData[1] = saveSuccess ? 1 : 0;

                    int sent = send(clientSocket, (char*)&respData, 2, 0);
                    printf("[클라이언트 %d] 데이터 저장 응답: %s (%d bytes)\n",
                        clientId, saveSuccess ? "성공" : "실패", sent);
                }
                continue;
            }

            // 로그인/회원가입 요청 처리
            if (received < 5) continue; // 최소 크기 체크

            auto readLen = [&](uint16_t& L) -> bool {
                if (idx + 2 > received) return false;
                L = buf[idx] | (buf[idx + 1] << 8);
                idx += 2;
                return true;
                };

            uint16_t uLen = 0, pLen = 0;
            if (!readLen(uLen)) continue;

            if (idx + uLen > received) continue;
            std::string user((char*)&buf[idx], uLen);
            idx += uLen;

            if (!readLen(pLen)) continue;

            if (idx + pLen > received) continue;
            std::string pass((char*)&buf[idx], pLen);

            // 요청 처리
            if (req == RegisterReq) {
                printf("[클라이언트 %d] 회원가입 요청: %s\n", clientId, user.c_str());

                bool ok = ProcessRegister(mysql_conn, user, pass);

                uint8_t resp = ok ? 1 : 0;
                int sent = send(clientSocket, (char*)&resp, 1, 0);
                printf("[클라이언트 %d] 회원가입 응답: %s (%d bytes)\n",
                    clientId, ok ? "성공" : "실패", sent);
            }
            else if (req == LoginReq) {
                printf("[클라이언트 %d] 로그인 요청: %s\n", clientId, user.c_str());

                bool success = false;
                std::string characterData = ProcessLogin(mysql_conn, user, pass, success);

                if (success && !characterData.empty()) {
                    // 로그인 성공: [응답타입][데이터크기][JSON데이터]
                    uint8_t respType = CharacterDataResponse;
                    uint32_t dataLen = static_cast<uint32_t>(characterData.length());

                    std::vector<uint8_t> response;
                    response.push_back(respType);

                    // 4바이트: 데이터 크기 (리틀 엔디언)
                    response.push_back((dataLen >> 0) & 0xFF);
                    response.push_back((dataLen >> 8) & 0xFF);
                    response.push_back((dataLen >> 16) & 0xFF);
                    response.push_back((dataLen >> 24) & 0xFF);

                    // JSON 데이터
                    response.insert(response.end(), characterData.begin(), characterData.end());

                    int sent = send(clientSocket, (char*)response.data(),
                        static_cast<int>(response.size()), 0);
                    printf("[클라이언트 %d] 로그인 성공, 캐릭터 데이터 전송 (%d bytes)\n",
                        clientId, sent);
                }
                else {
                    // 로그인 실패
                    uint8_t resp = 0;
                    int sent = send(clientSocket, (char*)&resp, 1, 0);
                    printf("[클라이언트 %d] 로그인 실패 (%d bytes)\n", clientId, sent);
                }
            }
        }
    }
    catch (const std::exception& e) {
        printf("[클라이언트 %d] 예외 발생: %s\n", clientId, e.what());
    }
    catch (...) {
        printf("[클라이언트 %d] 알 수 없는 예외 발생\n", clientId);
    }

    // 정리
    g_connectionPool->releaseConnection();
    closesocket(clientSocket);
    g_activeClients--;
    printf("[클라이언트 %d] 연결 해제 완료 (활성 클라이언트: %d)\n",
        clientId, g_activeClients.load());
}

int main() {
    printf("=== 개선된 멀티스레드 MySQL Echo Server 시작 ===\n");

    // MySQL 연결 풀 초기화
    g_connectionPool = std::make_unique<MySQLConnectionPool>(
        "localhost",          // host
        "root",              // user
        "2022180049!gksrnr", // password
        "game_db",           // database
        3306                 // port
    );

    // 초기 연결 테스트
    MYSQL* testConn = g_connectionPool->getConnection();
    if (!testConn) {
        printf("MySQL 연결 풀 초기화 실패\n");
        system("pause");
        return 1;
    }
    g_connectionPool->releaseConnection();

    // Winsock 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup 실패\n");
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        printf("소켓 생성 실패\n");
        WSACleanup();
        return 1;
    }

    // SO_REUSEADDR 설정
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // 네이글 알고리즘 비활성화 (응답 속도 개선)
    setsockopt(listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(3000);

    if (bind(listenSock, (sockaddr*)&service, sizeof(service)) == SOCKET_ERROR) {
        printf("bind 실패: %d\n", WSAGetLastError());
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen 실패: %d\n", WSAGetLastError());
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    printf("서버 시작 - 포트 3000에서 캐릭터 데이터 서비스 대기 중...\n");
    printf("Press Ctrl+C to stop the server\n\n");

    int clientIdCounter = 1;
    std::vector<std::thread> clientThreads;

    // 메인 루프
    while (true) {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET client = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);

        if (client == INVALID_SOCKET) {
            int error = WSAGetLastError();
            if (error == WSAEINTR) {
                printf("서버 종료 신호 감지\n");
                break;
            }
            printf("Accept 오류: %d\n", error);
            Sleep(100);
            continue;
        }

        // 클라이언트 정보 출력
        char ipStr[INET_ADDRSTRLEN];
        InetNtopA(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
        printf("[메인] 새 클라이언트 연결: %s:%d (ID: %d)\n",
            ipStr, ntohs(clientAddr.sin_port), clientIdCounter);

        // 새 클라이언트를 별도 스레드에서 처리
        int currentClientId = clientIdCounter++;
        try {
            clientThreads.emplace_back(HandleClient, client, currentClientId);
            clientThreads.back().detach();
        }
        catch (const std::exception& e) {
            printf("[메인] 스레드 생성 실패: %s\n", e.what());
            closesocket(client);
        }
    }

    // 서버 종료 처리
    printf("\n서버 종료 중...\n");

    // 모든 활성 클라이언트가 종료될 때까지 대기
    int waitCount = 0;
    while (g_activeClients > 0 && waitCount < 100) { // 최대 10초 대기
        printf("활성 클라이언트 대기 중: %d\n", g_activeClients.load());
        Sleep(100);
        waitCount++;
    }

    // 정리
    closesocket(listenSock);
    WSACleanup();

    printf("서버 종료 완료\n");
    system("pause");
    return 0;
}