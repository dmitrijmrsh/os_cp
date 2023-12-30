#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <sstream>
#include <signal.h>
#include <cassert>
#include <chrono>
#include <thread>
#include <exception>
#include <map>
#include <zmq.hpp>
#include <cstdlib>
#include <ctime>

#include "myMQ.hpp"
#include "player.hpp"
#include "game.hpp"

zmq::context_t context(3);
zmq::socket_t main_socket(context, ZMQ_REP);

int main() {
    zmq::socket_t  first_player_socket(context, ZMQ_REQ);
    zmq::socket_t second_player_socket(context, ZMQ_REQ);
    main_socket.bind("tcp://*:5555");
    first_player_socket.bind("tcp://*:5556");
    second_player_socket.bind("tcp://*:5557");
    std::cout << "Сервер начал работу" << std::endl;
    std::map<int, std::string> login_map;
    int port_iter = 1;
    int invite_applicants_count = 0;
    bool alive;
    pid_t first_player_pid, second_player_pid, first_applicator_pid, second_applicator_pid;
    std::vector<std::string> yes_no_vector;
    while (true) {
        std::string received_message = receive_message(main_socket);
        std::cout << "На сервер поступил запрос: '" + received_message + "'" << std::endl;
        if (port_iter > 2) {
            alive = try_recv(first_player_pid, second_player_pid);
            if (!alive) {
                kill(first_player_pid, SIGTERM);
                kill(second_player_pid, SIGTERM);
                std::cout << "Игра завершена из-за убийства одного или обоих клиентов" << std::endl;
                return 0;
            }
        }
        std::stringstream ss(received_message);
        std::string tmp;
        std::getline(ss, tmp, ':');
        if (tmp == "login") {
            if (port_iter > 2) {
                send_message(main_socket, "Error:TwoPlayersAlreadyExist");
            } else {
                std::getline(ss, tmp, ':');
				if (login_map[1] == tmp) {
					std::cout << "Игрок ввел занятое имя" << std::endl;
					send_message(main_socket, "Error:NameAlreadyExist");
				} else {
					std::cout << "Логин игрока номер " + std::to_string(port_iter) + ": " + tmp << std::endl;
					std::string login = tmp;
					login_map[port_iter] = login;
                    std::getline(ss, tmp, ':');
                    if (port_iter == 1) {
                        first_player_pid = pid_t(std::stoi(tmp));
                    } else {
                        second_player_pid = pid_t(std::stoi(tmp));
                    }
					
					send_message(main_socket, "Ok:" + std::to_string(port_iter));
					port_iter += 1;
				}
            }
        } else if (tmp == "yes" || tmp == "no") {
            std::string application = tmp;
            std::string reply_message;
            if (application == "yes" && invite_applicants_count == 0) {
                reply_message = "good";
                send_message(main_socket, reply_message);
                ++invite_applicants_count;
            } else {
                if (application == "yes" && invite_applicants_count > 0) {
                    reply_message = "bad";
                    send_message(main_socket, reply_message);
                }
            }
            if (application == "no" && invite_applicants_count == 0) {
                if (yes_no_vector.size() == 0) {
                    reply_message = "good";
                    send_message(main_socket, reply_message);
                } else {
                    reply_message = "bad";
                    send_message(main_socket, reply_message);
                }
            }
            if (application == "no" && invite_applicants_count == 1) {
                reply_message = "good";
                send_message(main_socket, reply_message);
            } 
            yes_no_vector.push_back(application);
            std::getline(ss, tmp, ':');
            if (yes_no_vector.size() == 1) {
                first_applicator_pid = pid_t(std::stoi(tmp));
            } else {
                second_applicator_pid = pid_t(std::stoi(tmp));
                if (yes_no_vector[0] == "yes" && yes_no_vector[1] == "no") {
                    if (first_applicator_pid == first_player_pid) {
                        send_message(first_player_socket, "move on");
                        receive_message(first_player_socket);
                    } else {
                        send_message(second_player_socket, "move on");
                        receive_message(second_player_socket);
                    }
                }
                if (yes_no_vector[0] == "yes" && yes_no_vector[1] == "yes") {
                    if (first_applicator_pid == first_player_pid) {
                        send_message(first_player_socket, "move on");
                        receive_message(first_player_socket);
                    } else {
                        send_message(second_player_socket, "move on");
                        receive_message(second_player_socket);
                    }
                }
                if (yes_no_vector[0] == "no" && yes_no_vector[1] == "yes") {
                    if (second_applicator_pid == first_player_pid) {
                        send_message(first_player_socket, "move on");
                        receive_message(first_player_socket);
                    } else {
                        send_message(second_player_socket, "move on");
                        receive_message(second_player_socket);
                    }
                }
                if (yes_no_vector[0] == "no" && yes_no_vector[1] == "no") {
                    if (second_applicator_pid == first_player_pid) {
                        send_message(first_player_socket, "move on");
                        receive_message(first_player_socket);
                    } else {
                        send_message(second_player_socket, "move on");
                        receive_message(second_player_socket);
                    }
                }
            }
        } else if (tmp == "invite") {
			std::cout << "Обрабатываю приглашение на игру" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::string invite_login;
            std::getline(ss, tmp, ':');
            int sender_id = std::stoi(tmp);
            std::getline(ss, invite_login, ':');
            
			if (invite_login == login_map[sender_id]) {
				std::cout << "Игрок пригласил сам себя" << std::endl;
				send_message(main_socket, "Error:SelfInvite");
			} else if (invite_login == login_map[2]) {
				std::cout << "Игрок " + login_map[1] + " пригласил в игру " + login_map[2] << std::endl;
                send_message(second_player_socket, "invite:" + login_map[1]);
				std::string invite_message = receive_message(second_player_socket);

                if (invite_message == "accept") {
					std::cout << "Игрок " + login_map[2] + " принял запрос " << std::endl;
                    send_message(main_socket, invite_message);
                    break;
                } else if (invite_message == "reject") {
					std::cout << "Игрок " + login_map[2] + " отклонил запрос. Игра окончена." << std::endl;
                    kill(first_player_pid, SIGTERM);
                    kill(second_player_pid, SIGTERM);
                    return 0;
                }
            } else if (invite_login == login_map[1]){
				std::cout << "Игрок " + login_map[2] + " пригласил в игру " + login_map[1] << std::endl;
                send_message(first_player_socket, "invite:" + login_map[2]);
				std::string invite_message = receive_message(first_player_socket);

                if (invite_message == "accept") {
					std::cout << "Игрок " + login_map[1] + " принял запрос " << std::endl;
                    send_message(main_socket, invite_message);
                    break;
                } else if (invite_message == "reject") {
					std::cout << "Игрок " + login_map[1] + " отклонил запрос. Игра окончена." << std::endl;
                    kill(first_player_pid, SIGTERM);
                    kill(second_player_pid, SIGTERM);
                    return 0;
                }
            } else {
				std::cout << "Ника " + invite_login + " отсутствует в базе" << std::endl;
				std::cout << "Отправляю ошибку игроку" << std::endl;
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				
				send_message(main_socket, "Error:LoginNotExist");
            }
            std::getline(ss, tmp, ':');
        }
    }

    std::cout << "Запускаю игру" << std::endl;
    Game game;
    game.play(first_player_socket, second_player_socket, first_player_pid, second_player_pid);
}