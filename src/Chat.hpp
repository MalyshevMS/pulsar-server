#pragma once

#include "lib/jsonlib.h"
#include <fstream>
#include <filesystem>
#include <iostream>

/*
    Server's chat structure (.chat):

                symbols num
    string num |     10           24             1024
             1 | (datetime)(sender username)(message text)
             2 | ...
           ... | ...
*/

class Chat {
private:
    std::string path;
    std::string path2;
    bool is_path2 = false;
public:
    Chat(const std::string& user1, const std::string& user2) {
        path  = PULSAR_CHATS_DIR"/users/" + user1.substr(1, std::string::npos) + user2.substr(1, std::string::npos) + ".chat";
        path2 = PULSAR_CHATS_DIR"/users/" + user2.substr(1, std::string::npos) + user1.substr(1, std::string::npos) + ".chat";
        is_path2 = true;
    }

    Chat(const std::string& channel) {
        path = PULSAR_CHATS_DIR"/channels/" + channel.substr(1, std::string::npos) + ".chat";
        is_path2 = false;
    }

    void write(time_t in_time, const std::string& in_src, const std::string& in_msg) {
        char time[PULSAR_DATE_SIZE + 1];
        char src[PULSAR_USERNAME_SIZE + 1];
        char msg[PULSAR_MESSAGE_SIZE + 1];

        std::string time_str = std::to_string(in_time);
        if (time_str.size() != PULSAR_DATE_SIZE) throw std::runtime_error("Given time has incorrect format!");
        for (int i = 0; i < PULSAR_DATE_SIZE; i++) {
            time[i] = time_str[i];
        }
        time[PULSAR_DATE_SIZE] = '\000';

        for (int i = 0; i < PULSAR_USERNAME_SIZE; i++) {
            if (i < in_src.size()) src[i] = in_src[i];
            else src[i] = '\x03';
        }
        src[PULSAR_USERNAME_SIZE] = '\000';

        for (int i = 0; i < PULSAR_MESSAGE_SIZE; i++) {
            if (i < in_msg.size()) msg[i] = in_msg[i];
            else msg[i] = '\x03';
        }
        msg[PULSAR_MESSAGE_SIZE] = '\000';

        std::ofstream file(path, std::ios::app);
        file << time << src << msg << '\n';
        file.close();

        if (is_path2) {
            std::ofstream file2(path2, std::ios::app);
            file2 << time << src << msg << '\n';
            file2.close();
        }
    }

    std::vector<std::string> read(size_t lines_count) {
        std::ifstream file(path, std::ios::ate);
        if (!file) return {};

        std::streampos size = file.tellg();
        std::vector<std::string> lines;
        std::string line;
        size_t count = 0;
        std::string buffer;
        char ch;

        for (std::streamoff i = 1; i <= size && count < lines_count; i++) {
            file.seekg(-i, std::ios::end);
            file.get(ch);
            if (ch == '\n' && !buffer.empty()) {
                std::reverse(buffer.begin(), buffer.end());
                lines.push_back(buffer);
                buffer.clear();
                count++;
            } else {
                buffer.push_back(ch);
            }
        }
        if (!buffer.empty()) {
            std::reverse(buffer.begin(), buffer.end());
            lines.push_back(buffer);
        }

        std::reverse(lines.begin(), lines.end());
        return lines;
    }
};