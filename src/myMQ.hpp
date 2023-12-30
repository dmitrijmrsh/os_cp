#pragma once

#include <zmq.hpp>
#include <iostream>
#include <signal.h>

const int BASE_PORT = 5555;
int PORT_ITER;

std::string receive_message(zmq::socket_t& socket) {
    zmq::message_t message;

    bool ok = false;
    try {
        ok = bool(socket.recv(message, zmq::recv_flags::none));
    }
    catch (...) {
        ok = false;
    }

    std::string recieved_message(static_cast<char*>(message.data()), message.size());
    if (recieved_message.empty() || !ok) {
        return "";
    }

    return recieved_message;
}

bool try_recv(pid_t first_player_pid, pid_t second_player_pid) {
    if (kill(first_player_pid, 0) != 0 || kill(second_player_pid, 0) != 0) {
        return false;
    }
    return true;
}

std::string GetConPort(int id) {
    return "tcp://127.0.0.1:" + std::to_string(id);
}

bool send_message(zmq::socket_t& socket, const std::string& message_string) {
    zmq::message_t message(message_string.size());
    memcpy(message.data(), message_string.c_str(), message_string.size());
    return bool(socket.send(message, zmq::send_flags::none));
}