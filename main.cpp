#include <opendht.h>
#include <iterator>
using namespace std;

vector<std::string> split(const string& str, const std::string& delimiter) {
    istringstream iss(str);
    vector<string> tokens{istream_iterator<string>{iss},
                          istream_iterator<string>{}};

    return tokens;
}

int main()
{
    string conn_to_existing;
    string bootstrap_IP;
    string bootstrap_Port;

    cout << "Welcome to Peace - A simple OpenDHT example." << endl << endl
    << "Do you want to connect to an existing network? [y/n]: ";

    getline(cin, conn_to_existing);

    if (conn_to_existing == "y" || conn_to_existing == "yes") {
        cout << "Enter the IP of a known node: ";
        getline(cin, bootstrap_IP);
        cout << "Enter the OpenDHT service port of the known node: ";
        getline(cin, bootstrap_Port);
    }

    string local_port;

    cout << "Enter the port this node should use: ";
    getline(cin, local_port);

    dht::DhtRunner node;

    // Launch a dht node on a new thread, using a
    // generated RSA key pair, and listen on port.
    dht::DhtRunner::Config cfg = dht::DhtRunner::Config();
    cfg.threaded = true;
    cfg.client_identity = dht::crypto::generateIdentity();
    cfg.dht_config.node_config.is_bootstrap = conn_to_existing.empty();
    cfg.dht_config.node_config.network = 0;
    node.run(stoi(local_port), cfg);

    if (conn_to_existing == "y" || conn_to_existing == "yes") {
        // Join the network through any running node,
        // here using a known bootstrap node.
        node.bootstrap(bootstrap_IP, bootstrap_Port);
    }

    cout << endl << "[!] Node started. Use 'help' to get a help dialog." << endl << endl;

    while (true) {
        cout << "> ";

        string cmd;
        getline(cin, cmd);

        if (cmd.empty()) continue;

        vector<string> tokens = split(cmd, " ");

        if (tokens[0] == "exit" || tokens[0] == "quit") break;
        else if (tokens[0] == "help") {
            cout << "put <key> <value> - Put a new key value pair into the DHT." << endl
            << "get <key> - Retrieves the values found under the given key from the DHT." << endl
            << "help - Print this help message." << endl
            << "exit - Stops the node and exits the program." << endl
            << "quit - Same as exit." << endl;
        }
        else if (tokens[0] == "put") {
            if (tokens.size() < 3) {
                cout << "The 'put' command takes 2 additional arguments (put <key> <value>) but you only provided "
                << tokens.size() - 1 << "." << endl;
                continue;
            }
            node.put(
                    dht::InfoHash::get(tokens[1]),
                    dht::Value(
                            (const uint8_t*) tokens[2].c_str(),
                            strlen(tokens[2].c_str())
                            ));
        } else if (tokens[0] == "get") {
            if (tokens.size() < 2) {
                cout << "The 'get' command takes 1 additional argument (get <key>) but you only provided "
                     << tokens.size() -1 << "." << endl;
                continue;
            }

            auto val = node.get(dht::InfoHash::get(tokens[1])).get();
            for (const auto& v : val) {
                auto data = v->data.data();
                cout << "Got value: ";
                for (int i = 0; i < v->size(); i++) {
                    cout << data[i];
                }
                cout << endl;
            }
            if (val.empty()) {
                cout << "No value for the key '" << tokens[1] << "' was found." << endl;
            }
        } else
            cout << "The command '" << tokens[0]
            << "' doesn't exit. Use 'help for an overview of the supported commands." << endl;
    }
    // wait for dht threads to end
    node.join();
    return 0;
}
