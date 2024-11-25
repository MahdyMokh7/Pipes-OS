#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include<filesystem>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <map>
#include <queue>


#define MAX_BUFFER_SIZE 256

#define INPUT "input"
#define OUTPUT "output"



/* send the data in the below format for each part via namedPipe
    a line string
    first element in the line: the store(city) name
    second element in the line: the amount of left over 
    third element in the line: the price of the left over

*/



/* send the data in the below format via unamedPipes [total left-overs and price-benefits] 
    a string 
    the element in the string: the total price of the left overs of the store

*/

using namespace std;

struct InputRecord {
    int amount;
    int price;
};

string findPipeNameByPartName(const vector<string>& pipeNames, const string& partName) {
    for (const string& pipeName : pipeNames) {
        // Find the position of the '/' and '.'
        size_t slashPos = pipeName.find('/');
        size_t dotPos = pipeName.find('.', slashPos);

        // Check if both '/' and '.' exist and the substring between them matches part_name
        if (slashPos != string::npos && dotPos != string::npos && dotPos > slashPos) {
            string subStr = pipeName.substr(slashPos + 1, dotPos - slashPos - 1);

            if (subStr == partName) {
                return pipeName;  // Return the matching pipe name
            }
        }
    }

    return "";  // Return empty string if no match is found
}

string extractName(string path) {
    size_t lastSlash = path.find_last_of('/');
    size_t lastDot = path.find_last_of('.');

    if (lastSlash != string::npos && lastDot != string::npos && lastSlash < lastDot) {
        return path.substr(lastSlash + 1, lastDot - lastSlash - 1);
    }

    // If the format is invalid, return an empty string
    return "";
}

vector<string> stringToVector(const string& str) {
    vector<string> partNames;
    istringstream iss(str);
    string part;
    while (iss >> part) {
        partNames.push_back(part);
    }
    return partNames;
}

void processRows(const vector<tuple<string, int, int, string>>& rows, map<string, int>& part_price_map, map<string, int>& part_amount_map) {
    map<string, queue<InputRecord>> part_queues; // Map each part to its FIFO queue

    for (const auto& row : rows) {
        string part_name;
        int amount, price;
        string type_io;

        // Unpack the row
        tie(part_name, price, amount, type_io) = row;

        if (type_io == "INPUT") {
            // Add input to the queue for the corresponding part
            part_queues[part_name].push({amount, price});
            part_amount_map[part_name] += amount;

        } else if (type_io == "OUTPUT") {
            part_amount_map[part_name] -= amount;

            int remaining_output = amount;

            while (remaining_output > 0 && !part_queues[part_name].empty()) {
                auto& front = part_queues[part_name].front();

                if (front.amount > remaining_output) {
                    // Partial output from the current input batch
                    part_price_map[part_name] += remaining_output * (price - front.price);
                    front.amount -= remaining_output; // Deduct from the current input batch
                    remaining_output = 0; // Output is fully satisfied
                } else {
                    // Full output from the current input batch
                    part_price_map[part_name] += front.amount * (price - front.price);
                    remaining_output -= front.amount; // Reduce the remaining output
                    part_queues[part_name].pop(); // Remove the current input batch
                }
            }

            if (remaining_output > 0) {
                cerr << "Warning: Output exceeds available stock for part " << part_name << endl;
            }
        }
    }
}

vector<tuple<string, int, int, string>> parseCSV(string csvFileName) {
    vector<tuple<string, int, int, string>> rows;

    ifstream file(csvFileName);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << csvFileName << endl;
        return rows; // Return an empty vector
    }

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string part_name, price_str, amount_str, type_io;

        // Read and split by ',' into the respective fields
        if (!getline(ss, part_name, ',') ||
            !getline(ss, price_str, ',') ||
            !getline(ss, amount_str, ',') ||
            !getline(ss, type_io, ',')) {
            cerr << "Error: Malformed line in CSV: " << line << endl;
            continue;
        }

        try {
            // Convert price and amount to integers
            int price = stoi(price_str);
            int amount = stoi(amount_str);

            // Add the parsed row to the vector
            rows.emplace_back(part_name, price, amount, type_io);
        } catch (const exception& e) {
            cerr << "Error: Invalid data in line: " << line << " (" << e.what() << ")" << endl;
            continue;
        }
    }

    file.close();
    return rows;
}

template<typename K, typename V>
vector<K> getKeys(const map<K, V>& m) {
    vector<K> keys;
    for (const auto& pair : m) {
        keys.push_back(pair.first);
    }
    return keys;
}

int main(int argc, char* argv[]) {

    // store == city

    int read_pipe = stoi(argv[1]);
    int write_pipe = stoi(argv[2]);

    vector<string> namedPipe_paths;
    int i = 3; 

    // getting the namedPipes
    while (argv[i] != nullptr) {
        namedPipe_paths.push_back(argv[i]);
        i++;
    }

    // read the coresponding csv file path from main
    char buffer_store_csv_file_path[MAX_BUFFER_SIZE];
    int bytesRead = read(read_pipe, buffer_store_csv_file_path, sizeof(buffer_store_csv_file_path));
    string store_name = extractName(buffer_store_csv_file_path);
    if (bytesRead == -1) {
        perror("read failed");
        return 1;
    }
    else {
        cout << "Store: store csv file name recieved,  " << store_name << endl;  // log
    }

    // reading the part names
    char buffer_part_names[MAX_BUFFER_SIZE];
    int bytesRead = read(read_pipe, buffer_part_names, sizeof(buffer_part_names));
    vector<string> part_names = stringToVector(buffer_part_names);
    if (bytesRead == -1) {
        perror("read failed");
        return 1;
    }
    else {
        cout << "Store: store csv file name recieved,  " << store_name << endl;  // log
    }

    map<string, int> part_price_map;
    map<string, int> part_amount_map;


    // The acocunting information gets calculated
    processRows(parseCSV(buffer_store_csv_file_path), part_price_map, part_amount_map);  

    vector<string> part_names = getKeys(part_price_map);

    // send acounting data to part via NamedPipes
    for (auto part_name: part_names) {
        string namedPipe = findPipeNameByPartName(namedPipe_paths, part_name);

        // Open the named pipe for writing
        int pipeFd = open(namedPipe.c_str(), O_WRONLY);
        if (pipeFd == -1) {
            perror(("Failed to open named pipe: " + namedPipe).c_str());
            return;
        }

        int price = part_price_map[part_name]; // Assume part exists in partPriceMap
        int amount = part_amount_map[part_name];

        ostringstream line;
        line << part_name << " " << amount << " " << price << " ";

        string lineStr = line.str();
        if (write(pipeFd, lineStr.c_str(), lineStr.size()) == -1) {
            perror(("Failed to write to named pipe: " + namedPipe).c_str());
        }

    }
    cout << "From Store to all Parts: acounting data has been sent from store,  " << store_name << endl;  // log


    // send accounting data to main via unnamed pipe
    int total_price = 0;
    int total_amount = 0;
    for (auto part: part_names) {
        total_amount += part_amount_map[part];
        total_price += part_price_map[part];
    }
    char buffer_send_line_to_main[MAX_BUFFER_SIZE];
    string line_sending_to_main = to_string(total_amount) + ' ' + to_string(total_price);
    strcpy(buffer_send_line_to_main, line_sending_to_main.c_str());
    int bytesWritten = write(write_pipe, buffer_send_line_to_main, sizeof(buffer_send_line_to_main));

    if (bytesWritten == -1) {
        // Error occurred
        perror("Error writing to pipe");
    }
    else {
        cout << "From Store to Main: acounting data has been sent successfully from store,  " << store_name;
    }

}