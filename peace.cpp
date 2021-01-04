// peace - A simple OpenDHT example
//    Copyright (C) 2021  Yannic Wehner
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see https://www.gnu.org/licenses/.

#include <opendht.h>
#include <iterator>
#include <fstream>

using namespace std;

void print_welcome_message() {
    cout << "Welcome to" << endl
         << "╔══╗╔══╗╔══╗ ╔══╗╔══╗" << endl
         << "║╔╗║║╔╗║╚ ╗║ ║╔═╝║╔╗║" << endl
         << "║╚╝║║║═╣║╚╝╚╗║╚═╗║║═╣" << endl
         << "║╔═╝╚══╝╚═══╝╚══╝╚══╝" << endl
         << "║║" << endl
         << "╚╝" << endl
         << "A simple OpenDHT example." << endl << endl;
}

// Split a string at every whitespace and return the result as std::vector
vector<string> split_at_ws(string str) {
    istringstream iss(str);
    vector<string> tokens{istream_iterator<string>{iss},
                          istream_iterator<string>{}};

    return tokens;
}

void store_data(dht::DhtRunner* node, vector<char> buffer, size_t size, string file_path) {
    auto hash = dht::InfoHash::get((uint8_t*) buffer.data(), size);

    node->put(hash, dht::Value((const uint8_t*) buffer.data(), size), dht::DoneCallback {}, dht::time_point::max(), true);

    std::ofstream outfile (file_path + ".odht", std::ofstream::out | std::ofstream::app);
    outfile << hash << std::endl;
    outfile.close();
}

void store_file(dht::DhtRunner* node, string file_path) {
    std::ifstream fin(file_path, std::ifstream::binary);
    if (fin.is_open()) {
        // Make sure the file we save the hashes to is empty.
        std::ofstream ofs (file_path + ".odht", std::ofstream::out | std::ofstream::trunc);
        ofs.close();

        const int BUFFER_SIZE = 4096;
        vector<char> buffer (BUFFER_SIZE + 1,0);
        while (true) {
            fin.read(buffer.data(), BUFFER_SIZE);
            streamsize data_size = ((fin) ? BUFFER_SIZE : fin.gcount());
            buffer[data_size] = 0;
            store_data(node, buffer, data_size, file_path);
            if (!fin) break;
        }
        cout << "Hash file was created and saved under '" + file_path + ".odht'." << endl;
    } else {
        cout << "[!] Couldn't open the file. Make sure it exists and you have the needed permissions to access it." << endl;
    }
}

void restore_data(dht::DhtRunner* node, string hash, string file_path) {
    std::ofstream outfile (file_path, std::ofstream::binary | std::ofstream::app);
    auto values = node->get(dht::InfoHash(hash)).get();
    auto data = values[0]->data.data();
    for (int i = 0; i < values[0]->size(); i++) {
        outfile << data[i];
    }
    outfile.close();
}

void restore_file(dht::DhtRunner* node, string file_path) {
    auto delim_i = file_path.find_last_of('.');
    auto orig_file_name = file_path.substr(0, delim_i);
    auto extension = file_path.substr(delim_i + 1);
    if (extension != "odht") {
        cout << "[!] Please provide a valid hash file for restoration." << endl;
        return;
    }

    std::ifstream fin(file_path);
    if (fin.is_open()) {
        // Make sure the file we restore to is empty.
        std::ofstream ofs (orig_file_name, std::ofstream::out | std::ofstream::trunc);
        ofs.close();

        std::string line;
        while (std::getline(fin, line)) {
            restore_data(node, line, orig_file_name);
        }
        fin.close();
        cout << "The file was restored and saved under '" + orig_file_name + "'." << endl;
    } else {
        cout << "[!] Couldn't open the file. Make sure it exists and you have the needed permissions to access it." << endl;
    }
}

void start_node(dht::DhtRunner* node) {
    // Ask the user if he wants to connect to an existing OpenDHT network.

    // If true peace will try to establish a connection to a given network
    bool conn_to_existing_network;

    // The IP of the known network node
    string bootstrap_IP;

    // The port the existing OpenDHT node is running on
    string bootstrap_port;

    cout << "Do you want to connect to an existing network? [y/n]: ";
    string temp;
    getline(cin, temp);
    conn_to_existing_network = temp == "yes" || temp == "y";


    // If yes, ask for the IP and port of a known network node.
    if (conn_to_existing_network) {
        cout << "Enter the IP of a known node: ";
        getline(cin, bootstrap_IP);
        cout << "Enter the OpenDHT service port of the known node: ";
        getline(cin, bootstrap_port);
    }

    // The local port this node should bind to.
    string local_port;

    cout << "Enter the port this node should use: ";
    getline(cin, local_port);

    // Launch a dht node on a new thread, using a
    // generated RSA key pair, and listen on the given port.
    // Set the note as bootstrap node when not connecting to existing network.
    dht::DhtRunner::Config cfg = dht::DhtRunner::Config();
    cfg.threaded = true;
    cfg.client_identity = dht::crypto::generateIdentity();
    cfg.dht_config.node_config.is_bootstrap = !conn_to_existing_network;
    // cfg.dht_config.node_config.network = 0;

    node->run(stoi(local_port), cfg);

    if (conn_to_existing_network) {
        // Join the network through the running node.
        node->bootstrap(bootstrap_IP, bootstrap_port);
    }

    cout << endl << "[!] Node started. Use 'help' to get a help dialog." << endl << endl;
}

// The control loop for the CLI interface.
void cmd_loop(dht::DhtRunner* node) {
    while (true) {
        cout << "> ";

        string input;
        getline(cin, input);

        if (input.empty()) continue;

        vector<string> tokens = split_at_ws(input);
        string cmd = tokens[0];

        if (cmd == "exit" || cmd == "quit") break;
        else if (cmd == "help") {
            cout << "put <key> <value> - Put a new key value pair into the DHT." << endl
                 << "get <key> - Retrieves the values found under the given key from the DHT." << endl
                 << "help - Print this help message." << endl
                 << "exit - Stops the node and exits the program." << endl
                 << "quit - Same as exit." << endl;
        } else if (cmd == "put") {
            if (tokens.size() < 3) {
                cout << "The 'put' command takes 2 additional arguments (put <key> <value>) but you only provided "
                     << tokens.size() - 1 << "." << endl;
                continue;
            }
            node->put(
                    dht::InfoHash::get(tokens[1]),
                    dht::Value(
                            (const uint8_t *) tokens[2].c_str(),
                            strlen(tokens[2].c_str())
                    ));
        } else if (cmd == "store") {
            store_file(node, tokens[1]);
        } else if (cmd == "get") {
            if (tokens.size() < 2) {
                cout << "[!] The 'get' command takes 1 additional argument (get <key>) but you only provided "
                     << tokens.size() - 1 << "." << endl;
                continue;
            }

            auto values = node->get(dht::InfoHash::get(tokens[1])).get();
            if (values.empty()) {
                cout << "No value for the key '" << tokens[1] << "' was found." << endl;
                continue;
            }
            for (const auto &v : values) {
                auto data = v->data.data();
                cout << "Got value: ";
                for (int i = 0; i < v->size(); i++) {
                    cout << data[i];
                }
                cout << endl;
            }
        } else if (cmd == "restore") {
            restore_file(node, tokens[1]);
        } else
            cout << "[!] The command '" << cmd
                 << "' doesn't exit. Use 'help' for an overview of the supported commands." << endl;
    }
}

int main() {
    dht::DhtRunner node;

    print_welcome_message();
    start_node(&node);
    cmd_loop(&node);

    // wait for dht thread to end
    node.join();
    return 0;
}
