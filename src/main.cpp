#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

#pragma GCC diagnostic ignored "-Wunused-result"

std::vector<sf::TcpSocket*> clients;
std::mutex clientsMutex;

void handleClient(sf::TcpSocket* clientSocket) {
    std::cout << "Client connected: " << *clientSocket->getRemoteAddress() << std::endl;
    
    while (true) {
        sf::Packet packet;
        if (clientSocket->receive(packet) == sf::Socket::Status::Done) {
            std::string message;
            packet >> message;
            
            std::cout << "Received: " << message << std::endl;
            
            // Отправляем сообщение всем клиентам
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& client : clients) {
                if (client != clientSocket) {
                    sf::Packet sendPacket;
                    sendPacket << message;
                    client->send(sendPacket);
                }
            }
        } else {
            // Клиент отключился
            break;
        }
    }
    
    // Удаляем клиента из списка
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
    delete clientSocket;
    
    std::cout << "Client disconnected" << std::endl;
}

int main() {
    sf::TcpListener listener;
    
    // Запускаем прослушивание на порту 53000
    if (listener.listen(53000) != sf::Socket::Status::Done) {
        std::cout << "Error: Could not listen on port 53000" << std::endl;
        return 1;
    }
    
    std::cout << "Server started on port 53000" << std::endl;
    
    while (true) {
        sf::TcpSocket* clientSocket = new sf::TcpSocket;
        
        if (listener.accept(*clientSocket) == sf::Socket::Status::Done) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
            
            // Запускаем отдельный поток для обработки клиента
            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();
        } else {
            delete clientSocket;
        }
    }
    
    return 0;
}