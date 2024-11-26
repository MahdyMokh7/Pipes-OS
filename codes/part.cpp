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


#define MAX_BUFFER_SIZE 1024
#define UNAMEDPIPE_MSG "     via UnnamedPipe"
#define NAMEDPIPE_MSG "     via NamedPipe"
#define ARGV_MSG "     via Argv"

using namespace std;


/*
send the acounting data of the Part to the Main by the following foramt:
    in a single line 
    1. first part name
    2. second element of the line: total_leftovers
    3. third element of the line: total_priceBenefit

*/
bool splitBuffer(const char* buffer, string &buffer_part_name, string &buffer_number_of_stores) {
    buffer_part_name.clear();
    buffer_number_of_stores.clear();
    bool foundNewline = false;

    for (size_t i = 0; buffer[i] != '\0'; ++i) {
        if (buffer[i] == '\n') {
            foundNewline = true;
            continue; // Skip the newline character
        }

        if (!foundNewline) {
            buffer_part_name += buffer[i];
        } else {
            buffer_number_of_stores += buffer[i];
        }
    }

    return foundNewline; // Return true if the delimiter '\n' was found
}

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

void processPipeData(const string& buffer, const string& part_name_str, int& process_store_count, int& total_leftovers, int& total_benefitPrice) {
    stringstream ss(buffer);
    string line;

    while (getline(ss, line, '\n')) {
        if (!line.empty()) {
            stringstream lineStream(line);
            string store_name, leftover_str, price_str;

            // Parse the data in the line
            if (getline(lineStream, store_name, ' ') && 
                getline(lineStream, leftover_str, ' ') && 
                getline(lineStream, price_str, ' ')) {
                
                // Convert to appropriate types
                int leftover = stoi(leftover_str);
                int price = stoi(price_str);
                strip(store_name);

                // Update totals
                total_leftovers += leftover;
                total_benefitPrice += price;

                // Increment the processed count
                process_store_count++;

                // Log the data
                cout << "Part " << part_name_str << ": received accounting data from store(city), " 
                     << store_name << ".  " << "leftover:  " << leftover << "  price:  " << price <<  endl;  // log

            }
            else {
                cerr << "Error: Failed to parse store data for line: " << line << endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {

    int read_pipe = stoi(argv[1]);
    int write_pipe = stoi(argv[2]);
    string pipeName = string(argv[3]);
    strip(pipeName);
    // cout << "readpipe " << read_pipe << endl;
    // cout << "writepipe " << write_pipe  << endl;
    // cout << "Pipename " << pipeName << endl;

    // Read from the pipe (parent to child) [part name]
    char buffer[MAX_BUFFER_SIZE];
    string part_name_str;
    string num_of_stores_str;
    int bytesRead = read(read_pipe, buffer, sizeof(buffer));
    // cout << "why? " << buffer << endl;  ///////////////////////////////////

    // cout << "start" << endl;
    bool flag_recieve = splitBuffer(buffer, part_name_str, num_of_stores_str);
    int num_of_stores;
    // cout << "end" << endl;
    if (bytesRead == -1 && !flag_recieve) {
        perror("read failed");
        return 1;
    }
    else {
        strip(part_name_str);
        num_of_stores = stoi(num_of_stores_str);
        cout << "Part: " << part_name_str << endl;
        cout << "read pipe,  " << read_pipe << ARGV_MSG << endl;
        cout << "write pipe,  " << write_pipe << ARGV_MSG << endl;
        cout << "part name recieved,  " << part_name_str << UNAMEDPIPE_MSG << endl;  // log
        cout << "stores size recieved,  " << num_of_stores << UNAMEDPIPE_MSG << endl;  // log
        cout << endl;
    }


    int total_leftovers = 0;   
    int total_benefitPrice = 0;

    // Loop to read data from the named pipe for each store
    int process_store_count = 0;
    while(process_store_count < num_of_stores) {
        // Open the named pipe for reading
        int namedPipe_fd = open(pipeName.c_str(), O_RDONLY);
        if (namedPipe_fd == -1) {
            perror("Failed to open named pipe");
            return 1;
        }
        // else {
        //     cout << "namedPipe " << pipeName  << "successfully opened for reading" << endl;  // log
        // }

        // Buffer to store data from the named pipe
        char buffer_store_data[MAX_BUFFER_SIZE];
        memset(buffer_store_data, 0, sizeof(buffer_store_data));  // Clear the buffer

        // Read data from the named pipe
        int bytesRead = read(namedPipe_fd, buffer_store_data, sizeof(buffer_store_data));
        if (bytesRead == -1) {
            perror("Failed to read from named pipe");
            close(namedPipe_fd);
            return 1;
        }   

        // process data read from store (we didnt do the read exactly the number of stores becuase multiple writes maybe read by only one read)
        processPipeData(buffer_store_data, part_name_str, process_store_count, total_leftovers, total_benefitPrice);
    }

    string total_leftovers_str = to_string(total_leftovers);
    string total_benefitPrice_str = to_string(total_benefitPrice);

    string accounting_data = part_name_str + " " + total_leftovers_str + " " + total_benefitPrice_str;

    // Write accounting data to the write pipe
    int bytesWritten = write(write_pipe, accounting_data.c_str(), accounting_data.length() + 1);
    if (bytesWritten == -1) {
        perror("Failed to write accounting data to pipe");
        return 1;
    }
    else {
        // cout << "From Part to Main: sending accounting Data of part, " << part_name_str << endl;  // log
    }

    return 0;

    //////////////////Done 
}