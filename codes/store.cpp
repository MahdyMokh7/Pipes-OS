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
#include<algorithm>
#include<mutex>


#define MAX_BUFFER_SIZE 1024
#define UNAMEDPIPE_MSG "     via UnnamedPipe"
#define NAMEDPIPE_MSG "     via NamedPipe"
#define ARGV_MSG "     via Argv"

#define INPUT "input"
#define OUTPUT "output"

#define INFO_MAIN "[info: Main]  "
#define INFO_PART "[info: Part]  "
#define INFO_STORE "[info: Stor]  "



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

mutex mtx;

struct InputRecord {
    int amount;
    float price;
};

void strip(std::string& str) {
    // Remove leading whitespace
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) {
        str.clear(); // If only whitespace, clear the string
        return;
    }

    // Remove trailing whitespace
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    
    // Update the string by keeping only the part between first and last
    str = str.substr(first, last - first + 1);
}

string findPipeNameByPartName(const vector<string>& pipeNames, const string& partName) {
    for (const string& pipeName : pipeNames) {
        // Find the position of the last '/' and the first '.' after it
        size_t slashPos = pipeName.find_last_of('/');
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

bool splitBuffer(const char* buffer, string& buffer_part_names, string& buffer_store_csv_file_path, string& parts_wanted) {
    buffer_part_names.clear();
    buffer_store_csv_file_path.clear();
    parts_wanted.clear();

    int newlineCount = 0;

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        if (buffer[i] == '\n') {
            ++newlineCount;
            continue; 
        }

        if (newlineCount == 0) {
            buffer_part_names += buffer[i];
        } else if (newlineCount == 1) {
            buffer_store_csv_file_path += buffer[i];
        } else if (newlineCount == 2) {
            parts_wanted += buffer[i];
        }
    }

    return newlineCount >= 2; // Return true if at least two delimiters '\n' were found
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
        strip(part);
        partNames.push_back(part);
    }
    return partNames;
}

void processRows(const vector<tuple<string, float, int, string>>& rows, map<string, int>& part_profit_map, map<string, int>& part_amount_map, const vector<string>& part_names, map<string, int>& part_leftover_price_map) {
    map<string, queue<InputRecord>> part_queues; // Map each part to its FIFO queue

    // init the part_price_map so that the without output rows won't have any problem
    for (auto part: part_names) {
        part_profit_map[part] = 0;
    } 

    for (const auto& row : rows) {
        string part_name;
        int amount;
        float price;
        string type_io;

        // Unpack the row
        tie(part_name, price, amount, type_io) = row;
        strip(type_io);

        // cout << INFO_STORE << "rowwwwwwwwwwwwwww, ..   " << part_name << "  " << price << "  " << amount << "  " << type_io << endl ; 

        if (type_io == INPUT) {
            // Add input to the queue for the corresponding part
            part_queues[part_name].push({amount, price});
            part_amount_map[part_name] += amount;
            // cout << INFO_STORE << "omaddddddd inpuuuuuuttttt, ..   " << part_name << "  " << price << "  " << amount << "  " << type_io << endl ; 

        } else { // Output
            part_amount_map[part_name] -= amount;

            int remaining_output = amount;

            while (remaining_output > 0 && !part_queues[part_name].empty()) {
                auto& front = part_queues[part_name].front();

                if (front.amount > remaining_output) {
                    // Partial output from the current input batch
                    part_profit_map[part_name] += (int)((float)remaining_output * (price - front.price));
                    front.amount -= remaining_output; // Deduct from the current input batch
                    remaining_output = 0; // Output is fully satisfied
                } else {
                    // Full output from the current input batch
                    part_profit_map[part_name] += (int)((float)front.amount * (price - front.price));
                    remaining_output -= front.amount; // Reduce the remaining output
                    part_queues[part_name].pop(); // Remove the current input batch
                }
            }

            if (remaining_output > 0) {
                cerr << "Warning: Output exceeds available stock for part " << part_name << endl;
            }
        }
    }

    for (auto part: part_names) {
        queue<InputRecord> temp_queue = part_queues[part]; 
        while (!temp_queue.empty()) {
            InputRecord record = temp_queue.front();
            part_leftover_price_map[part] += (int)(record.price * (float)record.amount);
            temp_queue.pop();
        }
    } 

}

vector<tuple<string, float, int, string>> parseCSV(string csvFileName) {
    vector<tuple<string, float, int, string>> rows;

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
            float price = stof(price_str);
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

bool isStringInVector(const std::vector<std::string>& vec, const std::string& str) {
    return find(vec.begin(), vec.end(), str) != vec.end();
}

void printStringElements(const std::vector<std::string>& vec) {
    for (const auto& str : vec) {
        cout << str << " ";
    }
}

int main(int argc, char* argv[]) {

    // store == city

    int read_pipe = stoi(argv[1]);
    int write_pipe = stoi(argv[2]);
    // cout << INFO_STORE << "read_pipe  " << read_pipe << endl;
    // cout << INFO_STORE << "write pipe  " << write_pipe << endl;

    vector<string> namedPipe_paths;
    int i = 3; 

    // getting the namedPipes
    while (argv[i] != nullptr) {
        namedPipe_paths.push_back(string(argv[i]));
        // cout << INFO_STORE  << "namedPipe_paths  " <<  string(argv[i]) << endl;//////
        strip(namedPipe_paths.back());
        i++;
    }

    // read the coresponding csv file path from main
    char buffer[MAX_BUFFER_SIZE];
    string part_names_str;
    string store_csv_file_path_str;
    string parts_wanted_str;

    int bytesRead = read(read_pipe, buffer, sizeof(buffer));
    splitBuffer(buffer, part_names_str, store_csv_file_path_str, parts_wanted_str);
    strip(part_names_str);
    strip(store_csv_file_path_str);
    strip(parts_wanted_str);

    string store_name = extractName(store_csv_file_path_str);
    vector<string> part_names = stringToVector(part_names_str);
    vector<string> parts_wanted = stringToVector(parts_wanted_str);

    // cout << INFO_STORE << "buffer " << buffer << endl;
    // cout << INFO_STORE << "buffer aprt name  " << buffer_part_names << endl;
    // cout << INFO_STORE << "buffer store csv file path  " << buffer_store_csv_file_path << endl;
    // cout << INFO_STORE << "store name  " << store_name << endl;  
    if (bytesRead == -1) {
        perror("read failed");
        return 1;
    }
    else {
        // lock_guard<mutex> lock(mtx);
        cout << INFO_STORE << "Store: " << store_name << endl;
        cout << INFO_STORE << "read pipe,  " << read_pipe << ARGV_MSG <<endl;
        cout << INFO_STORE << "write pipe,  " << write_pipe << ARGV_MSG << endl;
        cout << INFO_STORE << "named pipe paths,  ";
        printStringElements(namedPipe_paths);  // namedPipe paths
        cout << INFO_STORE << ARGV_MSG << endl;
        cout << INFO_STORE << "part names recieved,  " << part_names_str << UNAMEDPIPE_MSG << endl;  // log
        cout << INFO_STORE << "store csv file name recieved,  " << store_name << UNAMEDPIPE_MSG << endl;  // log
        cout << endl;
    }

    map<string, int> part_profit_map;
    map<string, int> part_amount_map;
    map<string, int> part_leftover_price_map;


    // The acocunting information gets calculated
    processRows(parseCSV(store_csv_file_path_str), part_profit_map, part_amount_map, part_names, part_leftover_price_map);  

    int total_valid = 0;
    // send acounting data to part via NamedPipes
    for (auto part_name: part_names) {
        string namedPipe = findPipeNameByPartName(namedPipe_paths, part_name);

        // Open the named pipe for writing
        // cout  << INFO_STORE <<"openinggggggggg"<< namedPipe << endl;
        int pipeFd = open(namedPipe.c_str(), O_WRONLY);
        if (pipeFd == -1) {
            perror(("Failed to open named pipe: " + namedPipe).c_str());
        }
        else {
            cout << INFO_STORE << "namedPipe " << namedPipe  << "successfully opened for reading" << endl;  // log
        }

        int profit = part_profit_map[part_name]; 
        int amount = part_amount_map[part_name];
        int leftover_price = part_leftover_price_map[part_name];

        ostringstream line;
        line << store_name << " " << amount << " " << leftover_price << "\n";   ///////// why is the amount and the price values zero ////////////////

        string lineStr = line.str();
        // cout << INFO_STORE << "line sent to " << part_name << " and the line is,  " << lineStr << endl;///////////////////////
        // sending data to part.cpp
        if (write(pipeFd, lineStr.c_str(), lineStr.size()) == -1) {
            perror(("Failed to write to named pipe: " + namedPipe).c_str());
        }
        else {
            total_valid++;
        }

    }
    if (total_valid == part_names.size()) {
    //    cout << INFO_STORE << "From Store to all Parts: acounting data has been sent successfully from store,  " << store_name << endl;  // log
    }
    else {
        cerr << "From Store to all Parts:  acounting data sent FAILED " << endl;  
    }

    // send accounting data to main via unnamed pipe
    int total_profit = 0;
    int total_amount = 0;

    for (const auto& part_name : part_names) {
        if (isStringInVector(parts_wanted, part_name)) {
            total_amount += part_amount_map.at(part_name); // Use at() for safety
            total_profit += part_profit_map.at(part_name);
        }
    }

    ostringstream line;
    line << store_name << " " << total_amount << " " << total_profit << "\n";

    string lineStr = line.str();
    // cout << INFO_STORE << "line sent to main and the line is: " << lineStr << endl;

    if (write(write_pipe, lineStr.c_str(), lineStr.size()) == -1) {
        cerr << "Failed to write to pipe";
    }
    else {
        // cout << INFO_STORE << "From Store to Main: acounting data has been sent successfully from store,  " << store_name << endl;  //log
    }

    cout << INFO_STORE << "store " << store_name << ": process finished successfully" << endl;

    return 0;

    //////Done
}