#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include "lib/jsonlib.h"
#include "lib/hash.h"
#include "Database.hpp"
#include "Datetime.hpp"

#pragma GCC diagnostic ignored "-Wunused-result"

std::vector<sf::TcpSocket*> clients;
std::mutex clientsMutex;

Database db("res/database.json");

#define PULSAR_PORT 4171

void sendAll(const std::string& message) {
    for (auto& client : clients) {
        sf::Packet sendPacket;
        sendPacket << message;
        client->send(sendPacket);
    }
}

void sendAllExcept(const std::string& message, sf::TcpSocket* exception) {
    for (auto& client : clients) {
        if (client != exception) {
            sf::Packet sendPacket;
            sendPacket << message;
            client->send(sendPacket);
        }
    }
}

void sendTo(const std::string& message, sf::TcpSocket* dest) {
    sf::Packet sendPacket;
    sendPacket << message;
    dest->send(sendPacket);
}

std::string parseRequest(const std::string& req) {
    if (req.substr(0, 3) == "!db") {
        db.save();

        if (req.substr(4, 4) == "user") {
            return jsonToString(db.user(req.substr(9, std::string::npos)));
        }
        else if (req.substr(4, 7) == "channel") {
            return jsonToString(db.channel(req.substr(12, std::string::npos)));
        }

        else return "Invalid database request!";
    }

    else if (req.substr(0, 6) == "!login") {
        auto json = Json::parse(req.substr(7, std::string::npos));
        auto login = std::string(json[0]);
        auto password = std::string(json[1]);
        if (!db.contains_user(login))
            return "login fail_username";
        if (db.login(login, password))
            return "login success";
        else
            return "login fail_password";
    }

    else return "Invalid request!";
}

void handleClient(sf::TcpSocket* clientSocket) {
    std::cout << "Client connected: " << *clientSocket->getRemoteAddress() << std::endl;
    
    while (true) {
        sf::Packet packet;
        if (clientSocket->receive(packet) == sf::Socket::Status::Done) {
            std::string message;
            packet >> message;
            
            std::cout << "Received: " << message << std::endl;
            auto json = Json::parse(message);
            
            if (std::string(json["dst"]) == "!server") {
                auto ans = Json({
                    {"type", "message"},
                    {"time", Datetime::now().toTime()},
                    {"src", "!server"},
                    {"dst", json["src"]},
                    {"msg", parseRequest(json["msg"])}
                });

                std::lock_guard<std::mutex> lock(clientsMutex);
                sendTo(jsonToString(ans), clientSocket);
                continue;
            }
            std::lock_guard<std::mutex> lock(clientsMutex);
            sendAllExcept(message, clientSocket);
        } else break;
    }
    
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    delete clientSocket;
    
    std::cout << "Client disconnected" << std::endl;
}

int main() {
    sf::TcpListener listener;
    
    if (listener.listen(PULSAR_PORT) != sf::Socket::Status::Done) {
        std::cout << "Error: Could not listen on port " << PULSAR_PORT << std::endl;
        return 1;
    }
    
    std::cout << "Server started on port " << PULSAR_PORT << std::endl;

    db.add_channel(":all");

    db.add_user("@test");
    db.add_channel(":ch");

    db.set_password("@test", hash("123"));
    
    while (true) {
        sf::TcpSocket* clientSocket = new sf::TcpSocket;
        
        if (listener.accept(*clientSocket) == sf::Socket::Status::Done) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
            
            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();
        } else {
            delete clientSocket;
        }
    }
    
    return 0;
}