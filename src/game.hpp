#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "myMQ.hpp"
#include "player.hpp"

class Game {
public:
    Player player1;
	Player player2;
	
    void play(zmq::socket_t& player1_socket, zmq::socket_t& player2_socket, pid_t first_player_pid, pid_t second_player_pid) {
        std::cout << "Игра началась!" << std::endl;

		std::string msg1;
		std::string msg2;
		pid_t pid = getpid();
		bool alive;

		player1.num = 1;
		player2.num = 2;
        player1.placeShips(player1_socket, first_player_pid, second_player_pid);
        player2.placeShips(player2_socket, first_player_pid, second_player_pid);

		int turn = 0;
        while (!gameOver()) {
			alive = try_recv(first_player_pid, second_player_pid);
			if (!alive) {
				kill(first_player_pid, SIGTERM);
				kill(second_player_pid, SIGTERM);
				std::cout << "Игра завершена из-за убийства одного или обоих клиентов" << std::endl;
				kill(pid, SIGTERM);
			}
			if (turn % 2 == 0) {
				msg1 = "your_turn";
				msg2 = "not_your_turn";
				send_message(player1_socket, msg1);
				receive_message(player1_socket);
				send_message(player2_socket, msg2);
				receive_message(player2_socket);
				std::cout << "Ход игрока 1:" << std::endl;
				if (playerTurn(player1, player2, player1_socket, player2_socket)) {
					if (gameOver())  {
						std::cout << "Победил игрок 1" << std::endl;
						send_message(player1_socket, "win");
						receive_message(player1_socket);
						send_message(player2_socket, "lose");
						receive_message(player2_socket);
						break;
					}
					continue;
				}
				else {
					turn += 1;
				}
			} else {
				std::cout << "Ход игрока 2:" << std::endl;
				msg1 = "your_turn";
				msg2 = "not_your_turn";
				send_message(player2_socket, msg1);
				receive_message(player2_socket);
				send_message(player1_socket, msg2);
				receive_message(player1_socket);
				if (playerTurn(player2, player1, player2_socket, player1_socket)) {
					if (gameOver()) {
						std::cout << "Победил игрок 2" << std::endl;
						send_message(player2_socket, "win");
						receive_message(player2_socket);
						send_message(player1_socket, "lose");
						receive_message(player1_socket);
						break;
					}
					continue;
				}
				else {
					turn += 1;
				}
			}
        }
        std::cout << "Игра завершена!" << std::endl;
    }

    bool gameOver() const {
        return allShipsDead(player1) || allShipsDead(player2);
    }

    bool allShipsDead(const Player& player) const {
        for (const auto& row : player.board) {
            for (char cell : row) {
                if (cell == 'O') {
                    return false;
                }
            }
        }
        return true;
    }

    bool playerTurn(Player& attacker, Player& defender, zmq::socket_t& attacker_socket, zmq::socket_t& defender_socket) {
		bool shoot = false;
		std::string received_message;
		std::string tmp;
        int x, y;
		send_message(attacker_socket, "shoot");
		received_message = receive_message(attacker_socket);
		std::stringstream strs(received_message);
		std::getline(strs, tmp, ':');
		std::getline(strs, tmp, ':');
		std::cout << tmp << std::endl;
		x = std::stoi(tmp);
		std::getline(strs, tmp, ':');
		std::cout << tmp << std::endl;
		y = std::stoi(tmp);

        if (isValidMove(x, y, defender)) {
            if (defender.board[x][y] == 'O') {
				send_message(attacker_socket, "shooted");
				received_message = receive_message(attacker_socket);
				send_message(defender_socket, "shooted");
				received_message = receive_message(defender_socket);

                std::cout << "Попадание!" << std::endl;
                defender.board[x][y] = 'X';
				shoot = true;
				return shoot;
            } else {
				send_message(attacker_socket, "miss");
				received_message = receive_message(attacker_socket);
				send_message(defender_socket, "miss");
				received_message = receive_message(defender_socket);

                std::cout << "Промах!" << std::endl;
                defender.board[x][y] = '*';
				std::string board = defender.getBoard();
				std::string clearBoard = defender.getClearBoard();
				
				send_message(attacker_socket, "board" + clearBoard);
				received_message = receive_message(attacker_socket);
				send_message(defender_socket, "board" + board);
				received_message = receive_message(defender_socket);
				return shoot;
            }

        } else {
            std::cout << "Неверные координаты. Игрок потерял ход" << std::endl;
			send_message(attacker_socket, "miss");
			received_message = receive_message(attacker_socket);
			send_message(defender_socket, "miss");
			received_message = receive_message(defender_socket);
			return shoot;
        }
    }

    bool isValidMove(int x, int y, const Player& defender) const {
        return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && (defender.board[x][y] == ' ' || defender.board[x][y] == 'O');
    }
};