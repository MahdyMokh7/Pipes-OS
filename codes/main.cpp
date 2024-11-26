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
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <unistd.h> 
#include <algorithm>
#include <iostream> 
#include <string>   
#include <vector>   
#include <cstdlib>  
#include <fcntl.h> 


#define PART_EXE_FILE "./part.out"
#define STORE_EXE_FILE "./store.out"

#define MAX_BUFFER_SIZE 1024
#define UNAMEDPIPE_MSG "     via UnnamedPipe"
#define NAMEDPIPE_MSG "     via NamedPipe"
#define ARGV_MSG "     via Argv"

using namespace std;


/* For each part format of recieved data
    string line
    1. part name
    2. total leftovers
    3. total benefit price

*/

/* For each store format of recieved data
    string line
    1. total leftovers
    2. total benefit price

*/


const string STORES_DIR_PATH = "../stores/";
const string PARTS_DIR_PATH = STORES_DIR_PATH + "Parts.csv";
const string DIR_NAMED_PIPES = "../PipedFolder/";

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

vector<vector<string>> readCSV(const string& filename) {
    ifstream file(filename);
    vector<vector<string>> data;
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string value;
        vector<string> row;

        while (getline(ss, value, ',')) {
            row.push_back(value);
        }
        data.push_back(row);
    }
    return data;
}

void print_all_part_names(vector<string> part_names) {
    int i = 0;
    cout << "Part names: " << endl;
    for (auto part: part_names) {
        i++;
        cout << i << ". " << part << endl;
    }
    cout << endl;
}

void print_all_file_names(vector<string> file_names) {
    int i = 0;
    cout << "CSV File names: " << endl;
    for (auto file: file_names) {
        i++;
        cout << "+ " << file << endl;
    }
    cout << endl;
}

vector<string> get_csv_file_paths(const string& directoryPath) {
    vector<string> csvFiles;

    try {
        for (const auto& entry : filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                string fileName = entry.path().filename().string();
                if (fileName != "Parts.csv") {
                    csvFiles.push_back(STORES_DIR_PATH + fileName);
                }
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return csvFiles;
}

vector<string> createNamedPipes(const std::vector<std::string>& parts) {

    string directory = DIR_NAMED_PIPES;
    vector<string> pipeNames;

    // Check if the directory exists; if not, create it
    struct stat st;
    if (stat(directory.c_str(), &st) == -1) {
        if (mkdir(directory.c_str(), 0777) == -1) {
            perror("mkdir"); 
            return pipeNames;
        }
        cout << "Directory " << directory << " has been created." << endl;
    }

    for (const auto& part : parts) {
        string pipeName = "../PipedFolder/" + part + ".pipe"; // Named pipe path for each part

        if (mkfifo(pipeName.c_str(), 0666) == -1) {
            perror("mkfifo"); 
        }
        else {
            pipeNames.push_back(pipeName);
            cout << "NamedPipe " << pipeName << " has been Created." << endl;  // log
        }
    }
    cout << endl;
    return pipeNames;
}

vector<int> get_part_indices() {
    cout << "Which parts do you want to see their analytics? Enter the numbers separated by spaces(e.g., 1 3 4)" << endl;

    // you have to -1 the int values
    string line;
    cin >> line;

    vector<int> indices;
    istringstream stream(line);
    int number;

    // Extract integers from the string stream
    while (stream >> number) {
        indices.push_back(number-1);  // index should start from 0
    }

    return indices;
}

string vectorToString(vector<string> partNames) {
    string result;
    for (const string& part : partNames) {
        result += part + " ";  // Add each part name followed by a space
    }
    // Remove trailing space at the end (optional)
    if (!result.empty()) {
        result.pop_back();
    }
    return result;
}

bool containsString(const std::vector<std::string>& vec, const std::string& target) {
    return find(vec.begin(), vec.end(), target) != vec.end();
}

vector<string> getSubsetFromIndices(const vector<int>& indices, const vector<std::string>& strVec) {
    vector<std::string> subset;

    for (int index : indices) {
        if (index >= 0 && index < strVec.size()) {  
            subset.push_back(strVec[index]);
        }
    }

    return subset;
}

bool checkAllProcessesExitedSuccessfully(const vector<int>& pids) {
    bool allExitedSuccessfully = true;

    for (int pid : pids) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            cerr << "Error waiting for process " << pid << endl;
            allExitedSuccessfully = false;
            continue;
        }

        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            if (exitCode != 0) {
                cerr << "Process " << pid << " exited with error code " << exitCode << endl;
                allExitedSuccessfully = false;
            }
        } else {
            cerr << "Process " << pid << " did not terminate normally" << endl;
            allExitedSuccessfully = false;
        }
    }

    return allExitedSuccessfully;
}

void closeAndRemoveNamedPipes(const vector<string>& namedPipes) {
    for (const string& pipePath : namedPipes) {
        // Remove the named pipe
        if (unlink(pipePath.c_str()) == -1) {
            perror(("Failed to remove named pipe: " + pipePath).c_str());
        } else {
            cout << "Removed named pipe: " << pipePath << endl;
        }
    }
}

bool isPollfdInVector(const std::vector<pollfd>& pfds, const pollfd& searchFd) {
    for (const auto& pfd : pfds) {
        if (pfd.fd == searchFd.fd) {
            return true; // Found the pollfd in the vector
        }
    }
    return false; // pollfd not found in the vector
}

void processStores(int fd, int& total_profit) {
    char buffer_received_from_store[256];
    memset(buffer_received_from_store, 0, sizeof(buffer_received_from_store));

    ssize_t bytesRead = read(fd, buffer_received_from_store, sizeof(buffer_received_from_store) - 1);
    if (bytesRead <= 0) {
        cerr << "Error reading from store pipe or EOF" << endl;
        return;
    }

    buffer_received_from_store[bytesRead] = '\0'; // Null-terminate the string
    istringstream iss(buffer_received_from_store);
    int leftovers, benefit;

    if (!(iss >> leftovers >> benefit)) {
        cerr << "Malformed data from store: " << buffer_received_from_store << endl;
    }

    total_profit += benefit;
}

void processParts(int fd2, vector<string> user_wanted_part_names) {
    char buffer_received_from_part[256];
    memset(buffer_received_from_part, 0, sizeof(buffer_received_from_part));

    ssize_t bytesRead = read(fd2, buffer_received_from_part, sizeof(buffer_received_from_part) - 1);
    if (bytesRead <= 0) {
        cerr << "Error reading from part pipe or EOF" << endl;
        return;
    }

    buffer_received_from_part[bytesRead] = '\0'; // Null-terminate the string
    istringstream iss(buffer_received_from_part);
    string part_name;
    int leftovers, benefit;

    if (!(iss >> part_name >> leftovers >> benefit)) {
        cerr << "Malformed data from part: " << buffer_received_from_part << endl;
    }
    strip(part_name);

    // return the total quantity and price of the wanted part////////////////////////////
    // if (containsString(user_wanted_part_names, part_name)) {    // log (answer)
        cout << part_name << ": " << endl;
        cout << "total leftover Quantity: " << leftovers << endl;
        cout << "total leftover Price: " << benefit << endl;     
    // }
}

int main(int argc, char const *argv[]) {

    // read the csv file names dynamically 
    vector<string> stores_csv_file_paths = get_csv_file_paths(STORES_DIR_PATH);
    // print_all_file_names(stores_csv_file_paths);  // log /////////////////////////////
    
    // read Parts.csv file and print the part names
    vector<vector<string>> read_csv = readCSV(PARTS_DIR_PATH);  
    vector<string> part_names = read_csv[0];
    print_all_part_names(part_names); // log //////////////////////////////////

    // get the parts that the user wants to see its analytics
    vector<int> indices = get_part_indices();
    vector<string> namedPipes = createNamedPipes(part_names);

    vector<int> pids;
    vector<pollfd> pfds_read_store;
    vector<pollfd> pfds_read_part;
    vector<pollfd> total_pfds;

    // part processes
    int i = 0;
    for (auto part: part_names) {

        int parent_to_child_pipe[2];
        int child_to_parent_pipe[2];

        // create the unnamed pipes
        if (pipe(child_to_parent_pipe) == -1 || pipe(parent_to_child_pipe) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {

            // close the unnecessary endpoints of the pipe
            close(child_to_parent_pipe[0]);
            close(parent_to_child_pipe[1]);

            // convert pipes to string to pass the args exec file
            char read_pipe[10], write_pipe[10];
            // cout <<"read pipe of part: "<< parent_to_child_pipe[0] << endl;
            sprintf(read_pipe, "%d", parent_to_child_pipe[0]);
            sprintf(write_pipe, "%d", child_to_parent_pipe[1]);
            string fifo_name = namedPipes[i];
            // cout << "namedpipes[i] :  " << fifo_name << endl; /////////////////////// 

            // exec system call
            execlp(PART_EXE_FILE, "part", read_pipe, write_pipe, fifo_name.c_str(), nullptr);

        }
        else {
            pids.push_back(pid);
            pfds_read_part.push_back(pollfd{child_to_parent_pipe[0], POLLIN, 0});

            // Close the read end of the pipe
            close(parent_to_child_pipe[0]);
            close(child_to_parent_pipe[1]);

            // send part name and store size to part.cpp; 
            string msg = part + "\n" + to_string(stores_csv_file_paths.size());
            if (write(parent_to_child_pipe[1], msg.c_str(), strlen(msg.c_str()) + 1) == -1) {
                perror("write to pipe failed for part name and store size");
            }
            else {
                // cout << "From Main to Part: sending part name, " << part << endl;  // log//////////////////////////
                // cout << "From Main to Part: sending stores size, " << to_string(stores_csv_file_paths.size()).c_str() << endl;  // log////////////////////////
            }

            // int status;
            // waitpid(pid, &status, 0);  // Wait for the child process to finish /////////////////////////////////////////////////
            
        }

        // unnamed pipes will be closed when the child process ends
        i++;
    }

    i = 0;
    for (auto store_csv_file_path: stores_csv_file_paths) {

        int parent_to_child_pipe[2];
        int child_to_parent_pipe[2];

        // create the unnamed pipes
        if (pipe(child_to_parent_pipe) == -1 || pipe(parent_to_child_pipe) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {

            // close the unnecessary endpoints of the pipe
            close(child_to_parent_pipe[0]);
            close(parent_to_child_pipe[1]);

            // convert pipes to string to pass the args exec file
            char read_pipe[10], write_pipe[10];
            sprintf(read_pipe, "%d", parent_to_child_pipe[0]);
            sprintf(write_pipe, "%d", child_to_parent_pipe[1]);

            // Prepare the arguments for execlp
            vector<const char*> args;
            args.push_back("store");
            args.push_back(read_pipe);
            args.push_back(write_pipe);

            // Add all named pipes to the arguments
            for (const auto& pipe : namedPipes) {
                args.push_back(pipe.c_str());
            }

            // Add a nullptr to mark the end of the arguments
            args.push_back(nullptr);

            // Use execvp to execute the command with the arguments
            execvp(STORE_EXE_FILE, const_cast<char* const*>(args.data()));

        }
        else {

            pids.push_back(pid);
            pfds_read_store.push_back(pollfd{child_to_parent_pipe[0], POLLIN, 0});

            // Close the read end of the pipe
            close(parent_to_child_pipe[0]);


            // send store name to store.cpp  // send the part names so that in each part calculation
            string temp_part_names = vectorToString(part_names);
            string msg = temp_part_names + "\n" + store_csv_file_path;
            int bytesWritten = write(parent_to_child_pipe[1], msg.c_str(), strlen(msg.c_str()) + 1);
            if(bytesWritten < 0) {
                cerr << "COULDNT send data via pipe in store" << endl;
            }
            else {
                // cout << "From Main to Store: sending store csv file path, " << store_csv_file_path << endl;  // log  /////////////////////////////
            }
            
            
        }

        // unnamed pipes will be closed when the child process ends
        i++;
    }

    total_pfds.insert(total_pfds.end(), pfds_read_part.begin(), pfds_read_part.end());
    total_pfds.insert(total_pfds.end(), pfds_read_store.begin(), pfds_read_store.end());

    // processing all data
    int total_profit = 0;
    for (int j = 0; j < stores_csv_file_paths.size() + part_names.size(); j++) {
        if (poll(total_pfds.data(), (nfds_t)(total_pfds.size()), -1) == -1) 
            write(2, "FAILED: Poll in client\n", strlen("FAILED: Poll in client\n"));
        
        for (int i = 0; i < total_pfds.size(); i++) {
            if(total_pfds[i].revents & POLLIN) {
                if (isPollfdInVector(pfds_read_store, total_pfds[i]))
                processStores(pfds_read_store[i].fd, total_profit);
            }
            else {
                processParts(total_pfds[i].fd, getSubsetFromIndices(indices, part_names));
            }
        }

    }

    // Total Profit
    cout << "Total profit of all stores:  " << total_profit << endl; // log (answer)

    // wait for all the child processes to end
    bool all_exited_successfully = checkAllProcessesExitedSuccessfully(pids);
    if (all_exited_successfully) {
        cout << "All children processes exited successfully." << endl;
    } else {
        cout << "Some children processes failed or did not exit properly." << endl;
    }

    // close all named pipes (function)
    // remove all named pipe (function)
    // closeAndRemoveNamedPipes(namedPipes);    

    return 0;
}
