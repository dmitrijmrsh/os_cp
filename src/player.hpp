#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <signal.h>

#include "myMQ.hpp"

const int BOARD_SIZE = 10;

class Player {
public:
    std::vector<std::vector<char>> board;
	int num;

    Player() {
        board = std::vector<std::vector<char>>(BOARD_SIZE, std::vector<char>(BOARD_SIZE, ' '));
    }

    void placeShips(zmq::socket_t& player_socket, pid_t first_player_pid, pid_t second_player_pid) {
		pid_t pid = getpid();
		std::string msg;
		std::string tmp;
		std::string orientation;
		std::string received_message;
		bool alive;
		int x, y;
		for (int i = 1; i <= 4; ++i) {
			for (int j = 4; j >= i; --j) {
				msg = "Введите ориентацию корабля";
				send_message(player_socket, msg);
				orientation = (receive_message(player_socket));
				orientation = orientation.substr(12, orientation.size());
				msg = "Разместите " + std::to_string(i) + " палубный корабль";
				bool valid_ship = true;
				std::vector<std::pair<int, int>> points;
				std::pair<int, int> prev_point = std::make_pair(-1, -1);
				for (int k = 0; k < i; ++k) {
					alive  = try_recv(first_player_pid, second_player_pid);
					if (!alive) {
						kill(first_player_pid, SIGTERM);
                		kill(second_player_pid, SIGTERM);
                		std::cout << "Игра завершена из-за убийства одного или обоих клиентов" << std::endl;
                		kill(pid, SIGTERM);
					}
					send_message(player_socket, msg + std::to_string(k));
					received_message = receive_message(player_socket);
					std::cout << "Получил запрос: " + received_message << std::endl;

					std::stringstream strs(received_message);
					std::getline(strs, tmp, ':');
					if (tmp == "coords") {
						std::getline(strs, tmp, ':');
						x = std::stoi(tmp);
						std::getline(strs, tmp, ':');
						y = std::stoi(tmp);

						if (!((orientation == "V") or (orientation == "H"))) {
							send_message(player_socket, "Error : Такой ориентации нет");
							received_message = receive_message(player_socket);
							++j;
							break;
						}
						if (!ValidPointCheck(x, y, prev_point, orientation)) {
							valid_ship = false;
						}
						prev_point.first = x;
						prev_point.second = y;
						points.push_back(std::make_pair(x, y));
					}
				}
				if (valid_ship) {
					for (auto& elem : points) {
						board[elem.first][elem.second] = 'O';
					}
					std::string boardState = "board";
					boardState += getBoard();
					send_message(player_socket, boardState);
					received_message = receive_message(player_socket);
				} else {
					send_message(player_socket, "Error : Неверное местоположение корабля. Попробуйте ещё раз");
					received_message = receive_message(player_socket);
					++j;
				}
			}
		}
    }

    bool ValidPointCheck(int x, int y, std::pair<int, int>& prev_point, std::string& orientation) const {
		if (x > 9 || x < 0 || y > 9 || y < 0) {
			return false;
		}

		if (prev_point.first == -1 && prev_point.second == -1) {
			if (!isEmptyAround(x, y)) {
				return false;
			}
		} else {
			if (orientation == "V") {
				if (prev_point.second != y || (abs(prev_point.first - x) > 1) || AroundCount(x, y) > 0) {
					return false;
				}
			}
			if (orientation == "H") {
				if (prev_point.first != x || (abs(prev_point.second - y) > 1) || AroundCount(x, y) > 0) {
					return false;
				}
			}
		}

        return true;
    }

	int AroundCount(int x, int y) const {
		int count = 0;
		for (int i = x - 1; i <= x + 1; ++i) {
			for (int j = y - 1; j <= y + 1; ++j) {
				if (i < BOARD_SIZE && j < BOARD_SIZE && i >= 0 && j >= 0 && board[i][j] != ' ') {
					++count;
				}
			}
		}
		return count;
	}

	bool isEmptyAround(int x, int y) const {
		for (int i = x - 1; i <= x + 1; ++i) {
			for (int j = y - 1; j <= y + 1; ++j) {
				if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE && board[i][j] != ' ') {
					return false;
				}
			}
		}
		return true;
	}

    std::string getBoard() const {
		std::string result;
		std::string probel(1, ' ');
        result = "  0 1 2 3 4 5 6 7 8 9\n";
        for (int i = 0; i < BOARD_SIZE; ++i) {
            result += std::to_string(i) + " ";
            for (int j = 0; j < BOARD_SIZE; ++j) {
				std::string brd(1, board[i][j]);
                result += brd + probel;
            }
            result += '\n';
        }
        result += '\n';
		return result;
    }

    std::string getClearBoard() const {
		std::string result;
		std::string space(1, ' ');
        result = "  0 1 2 3 4 5 6 7 8 9\n";
        for (int i = 0; i < BOARD_SIZE; ++i) {
            result += std::to_string(i) + " ";
            for (int j = 0; j < BOARD_SIZE; ++j) {
				std::string brd(1, board[i][j]);
				if (brd == "O") {
                	result += space + space;	
				} else {
					result += brd + space;
				}
            }
            result += '\n';
        }
        result += '\n';
		return result;
    }
};