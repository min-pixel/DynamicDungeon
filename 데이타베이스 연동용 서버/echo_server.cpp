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
    RegisterReq = 1,
    SaveDataReq = 2  // 새로 추가: 데이터 저장 요청
};

enum ResponseType : uint8_t {
    SimpleResponse = 0,        // 기본 1바이트 응답 (회원가입, 로그인 실패)
    CharacterDataResponse = 1, // 캐릭터 데이터 포함 응답 (로그인 성공)
    SaveDataResponse = 2       // 데이터 저장 응답
};

MYSQL* mysql_conn = nullptr;
std::mutex mysql_mutex;

bool ConnectMySQL() {
    mysql_conn = mysql_init(NULL);
    if (!mysql_conn) {
        printf("MySQL 초기화 실패\n");
        return false;
    }

    // 연결 타임아웃 설정
    unsigned int timeout = 10;
    mysql_options(mysql_conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(mysql_conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(mysql_conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    // 자동 재연결 설정
    bool reconnect = true;
    mysql_options(mysql_conn, MYSQL_OPT_RECONNECT, &reconnect);

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
        mysql_conn = nullptr;
        return false;
    }

    printf("MySQL 연결 성공!\n");

    if (mysql_query(mysql_conn, "USE game_db")) {
        printf("데이터베이스 선택 실패: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // 트랜잭션 설정 강화
    if (mysql_query(mysql_conn, "SET autocommit = 1")) {
        printf("AutoCommit 설정 실패: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // 격리 수준 설정
    if (mysql_query(mysql_conn, "SET SESSION transaction_isolation = 'READ-COMMITTED'")) {
        printf("격리 수준 설정 실패: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // 세션 타임아웃 설정
    if (mysql_query(mysql_conn, "SET SESSION wait_timeout = 28800")) {
        printf("타임아웃 설정 실패: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // 문자셋 설정
    if (mysql_query(mysql_conn, "SET NAMES utf8")) {
        printf("문자셋 설정 실패: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // 설정 확인
    printf("MySQL 설정 확인 중...\n");

    // AutoCommit 상태 확인
    if (mysql_query(mysql_conn, "SELECT @@autocommit")) {
        printf("AutoCommit 상태 확인 실패: %s\n", mysql_error(mysql_conn));
    }
    else {
        MYSQL_RES* result = mysql_store_result(mysql_conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                printf("AutoCommit 상태: %s\n", row[0]);
            }
            mysql_free_result(result);
        }
    }

    // 연결 ID 확인
    if (mysql_query(mysql_conn, "SELECT CONNECTION_ID()")) {
        printf("연결 ID 확인 실패: %s\n", mysql_error(mysql_conn));
    }
    else {
        MYSQL_RES* result = mysql_store_result(mysql_conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                printf("연결 ID: %s\n", row[0]);
            }
            mysql_free_result(result);
        }
    }

    printf("game_db 데이터베이스 사용 준비 (모든 설정 완료)\n");
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
        "SELECT id, character_name, player_class, level, max_health, max_stamina, max_knowledge, gold, "
        "current_health, current_stamina, current_knowledge "
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
    json << "\"gold\":" << char_row[7] << ",";
    json << "\"current_health\":" << (char_row[8] ? char_row[8] : char_row[4]) << ",";
    json << "\"current_stamina\":" << (char_row[9] ? char_row[9] : char_row[5]) << ",";
    json << "\"current_knowledge\":" << (char_row[10] ? char_row[10] : char_row[6]);
    //json << "}";

    int character_id = atoi(char_row[0]);

    mysql_free_result(char_result);

    // 인벤토리 아이템 로드
    std::string inventory_query =
        "SELECT slot_index, item_class, item_name, item_type, grade, count, potion_effect "
        "FROM inventory_items WHERE character_id = " + std::to_string(character_id);

    if (mysql_query(mysql_conn, inventory_query.c_str()) == 0) {
        MYSQL_RES* inv_result = mysql_store_result(mysql_conn);
        if (inv_result) {
            json << ",\"inventory_items\":[";
            bool first = true;
            MYSQL_ROW inv_row;
            while ((inv_row = mysql_fetch_row(inv_result))) {
                if (!first) json << ",";
                json << "{";
                json << "\"slot_index\":" << inv_row[0] << ",";
                json << "\"item_class\":\"" << inv_row[1] << "\",";
                json << "\"item_name\":\"" << inv_row[2] << "\",";
                json << "\"item_type\":" << inv_row[3] << ",";
                json << "\"grade\":" << inv_row[4] << ",";
                json << "\"count\":" << inv_row[5] << ",";
                json << "\"potion_effect\":" << inv_row[6];
                json << "}";
                first = false;
            }
            json << "]";
            mysql_free_result(inv_result);
        }
    }

    json << "}";
    success = true;

    printf("로그인 성공: %s (캐릭터: %s, 직업: %s, 골드: %s)\n",
        username.c_str(), char_row[1], char_row[2], char_row[7]);

    return json.str();
}

bool ProcessSaveData(const std::string& jsonData) {
    std::lock_guard<std::mutex> lock(mysql_mutex);

    printf("데이터 저장 요청 받음: %s\n", jsonData.c_str());

    // 연결 상태 확인
    if (!mysql_conn) {
        printf("MySQL 연결이 null입니다.\n");
        return false;
    }

    if (mysql_ping(mysql_conn)) {
        printf("MySQL 연결이 끊어졌습니다: %s\n", mysql_error(mysql_conn));
        // 재연결 시도
        mysql_close(mysql_conn);
        if (!ConnectMySQL()) {
            printf("MySQL 재연결 실패\n");
            return false;
        }
    }

    // 명시적 트랜잭션 시작
    if (mysql_query(mysql_conn, "START TRANSACTION")) {
        printf("트랜잭션 시작 실패: %s\n", mysql_error(mysql_conn));
        return false;
    }

    // 디버깅: 현재 상태 확인
    printf("트랜잭션 시작됨, 데이터 처리 시작\n");

    // 더 안전한 JSON 파싱
    auto extractValue = [&](const std::string& key, bool isString = false) -> std::string {
        std::string search = "\"" + key + "\": ";
        size_t pos = jsonData.find(search);
        if (pos == std::string::npos) return isString ? "" : "0";

        pos += search.length();
        if (isString) {
            if (jsonData[pos] == '"') pos++; // 시작 " 건너뛰기
            size_t end = jsonData.find("\"", pos);
            return (end != std::string::npos) ? jsonData.substr(pos, end - pos) : "";
        }
        else {
            size_t end = pos;
            while (end < jsonData.length() &&
                (std::isdigit(jsonData[end]) || jsonData[end] == '.' || jsonData[end] == '-')) {
                end++;
            }
            return jsonData.substr(pos, end - pos);
        }
        };

    // 캐릭터 정보 추출
    int character_id = std::stoi(extractValue("character_id"));
    std::string character_name = extractValue("character_name", true);
    std::string player_class = extractValue("player_class");
    std::string level = extractValue("level");
    std::string max_health = extractValue("max_health");
    std::string max_stamina = extractValue("max_stamina");
    std::string max_knowledge = extractValue("max_knowledge");
    std::string gold = extractValue("gold");

    printf("추출된 데이터 - ID: %d, 이름: %s, 직업: %s, 골드: %s\n",
        character_id, character_name.c_str(), player_class.c_str(), gold.c_str());

    // 더 안전한 문자열 이스케이핑
    auto escapeString = [&](const std::string& str) -> std::string {
        if (str.empty()) return "";

        char* escaped = new char[str.length() * 2 + 1];
        mysql_real_escape_string(mysql_conn, escaped, str.c_str(), str.length());
        std::string result(escaped);
        delete[] escaped;
        return result;
        };

    character_name = escapeString(character_name);

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

    if (mysql_query(mysql_conn, update_character_query.c_str())) {
        printf("캐릭터 업데이트 실패: %s\n", mysql_error(mysql_conn));
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    printf("캐릭터 기본 정보 업데이트 완료 (영향받은 행: %lu)\n", mysql_affected_rows(mysql_conn));

    // 기존 아이템 삭제
    std::string delete_queries[] = {
        "DELETE FROM inventory_items WHERE character_id = " + std::to_string(character_id),
        "DELETE FROM equipment_items WHERE character_id = " + std::to_string(character_id),
        "DELETE FROM storage_items WHERE character_id = " + std::to_string(character_id)
    };

    for (const auto& query : delete_queries) {
        if (mysql_query(mysql_conn, query.c_str())) {
            printf("기존 데이터 삭제 실패: %s\n", mysql_error(mysql_conn));
            mysql_query(mysql_conn, "ROLLBACK");
            return false;
        }
    }

    printf("기존 아이템 데이터 삭제 완료\n");

    // 단순화된 아이템 저장
    // 아이템 저장 함수
    auto saveItemsFromArray = [&](const std::string& arrayName, const std::string& tableName) -> bool {
        // 배열 시작 위치 찾기
        std::string searchPattern = "\"" + arrayName + "\": [";
        size_t arrayStart = jsonData.find(searchPattern);
        if (arrayStart == std::string::npos) {
            printf("%s 배열을 찾을 수 없습니다\n", arrayName.c_str());
            return true; // 배열이 없는 것은 오류가 아님
        }

        arrayStart += searchPattern.length();

        // 배열 끝 위치 찾기
        size_t arrayEnd = arrayStart;
        int bracketCount = 1;
        while (arrayEnd < jsonData.length() && bracketCount > 0) {
            if (jsonData[arrayEnd] == '[') bracketCount++;
            else if (jsonData[arrayEnd] == ']') bracketCount--;
            arrayEnd++;
        }

        if (bracketCount != 0) {
            printf("%s 배열의 끝을 찾을 수 없습니다\n", arrayName.c_str());
            return false;
        }

        // 배열 내용 추출
        std::string arrayContent = jsonData.substr(arrayStart, arrayEnd - arrayStart - 1);

        // 빈 배열 체크
        bool isEmpty = true;
        for (char c : arrayContent) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                isEmpty = false;
                break;
            }
        }

        if (isEmpty) {
            printf("%s 배열이 비어있음\n", arrayName.c_str());
            return true;
        }

        // 각 아이템 객체 파싱
        size_t pos = 0;
        int itemCount = 0;

        while (pos < arrayContent.length()) {
            // 다음 { 찾기
            size_t objStart = arrayContent.find("{", pos);
            if (objStart == std::string::npos) break;

            // 매칭되는 } 찾기
            size_t objEnd = objStart + 1;
            int braceCount = 1;
            while (objEnd < arrayContent.length() && braceCount > 0) {
                if (arrayContent[objEnd] == '{') braceCount++;
                else if (arrayContent[objEnd] == '}') braceCount--;
                objEnd++;
            }

            if (braceCount != 0) {
                printf("아이템 객체의 끝을 찾을 수 없습니다\n");
                return false;
            }

            // 아이템 JSON 추출
            std::string itemJson = arrayContent.substr(objStart, objEnd - objStart);

            // 아이템 필드 추출
            auto getField = [&](const std::string& fieldName, bool isString = false) -> std::string {
                std::string pattern = "\"" + fieldName + "\": ";
                size_t fieldPos = itemJson.find(pattern);
                if (fieldPos == std::string::npos) return isString ? "" : "0";

                fieldPos += pattern.length();

                if (isString) {
                    if (itemJson[fieldPos] == '"') fieldPos++; // 시작 " 건너뛰기
                    size_t endPos = itemJson.find("\"", fieldPos);
                    return (endPos != std::string::npos) ? itemJson.substr(fieldPos, endPos - fieldPos) : "";
                }
                else {
                    size_t endPos = fieldPos;
                    while (endPos < itemJson.length() &&
                        (std::isdigit(itemJson[endPos]) || itemJson[endPos] == '.' || itemJson[endPos] == '-')) {
                        endPos++;
                    }
                    return itemJson.substr(fieldPos, endPos - fieldPos);
                }
                };

            // 필드 값들 추출
            std::string slot_index = getField("slot_index");
            std::string item_class = getField("item_class", true);
            std::string item_name = getField("item_name", true);
            std::string item_type = getField("item_type");
            std::string grade = getField("grade");
            std::string count = getField("count");
            std::string potion_effect = getField("potion_effect");

            // SQL 인젝션 방지를 위한 이스케이핑
            item_class = escapeString(item_class);
            item_name = escapeString(item_name);

            // INSERT 쿼리 생성
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

            // 쿼리 실행
            if (mysql_query(mysql_conn, insertQuery.c_str())) {
                printf("%s 아이템 삽입 실패: %s\n", tableName.c_str(), mysql_error(mysql_conn));
                printf("실패한 쿼리: %s\n", insertQuery.c_str());
                return false;
            }

            itemCount++;
            printf("%s에 아이템 삽입: %s (슬롯 %s)\n", tableName.c_str(), item_name.c_str(), slot_index.c_str());

            pos = objEnd;
        }

        printf("%s에 총 %d개 아이템 저장 완료\n", tableName.c_str(), itemCount);
        return true;
        };

    // 각 아이템 타입별 저장
    if (!saveItemsFromArray("inventory_items", "inventory_items")) {
        printf("인벤토리 아이템 저장 실패\n");
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    if (!saveItemsFromArray("equipment_items", "equipment_items")) {
        printf("장비 아이템 저장 실패\n");
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    if (!saveItemsFromArray("storage_items", "storage_items")) {
        printf("창고 아이템 저장 실패\n");
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    // 명시적 커밋
    if (mysql_query(mysql_conn, "COMMIT")) {
        printf("커밋 실패: %s\n", mysql_error(mysql_conn));
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    printf("모든 데이터 저장 및 커밋 완료!\n");

    // 저장 후 확인
    printf("저장 후 확인 쿼리 실행 중...\n");
    std::string verify_query = "SELECT gold FROM Characters WHERE id = " + std::to_string(character_id);
    if (mysql_query(mysql_conn, verify_query.c_str())) {
        printf("확인 쿼리 실패: %s\n", mysql_error(mysql_conn));
    }
    else {
        MYSQL_RES* result = mysql_store_result(mysql_conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                printf("저장 후 DB의 골드 값: %s\n", row[0]);
            }
            mysql_free_result(result);
        }
    }

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

        if (req == SaveDataReq) {
            // 새로 추가: 데이터 저장 요청 처리
            printf("[클라이언트 %d] 데이터 저장 요청\n", clientId);

            // 데이터 크기 읽기 (4바이트, 리틀 엔디안)
            uint32_t dataSize = 0;
            if (received >= 5) {
                dataSize |= static_cast<uint32_t>(buf[1]);
                dataSize |= static_cast<uint32_t>(buf[2]) << 8;
                dataSize |= static_cast<uint32_t>(buf[3]) << 16;
                dataSize |= static_cast<uint32_t>(buf[4]) << 24;
                idx = 5;
            }

            if (dataSize > 0 && received >= 5 + dataSize) {
                // JSON 데이터 추출
                std::string jsonData((char*)&buf[idx], dataSize);

                bool saveSuccess = ProcessSaveData(jsonData);

                // 응답 전송
                // 응답 타입을 명확히 지정
                uint8_t respData[2];
                respData[0] = 2; // SaveDataResponse 타입
                respData[1] = saveSuccess ? 1 : 0;
                //uint8_t resp = saveSuccess ? 1 : 0;
                int sent = send(clientSocket, (char*)&respData, 2, 0);
                printf("[클라이언트 %d] 데이터 저장 응답: %s (%d bytes)\n",
                    clientId, saveSuccess ? "성공" : "실패", sent);
            }
            continue;
        }

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

            // 회원가입은 기본 방식 (1바이트 응답)
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
                // 로그인 실패: 기본 방식 (0)
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
    printf("=== 캐릭터 데이터 저장 MySQL Echo Server 시작 ===\n");

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