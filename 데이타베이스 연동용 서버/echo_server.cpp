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
    RegisterReq = 1,
    SaveDataReq = 2  // ���� �߰�: ������ ���� ��û
};

enum ResponseType : uint8_t {
    SimpleResponse = 0,        // �⺻ 1����Ʈ ���� (ȸ������, �α��� ����)
    CharacterDataResponse = 1, // ĳ���� ������ ���� ���� (�α��� ����)
    SaveDataResponse = 2       // ������ ���� ����
};

MYSQL* mysql_conn = nullptr;
std::mutex mysql_mutex;

bool ConnectMySQL() {
    mysql_conn = mysql_init(NULL);
    if (!mysql_conn) {
        printf("MySQL �ʱ�ȭ ����\n");
        return false;
    }

    // ���� Ÿ�Ӿƿ� ����
    unsigned int timeout = 10;
    mysql_options(mysql_conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(mysql_conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(mysql_conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    // �ڵ� �翬�� ����
    bool reconnect = true;
    mysql_options(mysql_conn, MYSQL_OPT_RECONNECT, &reconnect);

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
        mysql_conn = nullptr;
        return false;
    }

    printf("MySQL ���� ����!\n");

    if (mysql_query(mysql_conn, "USE game_db")) {
        printf("�����ͺ��̽� ���� ����: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // Ʈ����� ���� ��ȭ
    if (mysql_query(mysql_conn, "SET autocommit = 1")) {
        printf("AutoCommit ���� ����: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // �ݸ� ���� ����
    if (mysql_query(mysql_conn, "SET SESSION transaction_isolation = 'READ-COMMITTED'")) {
        printf("�ݸ� ���� ���� ����: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // ���� Ÿ�Ӿƿ� ����
    if (mysql_query(mysql_conn, "SET SESSION wait_timeout = 28800")) {
        printf("Ÿ�Ӿƿ� ���� ����: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // ���ڼ� ����
    if (mysql_query(mysql_conn, "SET NAMES utf8")) {
        printf("���ڼ� ���� ����: %s\n", mysql_error(mysql_conn));
        mysql_close(mysql_conn);
        mysql_conn = nullptr;
        return false;
    }

    // ���� Ȯ��
    printf("MySQL ���� Ȯ�� ��...\n");

    // AutoCommit ���� Ȯ��
    if (mysql_query(mysql_conn, "SELECT @@autocommit")) {
        printf("AutoCommit ���� Ȯ�� ����: %s\n", mysql_error(mysql_conn));
    }
    else {
        MYSQL_RES* result = mysql_store_result(mysql_conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                printf("AutoCommit ����: %s\n", row[0]);
            }
            mysql_free_result(result);
        }
    }

    // ���� ID Ȯ��
    if (mysql_query(mysql_conn, "SELECT CONNECTION_ID()")) {
        printf("���� ID Ȯ�� ����: %s\n", mysql_error(mysql_conn));
    }
    else {
        MYSQL_RES* result = mysql_store_result(mysql_conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                printf("���� ID: %s\n", row[0]);
            }
            mysql_free_result(result);
        }
    }

    printf("game_db �����ͺ��̽� ��� �غ� (��� ���� �Ϸ�)\n");
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
        "SELECT id, character_name, player_class, level, max_health, max_stamina, max_knowledge, gold, "
        "current_health, current_stamina, current_knowledge "
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
    json << "\"gold\":" << char_row[7] << ",";
    json << "\"current_health\":" << (char_row[8] ? char_row[8] : char_row[4]) << ",";
    json << "\"current_stamina\":" << (char_row[9] ? char_row[9] : char_row[5]) << ",";
    json << "\"current_knowledge\":" << (char_row[10] ? char_row[10] : char_row[6]);
    //json << "}";

    int character_id = atoi(char_row[0]);

    mysql_free_result(char_result);

    // �κ��丮 ������ �ε�
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

    printf("�α��� ����: %s (ĳ����: %s, ����: %s, ���: %s)\n",
        username.c_str(), char_row[1], char_row[2], char_row[7]);

    return json.str();
}

bool ProcessSaveData(const std::string& jsonData) {
    std::lock_guard<std::mutex> lock(mysql_mutex);

    printf("������ ���� ��û ����: %s\n", jsonData.c_str());

    // ���� ���� Ȯ��
    if (!mysql_conn) {
        printf("MySQL ������ null�Դϴ�.\n");
        return false;
    }

    if (mysql_ping(mysql_conn)) {
        printf("MySQL ������ ���������ϴ�: %s\n", mysql_error(mysql_conn));
        // �翬�� �õ�
        mysql_close(mysql_conn);
        if (!ConnectMySQL()) {
            printf("MySQL �翬�� ����\n");
            return false;
        }
    }

    // ����� Ʈ����� ����
    if (mysql_query(mysql_conn, "START TRANSACTION")) {
        printf("Ʈ����� ���� ����: %s\n", mysql_error(mysql_conn));
        return false;
    }

    // �����: ���� ���� Ȯ��
    printf("Ʈ����� ���۵�, ������ ó�� ����\n");

    // �� ������ JSON �Ľ�
    auto extractValue = [&](const std::string& key, bool isString = false) -> std::string {
        std::string search = "\"" + key + "\": ";
        size_t pos = jsonData.find(search);
        if (pos == std::string::npos) return isString ? "" : "0";

        pos += search.length();
        if (isString) {
            if (jsonData[pos] == '"') pos++; // ���� " �ǳʶٱ�
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

    // ĳ���� ���� ����
    int character_id = std::stoi(extractValue("character_id"));
    std::string character_name = extractValue("character_name", true);
    std::string player_class = extractValue("player_class");
    std::string level = extractValue("level");
    std::string max_health = extractValue("max_health");
    std::string max_stamina = extractValue("max_stamina");
    std::string max_knowledge = extractValue("max_knowledge");
    std::string gold = extractValue("gold");

    printf("����� ������ - ID: %d, �̸�: %s, ����: %s, ���: %s\n",
        character_id, character_name.c_str(), player_class.c_str(), gold.c_str());

    // �� ������ ���ڿ� �̽�������
    auto escapeString = [&](const std::string& str) -> std::string {
        if (str.empty()) return "";

        char* escaped = new char[str.length() * 2 + 1];
        mysql_real_escape_string(mysql_conn, escaped, str.c_str(), str.length());
        std::string result(escaped);
        delete[] escaped;
        return result;
        };

    character_name = escapeString(character_name);

    // Characters ���̺� ������Ʈ
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
        printf("ĳ���� ������Ʈ ����: %s\n", mysql_error(mysql_conn));
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    printf("ĳ���� �⺻ ���� ������Ʈ �Ϸ� (������� ��: %lu)\n", mysql_affected_rows(mysql_conn));

    // ���� ������ ����
    std::string delete_queries[] = {
        "DELETE FROM inventory_items WHERE character_id = " + std::to_string(character_id),
        "DELETE FROM equipment_items WHERE character_id = " + std::to_string(character_id),
        "DELETE FROM storage_items WHERE character_id = " + std::to_string(character_id)
    };

    for (const auto& query : delete_queries) {
        if (mysql_query(mysql_conn, query.c_str())) {
            printf("���� ������ ���� ����: %s\n", mysql_error(mysql_conn));
            mysql_query(mysql_conn, "ROLLBACK");
            return false;
        }
    }

    printf("���� ������ ������ ���� �Ϸ�\n");

    // �ܼ�ȭ�� ������ ����
    // ������ ���� �Լ�
    auto saveItemsFromArray = [&](const std::string& arrayName, const std::string& tableName) -> bool {
        // �迭 ���� ��ġ ã��
        std::string searchPattern = "\"" + arrayName + "\": [";
        size_t arrayStart = jsonData.find(searchPattern);
        if (arrayStart == std::string::npos) {
            printf("%s �迭�� ã�� �� �����ϴ�\n", arrayName.c_str());
            return true; // �迭�� ���� ���� ������ �ƴ�
        }

        arrayStart += searchPattern.length();

        // �迭 �� ��ġ ã��
        size_t arrayEnd = arrayStart;
        int bracketCount = 1;
        while (arrayEnd < jsonData.length() && bracketCount > 0) {
            if (jsonData[arrayEnd] == '[') bracketCount++;
            else if (jsonData[arrayEnd] == ']') bracketCount--;
            arrayEnd++;
        }

        if (bracketCount != 0) {
            printf("%s �迭�� ���� ã�� �� �����ϴ�\n", arrayName.c_str());
            return false;
        }

        // �迭 ���� ����
        std::string arrayContent = jsonData.substr(arrayStart, arrayEnd - arrayStart - 1);

        // �� �迭 üũ
        bool isEmpty = true;
        for (char c : arrayContent) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                isEmpty = false;
                break;
            }
        }

        if (isEmpty) {
            printf("%s �迭�� �������\n", arrayName.c_str());
            return true;
        }

        // �� ������ ��ü �Ľ�
        size_t pos = 0;
        int itemCount = 0;

        while (pos < arrayContent.length()) {
            // ���� { ã��
            size_t objStart = arrayContent.find("{", pos);
            if (objStart == std::string::npos) break;

            // ��Ī�Ǵ� } ã��
            size_t objEnd = objStart + 1;
            int braceCount = 1;
            while (objEnd < arrayContent.length() && braceCount > 0) {
                if (arrayContent[objEnd] == '{') braceCount++;
                else if (arrayContent[objEnd] == '}') braceCount--;
                objEnd++;
            }

            if (braceCount != 0) {
                printf("������ ��ü�� ���� ã�� �� �����ϴ�\n");
                return false;
            }

            // ������ JSON ����
            std::string itemJson = arrayContent.substr(objStart, objEnd - objStart);

            // ������ �ʵ� ����
            auto getField = [&](const std::string& fieldName, bool isString = false) -> std::string {
                std::string pattern = "\"" + fieldName + "\": ";
                size_t fieldPos = itemJson.find(pattern);
                if (fieldPos == std::string::npos) return isString ? "" : "0";

                fieldPos += pattern.length();

                if (isString) {
                    if (itemJson[fieldPos] == '"') fieldPos++; // ���� " �ǳʶٱ�
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

            // �ʵ� ���� ����
            std::string slot_index = getField("slot_index");
            std::string item_class = getField("item_class", true);
            std::string item_name = getField("item_name", true);
            std::string item_type = getField("item_type");
            std::string grade = getField("grade");
            std::string count = getField("count");
            std::string potion_effect = getField("potion_effect");

            // SQL ������ ������ ���� �̽�������
            item_class = escapeString(item_class);
            item_name = escapeString(item_name);

            // INSERT ���� ����
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

            // ���� ����
            if (mysql_query(mysql_conn, insertQuery.c_str())) {
                printf("%s ������ ���� ����: %s\n", tableName.c_str(), mysql_error(mysql_conn));
                printf("������ ����: %s\n", insertQuery.c_str());
                return false;
            }

            itemCount++;
            printf("%s�� ������ ����: %s (���� %s)\n", tableName.c_str(), item_name.c_str(), slot_index.c_str());

            pos = objEnd;
        }

        printf("%s�� �� %d�� ������ ���� �Ϸ�\n", tableName.c_str(), itemCount);
        return true;
        };

    // �� ������ Ÿ�Ժ� ����
    if (!saveItemsFromArray("inventory_items", "inventory_items")) {
        printf("�κ��丮 ������ ���� ����\n");
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    if (!saveItemsFromArray("equipment_items", "equipment_items")) {
        printf("��� ������ ���� ����\n");
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    if (!saveItemsFromArray("storage_items", "storage_items")) {
        printf("â�� ������ ���� ����\n");
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    // ����� Ŀ��
    if (mysql_query(mysql_conn, "COMMIT")) {
        printf("Ŀ�� ����: %s\n", mysql_error(mysql_conn));
        mysql_query(mysql_conn, "ROLLBACK");
        return false;
    }

    printf("��� ������ ���� �� Ŀ�� �Ϸ�!\n");

    // ���� �� Ȯ��
    printf("���� �� Ȯ�� ���� ���� ��...\n");
    std::string verify_query = "SELECT gold FROM Characters WHERE id = " + std::to_string(character_id);
    if (mysql_query(mysql_conn, verify_query.c_str())) {
        printf("Ȯ�� ���� ����: %s\n", mysql_error(mysql_conn));
    }
    else {
        MYSQL_RES* result = mysql_store_result(mysql_conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            if (row) {
                printf("���� �� DB�� ��� ��: %s\n", row[0]);
            }
            mysql_free_result(result);
        }
    }

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

        if (req == SaveDataReq) {
            // ���� �߰�: ������ ���� ��û ó��
            printf("[Ŭ���̾�Ʈ %d] ������ ���� ��û\n", clientId);

            // ������ ũ�� �б� (4����Ʈ, ��Ʋ �����)
            uint32_t dataSize = 0;
            if (received >= 5) {
                dataSize |= static_cast<uint32_t>(buf[1]);
                dataSize |= static_cast<uint32_t>(buf[2]) << 8;
                dataSize |= static_cast<uint32_t>(buf[3]) << 16;
                dataSize |= static_cast<uint32_t>(buf[4]) << 24;
                idx = 5;
            }

            if (dataSize > 0 && received >= 5 + dataSize) {
                // JSON ������ ����
                std::string jsonData((char*)&buf[idx], dataSize);

                bool saveSuccess = ProcessSaveData(jsonData);

                // ���� ����
                // ���� Ÿ���� ��Ȯ�� ����
                uint8_t respData[2];
                respData[0] = 2; // SaveDataResponse Ÿ��
                respData[1] = saveSuccess ? 1 : 0;
                //uint8_t resp = saveSuccess ? 1 : 0;
                int sent = send(clientSocket, (char*)&respData, 2, 0);
                printf("[Ŭ���̾�Ʈ %d] ������ ���� ����: %s (%d bytes)\n",
                    clientId, saveSuccess ? "����" : "����", sent);
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

        // ��û ó��
        if (req == RegisterReq) {
            printf("[Ŭ���̾�Ʈ %d] ȸ������ ��û: %s\n", clientId, user.c_str());

            bool ok = ProcessRegister(user, pass);

            // ȸ�������� �⺻ ��� (1����Ʈ ����)
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
                // �α��� ����: �⺻ ��� (0)
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