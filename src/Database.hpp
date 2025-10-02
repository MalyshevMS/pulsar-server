#pragma once

#include "lib/jsonlib.h"
#include <fstream>
#include <filesystem>
#include <iostream>

/*
    Server's Database structure (JSON):
    {
        "type": "database",
        "users": {
            "@user": {
                "password": (none/string),
                "name": "user's real name (string)",
                "channels": [array with channels names],
                "contacts": {
                    "@username": "contact name (string)"
                },
                "email": "user's email (string)",
                "status": "active/inactive/banned (string)"
            }
        },

        "channels": {
            ":channel" {
                "name": "channel's name (string)",
                "members": [array with usernames],
                "admins": [array with usernames],
                "status": "active/inactive/banned (string)"
            }
        }
    }
*/

class Database {
private:
    Json db;
    std::string path;
public:
    Database(const std::string& path) {
        if (std::filesystem::exists(path)) {
            std::ifstream f(path);
            f >> db;
            f.close();
            this->path = path;
        } else throw std::runtime_error("Database not exists!");

        db["type"] = "database";
        db["users"] = Json::object();
        db["channels"] = Json::object();
    }

    std::string toString() {
        return jsonToString(db);
    }

    void save() {
        std::ofstream f(path);
        f << db;
        f.close();
        std::cout << "Database was saved to " << path << std::endl;
    }

    void add_user(const std::string& username) {
        if (db["users"].contains(username)) return;

        db["users"][username]["password"] = Json();
        db["users"][username]["name"] = username;
        db["users"][username]["channels"] = Json::array();
        db["users"][username]["contacts"] = Json::object();
        db["users"][username]["email"] = "EMAIL";
        db["users"][username]["status"] = "active";
    }

    void add_channel(const std::string& channelname) {
        if (db["channels"].contains(channelname)) return;

        db["channels"][channelname]["name"] = channelname;
        db["channels"][channelname]["members"] = Json::array();
        db["channels"][channelname]["admins"] = Json::array();
        db["channels"][channelname]["status"] = "active";
    }

    Json user(const std::string& username) {
        if (!db["users"].contains(username)) return Json({{"error", "user is not exists"}});

        return db["users"][username];
    }

    Json channel(const std::string& channelname) {
        if (!db["channels"].contains(channelname)) return Json({{"error", "channel is not exists"}});

        return db["channels"][channelname];
    }

    bool contains_user(const std::string& username) { return db["users"].contains(username); }
    bool contains_channel(const std::string& channelname) { return db["channels"].contains(channelname); }

    /// @param password hashed password string
    void set_password(const std::string& username, const std::string& password) {
        if (!db["users"].contains(username)) return;

        if (password == "") db["users"][username]["password"] = Json();
        else db["users"][username]["password"] = password;
    }

    /// @param password hashed password string
    bool login(const std::string& username, const std::string& password) {
        if (!db["users"].contains(username)) return false;

        if (db["users"][username]["password"] == Json()) return true;
        return std::string(db["users"][username]["password"]) == password;
    }
};