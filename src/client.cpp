#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <signal.h>
#include <thread>
#include <chrono>
#include <ctime>

#include "myMQ.hpp"

std::string command;
pthread_mutex_t mutex;

zmq::context_t context(2);
zmq::socket_t player_socket(context, ZMQ_REP);

void* check_invite(void *param) {
    pid_t* mypid = static_cast<pid_t*>(param);
    std::string invite_tmp;
    pthread_mutex_lock(&mutex);
    std::string invite_msg = receive_message(player_socket);
    std::stringstream invite_ss(invite_msg);
    std::getline(invite_ss, invite_tmp, ':');
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::getline(invite_ss, invite_tmp, ':');
    std::cout << "Игрок с ником " + invite_tmp + " приглашает вас в игру!" << std::endl;
    std::cout << "Вы согласны? (y/n)" << std::endl;
    std::cin >> command;
    std::cout << "Ваш ответ: " + command + "\n";
    if (command[0] == 'y') {
        std::cout << "Вы приняли запрос!" << std::endl;
        send_message(player_socket, "accept");
        pthread_mutex_unlock(&mutex);
        pthread_exit(0);
    } else {
        std::cout << "Вы отклонили запрос!. Игра окончена, даже не начавшись :(" << std::endl;
        pthread_mutex_unlock(&mutex);
        send_message(player_socket, "reject");
    }
	pthread_exit(0);
}

int main(int argc, char** argv) {
    zmq::context_t context(2);
    zmq::socket_t main_socket(context, ZMQ_REQ);
    pid_t pid = getpid();
    main_socket.connect(GetConPort(5555));
    pthread_mutex_init(&mutex, NULL);

    pthread_t tid;
    std::string received_message;
    std::string tmp;
    bool login_stage = false;
    bool pid_flag = false;
    bool move_on_flag = false;

    while(true) {
        if (!login_stage) {
            login_stage = true;

            std::string login;
            std::cout << "Введите ваш логин: ";
            std::cin >> login;

            std::string login_msg = "login:" + login + ":" + std::to_string(pid);
            send_message(main_socket, login_msg);
            received_message = receive_message(main_socket);
            std::stringstream ss(received_message);

            std::getline(ss, tmp, ':');
            if (tmp == "Ok") {
                std::getline(ss, tmp, ':');
                PORT_ITER = std::stoi(tmp);
                player_socket.connect(GetConPort(5555 + PORT_ITER));
                std::cout << "Авторизация прошла успешно" << std::endl;
                std::cout << "Вы хотите пригласить друга? (y/n)" << std::endl;
                std::cin >> tmp;
                std::string yes_no_message;
                if (tmp[0] == 'n') {
                    yes_no_message = "no:" + std::to_string(pid);
                    send_message(main_socket, yes_no_message);
                    received_message = receive_message(main_socket);
                    if (received_message == "good") {
                        std::cout << "Ждем приглашения от друга..." << std::endl;
                        pthread_create(&tid, NULL, check_invite, &pid);
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                        break;
                    } else {
                        std::cout << "Тебе нужно пригласить второго игрока, чтобы начать игру" << std::endl;
                        std::cout << "Чтобы пригласить игрока напишите invite и его логин через пробел" << std::endl;
                    }
                } else if (tmp[0] == 'y') {
                    yes_no_message = "yes:" + std::to_string(pid);
                    send_message(main_socket, yes_no_message);
                    received_message = receive_message(main_socket);
                    if (received_message == "good") {
                        std::cout << "Чтобы пригласить игрока напишите invite и его логин через пробел" << std::endl;
                    } else {
                        std::cout<< "Тебя уже хочет пригласить другой игрок, дождись его" << std::endl;
                        pthread_create(&tid, NULL, check_invite, &pid);
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                        break;
                    }
                }
            } else if (tmp == "Error") {
                std::getline(ss, tmp, ':');
                if (tmp == "NameAlreadyExist") {
                    std::cout << "ERROR: Это имя уже занято! Попробуйте другое" << std::endl;
                    login_stage = false;
                }
            }
        } else {
            std::cin >> command;
            if (!move_on_flag) {
                received_message = receive_message(player_socket);
                send_message(player_socket, "do:nothing");
                move_on_flag = true;
            }
            if (command == "invite") {
                std::string invite_login;
                std::cin >> invite_login;
                std::cout << "Вы пригласили игрока с ником " + invite_login << std::endl;
                std::cout << "Ждем ответ..." << std::endl;
                std::string invite_cmd = "invite:" + std::to_string(PORT_ITER) + ":" + invite_login; 
                send_message(main_socket, invite_cmd);
                received_message = receive_message(main_socket);
                std::stringstream ss(received_message);
                std::getline(ss, tmp, ':');
                if (tmp == "accept") {
                    std::cout << "Запрос принят!" << std::endl;
                    break;
                } else if (tmp == "reject") {
                    std::cout << "Запрос отклонен! " << std::endl;
                } else if (tmp == "Error") {
                    std::getline(ss, tmp, ':');
                    if (tmp == "SelfInvite") {
                        std::cout << "ERROR: Вы отправили запрос на игру самому себе. Попробуйте снова" << std::endl;
                    } else if (tmp == "LoginNotExist") {
                        std::cout << "ERROR: Игрока с таким ником не существует. Попробуйте снова" << std::endl;
                    }
                }
            } else {
                std::cout << "Вы ввели несуществующую команду. Попробуйте снова" << std::endl;
            }
        }
    }

    pthread_mutex_lock(&mutex);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (PORT_ITER == 1) {
        std::cout << "Начинаем игру" << std::endl;
    } else {
        std::cout << "Начинаем игру. Подождите, пока другой пользователь расставит корабли" << std::endl;
    }

    while(1) {
        std::string inside_msg;
        std::string send_msg;
        inside_msg = receive_message(player_socket);
        std::stringstream strs(inside_msg);
        strs >> tmp;

        if (tmp == "Разместите") {
            if (inside_msg[inside_msg.size() - 1] == '0') {
                std::cout << inside_msg.substr(0, inside_msg.size() - 1) << std::endl;
            }
            int x, y;
		    std::cin >> x >> y;
            send_msg = "coords:" + std::to_string(x) + ":" + std::to_string(y);
            send_message(player_socket, send_msg);
        } else if (tmp == "Введите") {
            std::cout << inside_msg << std::endl;
            std::string orientation;
            std::cin >> orientation;
            send_msg = "orientation:" + orientation;
            send_message(player_socket, send_msg);
        } else if (tmp == "board") {
            std::cout << inside_msg.substr(5, inside_msg.size()) << std::endl;
            send_message(player_socket, "ok");
        } else if (tmp == "Error") {
            std::cout << inside_msg << std::endl;
            send_message(player_socket, "ok");
        } else if (tmp == "your_turn") {
            send_message(player_socket, "ok");
            std::cout << "Ваш ход:" << std::endl;
            inside_msg = receive_message(player_socket);
            if (inside_msg == "shoot") {
                int x, y;
                std::cout << "Введите координаты выстрела (формат: x y):"  << std::endl;
                std::cin >> x >> y;
                send_msg = "coords:" + std::to_string(x) + ":" + std::to_string(y);
                send_message(player_socket, send_msg);
                inside_msg = receive_message(player_socket);
                if (inside_msg == "shooted") {
                    std::cout << "Попадание!" << std::endl;
                    send_message(player_socket, "ok");
                } else if (inside_msg == "miss") {
                    std::cout << "Промах!" << std::endl;
                    send_message(player_socket, "ok");
                }
            }
        } else if (tmp == "not_your_turn") {
            std::cout << "Ход соперника: " << std::endl;
            send_message(player_socket, "ok");
            inside_msg = receive_message(player_socket);
            if (inside_msg == "shooted") {
                std::cout << "Вас подстрелили!" << std::endl;
                send_message(player_socket, "ok");
            } else if (inside_msg == "miss") {
                std::cout << "Противник промахнулся" << std::endl;
                send_message(player_socket, "ok");
            }
        } else if (tmp == "win") {
            std::cout << "Вы выиграли!" << std::endl;
            send_message(player_socket, "ok");
            pthread_mutex_unlock(&mutex);
            return 0;
        } else if (tmp == "lose") {
            std::cout << "Вы проиграли!" << std::endl;
            send_message(player_socket, "ok");
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
}
 