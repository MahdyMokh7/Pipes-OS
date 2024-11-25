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


#define MAX_BUFFER_SIZE 256

using namespace std;


/*
send the acounting data of the Part to the Main by the following foramt:
    in a single line 
    1. first part name
    2. second element of the line: total_leftovers
    3. third element of the line: total_priceBenefit

*/

int main(int argc, char* argv[]) {

    int read_pipe = stoi(argv[1]);
    int write_pipe = stoi(argv[2]);
    string pipeName = argv[3];

    // Read from the pipe (parent to child) [part name]
    char buffer_part_name[MAX_BUFFER_SIZE];
    int bytesRead = read(read_pipe, buffer_part_name, sizeof(buffer_part_name));

    if (bytesRead == -1) {
        perror("read failed");
        return 1;
    }
    else {
        cout << "Part: part name recieved,  " << buffer_part_name << endl;  // log
    }


    // Read from the pipe (parent to child) [number of stores]
    char buffer_number_of_stores[MAX_BUFFER_SIZE];
    int bytesRead = read(read_pipe, buffer_number_of_stores, sizeof(buffer_number_of_stores));

    if (bytesRead == -1) {
        perror("read failed");
        return 1;
    }
    else {
        cout << "Part: number of stores recieved,  " << buffer_part_name << endl;  // log
    }

    int num_of_stores = stoi(buffer_number_of_stores);
    int total_leftovers ;   
    int total_benefitPrice;



    // Loop to read data from the named pipe for each store
    for (int i = 0; i < num_of_stores; i++) {
        // Open the named pipe for reading
        int namedPipe_fd = open(pipeName.c_str(), O_RDONLY);
        if (namedPipe_fd == -1) {
            perror("Failed to open named pipe");
            return 1;
        }

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

        stringstream ss(buffer_store_data);
        string store_name, leftover_str, price_str;

        // Read the store_name, leftover, price
        if (getline(ss, store_name, ' ') && getline(ss, leftover_str, ' ') && getline(ss, price_str, ' ')) {
            // Convert the string elements to appropriate data types
            int leftover = stoi(leftover_str);  // Convert leftover to float
            int price = stoi(price_str);        // Convert price to float
            total_leftovers += leftover;
            total_benefitPrice += price;

            // Log the data for each store
            cout << "Part " << buffer_part_name << ": recieved accounting data from store(city),  " << store_name << endl;   // log
        } else {
            cerr << "Error: Failed to parse store data for store " << (i + 1) << endl;
        }
    }

    string total_leftovers_str = to_string(total_leftovers);
    string total_benefitPrice_str = to_string(total_benefitPrice);

    string accounting_data = string(buffer_part_name) + " " + total_leftovers_str + " " + total_benefitPrice_str;

    // Write accounting data to the write pipe
    int bytesWritten = write(write_pipe, accounting_data.c_str(), accounting_data.length() + 1);
    if (bytesWritten == -1) {
        perror("Failed to write accounting data to pipe");
        return 1;
    }
    else {
        cout << "From Part to Main: sending accounting Data of part, " << buffer_part_name << endl;
    }

    return 0;

    //////////////////Done 
}