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

#include <fstream>
#include <iterator>
#include "opendht.h"

using namespace std;

// Prints a welcome message.
void print_welcome_message()
{
    cout << "Welcome to" << endl
         << "╔══╗╔══╗╔══╗ ╔══╗╔══╗" << endl
         << "║╔╗║║╔╗║╚ ╗║ ║╔═╝║╔╗║" << endl
         << "║╚╝║║║═╣║╚╝╚╗║╚═╗║║═╣" << endl
         << "║╔═╝╚══╝╚═══╝╚══╝╚══╝" << endl
         << "║║ A simple" << endl
         << "╚╝  OpenDHT example." << endl
         << endl;
}

// Returns the file extension used for the hash files.
string get_file_extension() { return ".pce"; }

// Returns the storage limit that should be used for a single OpenDHT node.
// Needs to be increased if you want to store larger amounts of data in a small group of nodes.
size_t get_storage_limit() { return 1024 * 1024 * 512; } // 512 MB

// Split a string at every whitespace and return the result as std::vector.
vector<string> split_at_ws(string str)
{
    istringstream iss(str);
    vector<string> tokens{istream_iterator<string>{iss},
                          istream_iterator<string>{}};

    return tokens;
}

// Stores a chunk of data contained in buffer into the OpenDHT instance running in node
// and writes its hash into a file for later retrieval.
void store_data(dht::DhtRunner *node, vector<char> buffer, size_t size, ofstream *hash_file)
{
    auto hash = dht::InfoHash::get((uint8_t *)buffer.data(), size);

    node->put(
        hash, dht::Value((const uint8_t *)buffer.data(), size), [](bool success) {
            cout << "[!] Put of chunk finished with " << (success ? "success." : "failure.") << endl;
            cout << "> ";
            cout.flush();
        },
        dht::time_point::max(), true);

    *hash_file << hash << endl;
}

// Stores the file under file_path into the OpenDHT instance running in node
// and creates a file with a list of hashes for later retrieval.
void store_file(dht::DhtRunner *node, string file_path)
{
    ifstream fin(file_path, ifstream::binary);
    if (fin.is_open())
    {
        auto hash_file_path = file_path + get_file_extension();

        // Make sure the file we save the hashes to is empty.
        ofstream ofs(hash_file_path, ofstream::out | ofstream::trunc);
        ofs.close();

        ofstream hash_file(hash_file_path, ofstream::out | ofstream::app);

        const int BUFFER_SIZE = 4096; // BYTES = 4 kib
        vector<char> buffer(BUFFER_SIZE + 1, 0);
        while (true)
        {
            fin.read(buffer.data(), BUFFER_SIZE);
            streamsize data_size = ((fin) ? BUFFER_SIZE : fin.gcount());
            buffer[data_size] = 0;
            store_data(node, buffer, data_size, &hash_file);
            if (!fin)
                break;
        }
        hash_file.close();
        cout << "Hash file was created and saved under '" << hash_file_path + "'." << endl;
    }
    else
    {
        cout << "[!] Couldn't open the file. Make sure it exists and you have the needed permissions to access it."
             << endl;
    }
}

// Restores a chunk of data of the hash out of the OpenDHT instance running in node
// and writes it into a file to restore its original content.
bool restore_data(dht::DhtRunner *node, string hash, ofstream *out_file)
{
    auto values = node->get(dht::InfoHash(hash)).get();
    if (values.empty())
    {
        cout << "[!] The chunk with hash '0x" << hash << "' couldn't be restored." << endl;
        return false;
    }
    auto data = values[0]->data.data();
    for (int i = 0; i < values[0]->size(); i++)
    {
        *out_file << data[i];
    }
    cout << "[!] Restored chunk with hash '0x" << hash << "' successfully." << endl;
    return true;
}

// Restores the original file from the OpenDHT instance running in node using the hash file under hash_file_path.
void restore_file(dht::DhtRunner *node, string hash_file_path)
{
    auto hash_extension_delimiter_i = hash_file_path.find_last_of('.');
    auto orig_file_path = hash_file_path.substr(0, hash_extension_delimiter_i);
    auto hash_file_extension = hash_file_path.substr(hash_extension_delimiter_i + 1);
    auto orig_extension_delimiter_i = orig_file_path.find_last_of('.');
    auto orig_file_extension = orig_file_path.substr(orig_extension_delimiter_i + 1);
    orig_file_path = orig_file_path.substr(0, orig_extension_delimiter_i) + "_restored." + orig_file_extension;

    if ("." + hash_file_extension != get_file_extension())
    {
        cout << "[!] Please provide a valid ." << get_file_extension() << " hash file for restoration." << endl;
        return;
    }

    ifstream fin(hash_file_path);
    if (fin.is_open())
    {
        // Make sure the file we restore to is empty.
        ofstream ofs(orig_file_path, ofstream::out | ofstream::trunc);
        ofs.close();

        ofstream outfile(orig_file_path, ofstream::binary | ofstream::app);

        string line;
        while (getline(fin, line))
        {
            if (!restore_data(node, line, &outfile))
            {
                cout << "[!] Aborting restoration and deleting broken file '" << orig_file_path << "'." << endl;
                fin.close();
                outfile.close();
                if (remove(orig_file_path.c_str()))
                    cout << "[!] Error deleting the broken file. Consider deleting it manually" << endl;
                else
                    cout << "[!] Successfully deleted broken file." << endl;
                return;
            }
        }
        fin.close();
        outfile.close();
        cout << "The file was restored and saved under '" + orig_file_path + "'." << endl;
    }
    else
    {
        cout << "[!] Couldn't open the file. Make sure it exists and you have the needed permissions to access it."
             << endl;
    }
}

// Takes some configuration input from the user and starts running the OpenDHT instance in node.
void start_node(dht::DhtRunner *node)
{
    // Ask the user if he wants to connect to an existing OpenDHT network.

    // If true peace will try to establish a connection to a given network.
    bool conn_to_existing_network;

    // If true will send broadcast peer discovery requests over the local network.
    bool peer_discovery;

    // If true will send broadcast peer publish requests over the local network.
    bool peer_publish;

    // The IP of the known network node.
    string bootstrap_IP;

    // The port the existing OpenDHT node is running on.
    string bootstrap_port;

    string temp;

    cout << "Do you want to connect to an existing remote network?" << endl
         << "If not peer discovery will be used to find nodes inside the local network. [y/n]: ";
    getline(cin, temp);
    conn_to_existing_network = (temp == "yes" || temp == "y");

    // If yes, ask for the IP and port of a known network node.
    if (conn_to_existing_network)
    {
        cout << "Enter the IP of the known remote node: ";
        getline(cin, bootstrap_IP);
        cout << "Enter the OpenDHT service port of the known remote node: ";
        getline(cin, bootstrap_port);
        cout << "[!] Connecting to the bootstrap node '" << bootstrap_IP << ":" << bootstrap_port << "'." << endl
             << "Do you want to activate peer discovery for the local network anyway? [y/n]: ";
        getline(cin, temp);
        peer_discovery = (temp == "yes" || temp == "y");
        peer_publish = peer_discovery;
    }
    else
    {
        cout << "[!] Using peer discovery in the local network." << endl;
        peer_publish = true;
        peer_discovery = true;
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
    cfg.peer_discovery = peer_discovery;
    cfg.peer_publish = peer_publish;
    // Changing the network ID will prevent this node from accidentally connecting to other public nodes.
    cfg.dht_config.node_config.network = 420;
    cfg.client_identity = dht::crypto::generateIdentity();
    cfg.dht_config.node_config.max_req_per_sec = -1;
    cfg.dht_config.node_config.max_peer_req_per_sec = -1;
    cfg.dht_config.node_config.max_searches = -1;

    node->run(stoi(local_port), cfg);
    node->setStorageLimit(get_storage_limit());

    if (conn_to_existing_network)
    {
        // Join the network through the running node.
        node->bootstrap(bootstrap_IP, bootstrap_port);
    }

    cout << endl
         << "[!] Node started. Use 'help' to get a help dialog." << endl
         << endl;
}

// The control loop for the CLI interface.
void cmd_loop(dht::DhtRunner *node)
{
    while (true)
    {
        cout << "> ";

        string input;
        getline(cin, input);

        if (input.empty())
            continue;

        vector<string> tokens = split_at_ws(input);
        string cmd = tokens[0];

        if (cmd == "exit" || cmd == "quit")
            break;
        else if (cmd == "help")
        {
            cout << "put <key> <value> - Put a new key value pair into the DHT." << endl
                 << "get <key> - Retrieves the values found under the given key from the DHT." << endl
                 << "store <file path> - Stores a file into the DHT and generates a hash file for restoration." << endl
                 << "restore <file path> - Restores the original file from a hash file." << endl
                 << "help - Print this help message." << endl
                 << "exit - Stops the node and exits the program." << endl
                 << "quit - Same as exit." << endl;
        }
        else if (cmd == "put")
        {
            if (tokens.size() < 3)
            {
                cout << "The 'put' command takes 2 additional arguments (put <key> <value>) but you only provided "
                     << tokens.size() - 1 << "." << endl;
                continue;
            }
            node->put(
                dht::InfoHash::get(tokens[1]),
                dht::Value(
                    (const uint8_t *)tokens[2].c_str(),
                    strlen(tokens[2].c_str())));
        }
        else if (cmd == "store")
        {
            if (tokens.size() < 2)
            {
                cout << "The 'store' command takes 1 additional argument (store <file path>) but you only provided "
                     << tokens.size() - 1 << "." << endl;
                continue;
            }
            store_file(node, tokens[1]);
        }
        else if (cmd == "get")
        {
            if (tokens.size() < 2)
            {
                cout << "[!] The 'get' command takes 1 additional argument (get <key>) but you only provided "
                     << tokens.size() - 1 << "." << endl;
                continue;
            }

            auto values = node->get(dht::InfoHash::get(tokens[1])).get();
            if (values.empty())
            {
                cout << "No value for the key '" << tokens[1] << "' was found." << endl;
                continue;
            }
            for (const auto &v : values)
            {
                auto data = v->data.data();
                cout << "Got value: ";
                for (int i = 0; i < v->size(); i++)
                {
                    cout << data[i];
                }
                cout << endl;
            }
        }
        else if (cmd == "restore")
        {
            if (tokens.size() < 2)
            {
                cout << "The 'restore' command takes 1 additional argument (store <file path>) but you only provided "
                     << tokens.size() - 1 << "." << endl;
                continue;
            }
            restore_file(node, tokens[1]);
        }
        else
            cout << "[!] The command '" << cmd
                 << "' doesn't exit. Use 'help' for an overview of the supported commands." << endl;
    }
}

int main()
{
    dht::DhtRunner node;

    print_welcome_message();
    start_node(&node);
    cmd_loop(&node);

    // wait for dht thread to end
    node.join();
    return 0;
}
