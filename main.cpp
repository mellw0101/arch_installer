#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

namespace tools
{
    int pacman(vector<string> packages)
    {
        string packages_string = "sudo pacman -S --noconfirm ";
        for (const string & part : packages)
        {
            packages_string += part + " ";
        }
    
        packages_string.erase(packages_string.length() - 1);
        int status = std::system(packages_string.c_str());
        if (status != 0)
        {
            cerr << "system call failed" << endl;
        }

        return status;
    }

    int pacstrap(vector<string> packages)
    {
        string packages_string = "pacstrap -i /mnt ";
        for (const string & part : packages)
        {
            packages_string += part + " ";
        }
        
        packages_string.erase(packages_string.length() - 1);
        int status = std::system(packages_string.c_str());
        if (status != 0)
        {
            cerr << "system call failed" << endl;
        }
        
        return status;
    }

    string get_responce()
    {
        string tmp;
        std::cin >> tmp;
        return tmp;
    }

    // Function to connect to a WiFi network using iwctl with a password
    bool connectToWiFi(const string & ssid, const string & password)
    {
        string command = "iwctl station wlan0 connect \"" + ssid + "\"";
        
        if (!password.empty())
        {
            command += " passphrase \"" + password + "\"";
        }
        
        // Run the iwctl command to connect
        int result = std::system(command.c_str());

        if (result == 0)
        {
            cout << "Connected to WiFi network: " << ssid << endl;
            return true;
        }
        else
        {
            cerr << "Failed to connect to WiFi network: " << ssid << endl;
            return false;
        }
    }
}

namespace drive
{
    void selector()
    {
        system("lsblk");
        cout << "\nWhere do you want to install arch?: ";
        cout << tools::get_responce() << "\n";
    }
}


#include <array>
#include <charconv>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <curses.h>
#include <iostream>
#include <string>
#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <fstream>
#include <filesystem>
#include <ext2fs/ext2fs.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <vector>
#include <thread>
#include <sstream>
#include <chrono>
#include <openssl/sha.h>
#include <iomanip>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <zlib.h>
#include <regex>


using namespace std;
namespace fs = std::filesystem;

namespace syscommand {
    const string lsblk = "/usr/bin/lsblk";
    const string clear = "/usr/bin/clear";
    const string mkfs_ext4 = "/usr/bin/mkfs.ext4";
    const string pacman = "/usr/bin/pacman";
    const string sleep = "/usr/bin/sleep";
    // const string sed = "/usr/bin/sed";
    const string systemctl = "/usr/bin/systemctl";
    const string cfdisk = "/usr/bin/cfdisk";
    const string mkfs_fat = "/usr/bin/mkfs.fat";
    const string sys_mount = "/usr/bin/mount";
    const string sudo = "/usr/bin/sudo";
    const string git = "/usr/bin/git";
    const string chown = "/usr/bin/chown";
    const string makepkg = "/usr/bin/makepkg";
    const string umount = "/usr/bin/umount";
    const string pacstrap = "/usr/bin/pacstrap";
}
namespace gbvar {
    string main_drive_name;
    string rootpass;
    string Username_start;
    string pass_start;
    string gpu_pick;
    string de_pick;
    string Username;
    std::string x_or_w;
    char a;
}
using namespace gbvar;
namespace tools {
    void error_message(const std::string& program, const std::string& message) {
        std::cerr << program + ": ERROR: " << message << " (press enter to continiue)";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    void runProgram(const string& program) {
        if (fork() == 0) {
            // Child process
            execlp(program.c_str(), program.c_str(), nullptr);
            perror("execlp failed"); // Print an error message if execlp fails
            exit(1);
        } else {
            // Parent process
            wait(NULL); // Wait for the child process to finish
        }
    }
    void runProgramWargs(const string &program, const vector<string> &args) {
        vector<char *> argv;
        for (const string &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        if (fork() == 0) {
            // Child process
            execvp(program.c_str(), argv.data());
            perror("execvp failed"); // Print an error message if execvp fails
            exit(1);
        } else {
            // Parent process
            wait(NULL); // Wait for the child process to finish
        }
    }
    void runProgramWithInput(const string& program, const vector<string>& args, const string& input) {
        vector<char*> argv;
        for (const string& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            exit(1);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(1);
        }

        if (pid == 0) {
            // Child process
            close(pipefd[1]);  // Close the write end of the pipe

            // Redirect stdin to read from the pipe
            if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                perror("dup2 failed");
                exit(1);
            }

            close(pipefd[0]);  // Close the read end of the pipe

            // Execute the program
            execvp(program.c_str(), argv.data());
            perror("execvp failed");
            exit(1);
        } else {
            // Parent process
            close(pipefd[0]);  // Close the read end of the pipe

            // Write the input to the child process's stdin through the pipe
            if (write(pipefd[1], input.c_str(), input.length()) == -1) {
                perror("write failed");
                exit(1);
            }

            close(pipefd[1]);  // Close the write end of the pipe

            // Wait for the child process to finish
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                int exitStatus = WEXITSTATUS(status);
                std::cout << "Child process exited with status: " << exitStatus << std::endl;
            }
        }
    }
    bool bash(const string& command, const string& input) {
        FILE* pipe = popen(command.c_str(), "w");
        if (!pipe) return false;
            fprintf(pipe, "%s", input.c_str());
            fflush(pipe);
            pclose(pipe);
            return true;
    }
    void input_to_file(const std::string& input, const std::string& directory, const std::string& FILEname) {
        std::string full_path = directory + "/" + FILEname;
        if (fs::exists(directory) && fs::is_directory(directory)) {
            std::cout << "input_to_file: Directory: '" << directory << "' is a directory\n";
            if (fs::exists(full_path) && fs::is_directory(full_path)) {
                error_message("input_to_file", "target file '" + FILEname + "' is a directory");
                return;
            }
            if (fs::exists(full_path)) {
                std::cout << "input_to_file: target file '" << full_path << "' exists\n";
                std::ofstream openFILE;
                openFILE.open(full_path, std::ios::app);
                if (openFILE.is_open()) {
                    openFILE << input << std::endl;
                    openFILE.close();
                    std::cout << "input_to_file: successfully appended '" << input << "' to '" << full_path << "'\n";
                } else {
                    error_message("input_to_file", "cant open '" + FILEname + "'");
                    return;
                }
            } else {
                error_message("input_to_file", "target file '" + full_path + "' does not exist");
                return;
            }
        } else if (fs::exists(directory)) {
            error_message("input_to_file", "'" + directory + "' is not a directory it is a file");
            return;
        }
        else {
            error_message("input_to_file", "Directory '" + directory + "' does not exist");
            return;
        }
    }
    void create_file(const std::string& full_PATH_to_file) {
        if (fs::exists(full_PATH_to_file)) {
            if (fs::is_directory(full_PATH_to_file)) {
                error_message("create_file", "target file '" + full_PATH_to_file + "' is a directory");
                return;
            } else {
                error_message("create_file", "target file '" + full_PATH_to_file + "' already exist");
                return;
            }
        } else {
            std::ofstream outFile;
            outFile.open(full_PATH_to_file, std::ios::out | std::ios::trunc);
            if (!outFile.is_open()) {
                error_message("create_file", "Failed to create new file at target '" + full_PATH_to_file + "'");
                return;
            } else {
                outFile.close();
                std::cout << "create_file: created empty file at '" << full_PATH_to_file << "'\n";
                return;
            }
        }
    }
    void export_from_file(const std::string& FILEname, std::string& evar) {
        if (fs::exists(FILEname)) {                         // if target file exist
            if (!fs::is_directory(FILEname)) {              // if target file is not a directory
                if (!fs::is_empty(FILEname)) {              // if target file is not empty
                    std::ifstream inFile;
                    inFile.open(FILEname);
                    if (inFile.is_open()) {                 // if function was able to open file
                        std::cout << "export_from_file: successfully opened target file '" + FILEname + "'\n";
                        std::string var;
                        std::stringstream buffer;
                        while (getline(inFile, var)) {
                            buffer << var << '\n';
                        }
                        evar = buffer.str();
                        if (!evar.empty()) {
                            std::cout << "export_from_file: successfully read file '" + FILEname + "'\n";
                        }
                        inFile.close();
                    } else {                                // if function was unable to open file
                        error_message("export_from_file", "cant open '" + FILEname + "'");
                    }
                } else {                                    // if target file is empty
                    error_message("export_from_file", "target file '" + FILEname + "' is empty");
                }
            } else {                                        // if target file is a directory
                error_message("export_from_file", "target file '" + FILEname + "' is a directory");
            }
        } else {                                            // if target file does not exist
            error_message("export_from_file", "target file '" + FILEname + "' does not exist");
        }
    }
    bool search_string(std::string& targetstring, const std::string& wordToFind) {

        // Create a regular expression pattern to match the whole word
        std::regex wordPattern("\\b" + wordToFind + "\\b");

        // Search for the whole word in the string
        std::smatch match;
        if (std::regex_search(targetstring, match, wordPattern)) {
            std::cout << "search_string: Word '" << wordToFind << "' found at position " << match.position() << std::endl;
            return true;
        } else {
            std::cout << "search_string: Word '" << wordToFind << "' not found" << std::endl;
            return false;
        }
    }
    void D_m(const string& drive, const string& target){    // mount drive on target
        string drive_tmp = drive.c_str();
        string dev_drive = "/dev/" + drive_tmp;
        runProgramWargs(syscommand::sys_mount, {"mount", dev_drive, target.c_str()});
    }
    void fD(const string& mode, const string& drive) {      // format drive in mode selected
        string mode_tmp = mode.c_str();
        string drive_tmp = drive.c_str();
        string dev_drive = "/dev/" + drive_tmp;
        if (mode_tmp == "F32") {runProgramWithInput(syscommand::mkfs_fat, {"mkfs.fat", "-F32", dev_drive}, "Y\n");}
        if (mode_tmp == "ext4") {runProgramWithInput(syscommand::mkfs_ext4, {"mkfs.ext4", "-b 4096", dev_drive}, "Y\n");}
    }
    string executeCommand(const string& command) {          // uses pipes to search for string in output of executed command
        string result; array<char, 128> buffer;

        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {cerr << "Error executing the command." << endl; return "";}

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        pclose(pipe); return result;
    }
    void cmkdir(const string& full_PATH_to_dir){            // make dir if one does not exist at target
        if (!fs::exists(full_PATH_to_dir)) {            // check if dir already exxist
            try {                                               //trys to create directory
                fs::create_directory(full_PATH_to_dir); cout << "Directory created successfully" << endl;
            }
            catch (const fs::filesystem_error& e) {
                cerr << "ERROR creating directory" << e.what() << endl;
            }
        } else {cout << "Directory already exists." << endl;}
    }
    void cddir(const string& full_PATH_to_dir) {            // move active directory to target
        if (fs::exists(full_PATH_to_dir) && fs::is_directory(full_PATH_to_dir)) {
            cout << "cddir: Directory '" << full_PATH_to_dir << "' Exists\n";

            if (chdir(full_PATH_to_dir.c_str()) != 0) {
                cerr << "cddir: Failed to change directory. Press enter to continue" << cin.get() << "\n";
            }
            else if (chdir(full_PATH_to_dir.c_str()) == 0) {
                cout << "cddir: changed Directory to: '" << full_PATH_to_dir.c_str() << "'\n";
            }
        }
        else if (fs::exists(full_PATH_to_dir.c_str())) {
            cerr << "cddir: '" << full_PATH_to_dir.c_str() << "' is a file press enter to continue" << cin.get() << "\n";
        }
        else {
            cerr << "cddir: '" << full_PATH_to_dir.c_str() << "' dosent exist press enter to continue" << cin.get() << "\n";
        }
    }
    void pacman_run(const string& argsString, const string& input) {
        vector<string> args;
        istringstream iss(argsString.c_str());
        string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        runProgramWithInput(syscommand::pacman, args, input.c_str());
    }
}
namespace c_cp_tools {
    bool calcCRC32(const std::string& filename, uint32_t& checksum) {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            cerr << "calcCRC32: Error: Cannot open FILE: '" << filename << "'\n";
            return false;
        }
        if (file.is_open()) {
            cout << "calcCRC32: SUCCESSFULLY Opened '" << filename << "'\n";
            uint32_t crc = 0;
            char buffer[1024];
            while (!file.eof()) {
                file.read(buffer, sizeof(buffer));
                crc = static_cast<uint32_t>(crc32(crc, reinterpret_cast<const Bytef*>(buffer), static_cast<uInt>(file.gcount())));
                //crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer), static_cast<uInt>(file.gcount()));
            }
            file.close();
            checksum = crc;
            cout << "calcCRC32: checksum for FILE: '" << filename << "' is '" << checksum << "'\n";
        }
        return true;
    }
    void Checksum(const std::string& sourcePath, const std::string& destinationPath) {
        uint32_t source_checksum;
        // returns true if calcCRC32 was able to calculate checksum for Source file and false if it couldent
        bool source = calcCRC32(sourcePath, source_checksum);
        if (source) {                                       // if source is true
            std::cout << "Checksum: source FILE: '" << sourcePath << "' has checksum '"<< source_checksum << "'\n";
        } else {                                            // if source is false
            std::cerr << "Failed to calculate checksum for FILE: '" << sourcePath << "'\n";
        }
        uint32_t destination_checksum;
        // returns true if calcCRC32 was able to calculate checksum for Destination and false if it couldent
        bool destination = calcCRC32(destinationPath, destination_checksum);
        if (destination) {                                  // if destination is true
            std::cout << "Checksum: destination FILE: '" << destinationPath << "' has checksum '"<< destination_checksum << "'\n";
        } else {                                            // if destination is false
            std::cerr << "Failed to calculate checksum for FILE: '" << destinationPath << "'\n";
        }
        if (source_checksum == destination_checksum) {      // if checksum of source and destination match print success message
            cout << "Checksum: Match!!\n";
        } else {                                            // if checksum does NOT match print error and wait for user to press enter to continue
            cerr << "Checksum: Dosent Match!! (press enter to continue): " << cin.get() << "\n";
        }
    }
    void copyFile(const std::string& sourcePath, const std::string& destinationPath) {
        // open source file
        std::ifstream source(sourcePath, std::ios::binary);

        // open destination file
        std::ofstream destination(destinationPath, std::ios::binary);

        if (!source) {                                      // if source file coult not be opened tell user and exit function
            std::cerr << "copyFile: Error opening source file: " << sourcePath << "' (press enter to continue): ";
            cin.ignore();

            while (std::cin.get() != '\n') {                // loop until enter is input by user
                std::cerr << "copyFile: (press enter to continue)";
                std::cin.ignore();                          // ignore input becuse it was not enter as enter would have ended the loop
            }
            return;
        }
        if (!destination) {                                 // if destination file could not be opened close function
            std::cerr << "Error opening destination file: " << destinationPath << std::endl;
            return;
        }

        // Use istreambuf_iterator to copy file
        destination << source.rdbuf();

        source.close();                                     // closes source file
        destination.close();                                // closes destination file

        Checksum(sourcePath, destinationPath);
    }
}
void c_cp(const std::string& sourcePath, const std::string& destinationPath, bool force = false) {
    using namespace c_cp_tools;
    if (fs::exists(sourcePath)) {
        cout << "c_cp: Source FILE: '" << sourcePath << "' Does exist\n";
        if (!fs::exists(destinationPath)) {
            cout << "c_cp: Destination FILE: '" << destinationPath << "' Does not exist" << endl;
            copyFile(sourcePath, destinationPath);
        } else {
            if (!force){
                char answer;
                cout << "c_cp: Destination FILE: '" << destinationPath << "' Does exist\nc_cp: do you want to overwrite FILE ?: ";
                cin >> answer;
                if (answer == 'Y' || answer == 'y') {
                    copyFile(sourcePath, destinationPath);
                } else {
                    return;
                }
            } else {
                cout << "c_cp: Destination FILE: '" << destinationPath << "' Does exist\nc_cp: (FORCE MODE) enabled, will OWERWRITE: '" << destinationPath << "' with content of: '" << sourcePath << "'\n";
                copyFile(sourcePath, destinationPath);
            }
        }
    } else {
        cerr << "c_cp: Source FILE: '" << sourcePath << "' Does not exist (press enter to continue): ";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cin.get();
        return;
    }
}
namespace config_tools {
    namespace desktop_enviroment {
        void 
        de_list(std::vector<std::string> items, std::string& pick) 
        {
            initscr();          // Initialize ncurses
            noecho();           // Don't echo keypresses to the screen
            cbreak();           // Disable line buffering
            keypad(stdscr, TRUE); // Enable function keys, arrow keys, etc.
            curs_set(FALSE);    // Hide the cursor

            int selected = 0;
            int currentState = 0;  // This variable stores the current state

            while (1) 
            {
                clear(); // Clear the screen

                if (currentState == 0) // default state
                {                            
                    // Display the first list
                    for (int i = 0; i < items.size(); ++i) 
                    {
                        if (i == selected) 
                        {
                            attron(A_REVERSE); // Highlight the selected item
                        }
                        mvprintw(i + 1, 1, "%s", items[i].c_str());
                        attroff(A_REVERSE); // Turn off highlight
                    }
                } 
                else if (currentState == 1)  // when item 1 is chosen do this
                {                    
                    mvprintw(1, 1, "press enter to confirm KDE as desktop enviroment");
                } 
                else if (currentState == 2) // when item 2 is chosen do this
                {                     
                    mvprintw(1, 1, "press enter to confirm Gnome as desktop enviroment");
                } 
                else if (currentState == 3) // when item 3 is chosen do this
                {                     
                    mvprintw(1, 1, "press enter to confirm xfce as desktop enviroment");
                } 
                else if (currentState == 10) 
                {
                    endwin();
                    return;
                }

                int ch = getch();                                   // Get user input
                switch (ch) 
                {
                    case KEY_UP:// when KEY_UP is pressed
                    {
                        if (selected > 0) 
                        {
                            selected--;// subtract one from selected
                        }
                        break;
                    }                                    
                    case KEY_DOWN:                                  // when KEY_DOWN is pressed
                    {
                        if (selected < items.size() - 1) 
                        {
                            selected++;                             // add 1 to selected
                        }
                        break;
                    }
                    case '\n':                                      // Enter key
                    {
                        if (selected == 0) 
                        {                        // if enter is pressed with item 1 highlighted do this
                            if (currentState == 0) 
                            {                // enters item 1 if user presses enter from main Menu
                                currentState = 1;
                            } 
                            else if (currentState == 1) 
                            {         // if user presses enter from item 1 it will return user to main menu
                                pick = "1";
                                currentState = 10;
                            }
                        }
                        
                        if (selected == 1) 
                        {                        // if enter is pressed with item 2 highlighted do this
                            if (currentState == 0) 
                            {                // enters item 2 if user presses enter from main Menu
                                currentState = 2;
                            } 
                            else if (currentState == 2) 
                            {         // if user presses enter from item 2 it will return user to main menu
                                pick = "2";
                                currentState = 10;
                            }
                        }
                        
                        if (selected == 2) // if enter is pressed with item 3 highlighted do this
                        {                        
                            if (currentState == 0) 
                            {                  
                                currentState = 3;// enters item 3 if user presses enter from main Menu
                            } 
                            else if (currentState == 3) // if user presses enter from item 3 it will return user to main menu
                            {         
                                pick = "3";
                                currentState = 10;
                            }
                        }
                        break;
                    }
                }
            
                // Update the screen
                refresh();
            }

            // Terminate ncurses
            endwin();
        }
        
        void de_pick_func() 
        {
            std::string pick;
            std::vector<std::string> items = {"KDE-Plasma", "Gnome", "xfce"};
            de_list(items, pick);

            if (pick == "1") 
            {
                de_pick = "1";
                return;
            }
            
            if (pick == "2") 
            {
                de_pick = "2";
                return;
            }
            
            if (pick == "3") 
            {
                de_pick = "3";
                return;
            }
        }
    }
    
    namespace sddm_config
    {
        void list(vector<string> items, string& pick)
        {
            initscr();          // Initialize ncurses
            noecho();           // Don't echo keypresses to the screen
            cbreak();           // Disable line buffering
            keypad(stdscr, TRUE); // Enable function keys, arrow keys, etc.
            curs_set(FALSE);    // Hide the cursor

            int selected = 0;
            int currentState = 0;  // This variable stores the current state

            while (1)
            {
                // Clear the screen
                clear();

                // default state
                if (currentState == 0)
                {
                    // Display the first list
                    for (int i = 0; i < items.size(); ++i)
                    {
                        if (i == selected)
                        {
                            // Highlight the selected item
                            attron(A_REVERSE);
                        }
                        
                        mvprintw(i + 1, 1, "%s", items[i].c_str());
                        
                        // Turn off highlight
                        attroff(A_REVERSE);
                    }
                }
                // when item 1 is chosen Display this
                else if (currentState == 1)
                {
                    mvprintw(1, 1, "press enter to confirm X11");
                }
                // when item 2 is chosen do this
                else if (currentState == 2)
                {
                    mvprintw(1, 1, "press enter to confirm Wayland");
                }
                else if (currentState == 10)
                {
                    endwin();
                    return;
                }

                //  Get user input
                int ch = getch();
                switch (ch)
                {
                    // when KEY_UP is pressed
                    case KEY_UP:
                    {
                        if (selected > 0)
                        {
                            // subtract one from selected
                            selected--;
                        }
                        break;
                    }
    
                    // when KEY_DOWN is pressed
                    case KEY_DOWN:
                    {
                        if (selected < items.size() - 1)
                        {
                            // add 1 to selected
                            selected++;
                        }
                        break;
                    }
                    
                    // Enter key
                    case '\n':
                    {
                        // if enter is pressed with item 1 highlighted do this
                        if (selected == 0)
                        {
                            // enters item 1 if user presses enter from main Menu
                            if (currentState == 0)
                            {
                                currentState = 1;
                            }
                            // if user presses enter from item 1 it will return user to main menu
                            else if (currentState == 1)
                            {
                                pick = "1";
                                currentState = 10;
                            }
                        }

                        // if enter is pressed with item 2 highlighted do this
                        if (selected == 1)
                        {
                            // enters item 2 if user presses enter from main Menu
                            if (currentState == 0)
                            {
                                currentState = 2;
                            }
                            // if user presses enter from item 2 it will return user to main menu
                            else if (currentState == 2)
                            {
                                pick = "2";
                                currentState = 10;
                            }
                        }
                    
                        break;
                    }
                }

                // Update the screen
                refresh();
            }

            // Terminate ncurses
            endwin();
        }
        
        void pick_x_or_w()
        {
            string pick;
            vector<string> items =
            {
                "X11", "Wayland"
            };

            list(items, pick);

            if (pick == "1")
            {
                x_or_w = "1";
                return;
            }
        
            if (pick == "2")
            {
                x_or_w = "2";
                return;
            }
        }
    }
}

void config() 
{
    tools::runProgram(syscommand::clear);
    string config_check = "/config.txt";
    
    // pick root password
    while (true) 
    {
        cout << "Select root password: ";
        cin >> rootpass;
        
        // confirm root Password
        while (true) 
        {
            std::cout << "Confirm '" << rootpass << "' as root password (Y/N): "; std::cin >> a;
            
            if (a == 'Y' || a == 'y' || a == 'N' || a == 'n') 
            {
                break;
            }
            else 
            {
                std::cout << "\nERROR : ONLY (Y/N)\n";
            }
        } 
        
        if (a == 'Y' || a == 'y') 
        {
            break;
        }
    }

    // Pick Username
    while (true) 
    {
        cout << "\nPick Username for new user: ";
        cin >> Username_start;

        // confirm Username
        while (true) 
        {
            cout << "Confirm '" << Username_start << "' as Username? (Y/N): "; cin >> a;
            
            if (a == 'Y' || a == 'y' || a == 'N' || a == 'n') 
            {
                break;
            }
            else 
            {
                cout << "ERROR : ONLY (Y/N)\n";
            }
        } 
        
        if (a == 'Y' || a == 'y')
        {
            break;
        }
    }

    // pick password for user
    while (true) 
    {
        cout << "Pick Password for '" <<  Username_start << "': "; cin >> pass_start;
        
        while (true) 
        {
            cout << "Confirm '" << pass_start << "' as password for '" << Username_start << "' (Y/n): "; cin >> a;
            if (a == 'Y' || a == 'y' || a == 'N' || a == 'n') 
            {
                break;
            }
            else 
            {
                cout << "\nERROR : ONLY (Y/N)\n";
            }
        } 

        if (a == 'Y' || a == 'y')
        {
            break;
        }
    }
    
    // gpu pick
    while (true) 
    {
        cout << "\n    1 = Nvidia\n    2 = Amd\n    3 = Intel\n    4 = none\n\n    what gpu do you have: ";
        cin >> gpu_pick;
        
        if (gpu_pick == "1" || gpu_pick == "2" || gpu_pick == "3" || gpu_pick == "4")
        {
            break;
        }
        else 
        {
            cout << "ERROR : ONLY (1/2/3/4)\n";
        } 
    }

    // prompts user to pick desktop-enviroment
    config_tools::desktop_enviroment::de_pick_func();
    
    // prompts user to pick X11 or wayland
    config_tools::sddm_config::pick_x_or_w();
}

void main_drive()
{
    std::string second_disk; std::string second_disk_name; std::string tmp; std::string path;
    char response;
    int lsblk_1 = 1;

    tools::runProgram(syscommand::clear); tools::runProgram(syscommand::lsblk);

    // Prompt the user to pick main drive
    while (true)
    {
        cout << "What is the name of the drive you want to INSTALL ARCH LINUX on: "; cin >> main_drive_name;
        string output = tools::executeCommand(syscommand::lsblk);
        
        if (output.find(main_drive_name) != string::npos)
        {
            cout << "FOUND '" << main_drive_name << "' AS A DRIVE\n";
            break;
        }
        else
        {
            cout << "DID NOT FIND '" << main_drive_name << "' AS A DRIVE\n";
        }
    }

    // Ask if user wants to partition Main Drive
    while (true)
    {
        cout << "Do you want to partition the drive? (Y/N): "; cin >> response;

        if (response == 'Y' || response == 'y')
        {
            tools::runProgramWargs(syscommand::cfdisk, {"cfdisk", "/dev/" + main_drive_name}); break;
        }
        else if (response == 'N' || response == 'n')
        {
            break;
        }
        else
        {
            cout << "ERROR : ONLY (Y/N)" << endl;
        }
    }

    // Main Drive Formating
    while (true)
    {
        tools::fD("F32", main_drive_name + "1");
        tools::fD("ext4", main_drive_name + "2"); tools::D_m(main_drive_name + "2", "/mnt");
        tools::fD("ext4", main_drive_name + "3"); tools::cmkdir("/mnt/home"); tools::D_m(main_drive_name + "3", "/mnt/home");

        string output = tools::executeCommand(syscommand::lsblk);
        
        if (output.find("/mnt") != string::npos)
        {
            cout << "' " << main_drive_name << " ' Was successfully mounted\n";
            break;
        }
        else
        {
            cout << "failed\n";
        }
    }

    // add second or more disks
    while (true)
    {
        if (lsblk_1 == 1)
        {
            tools::runProgram(syscommand::clear);
        }

        tools::runProgram(syscommand::lsblk);
        
        while (true)
        {
            cout << "Do you want to add another drive (Y/N): "; cin >> a;
            
            if (a == 'Y' || a == 'y')
            {
                // pick name for volume on disk
                while (true)
                {
                    cout << "Pick: "; cin >> second_disk_name; string output = tools::executeCommand(syscommand::lsblk);
                    if (output.find(second_disk_name) != string::npos)
                    {
                        cout << "FOUND ' " << second_disk_name << " ' AS A DRIVE\n";
                        break;
                    }
                    else
                    {
                        cout << "Did Not find ' " << second_disk_name << " ' as a Drive\n";
                    }
                }

                break;
            }
            else if (a == 'N' || a == 'n')
            {
                break;
            }
            else
            {
                cout << "ERROR : ONLY (Y/N)\n";
            }
        }
        if (a == 'N' || a == 'n') {
            break;
        }

        // ask if user wants to format drive
        while (true)
        {
            cout << "Do you want to FORMAT the drive (Y/N): "; cin >> a;
        
            // formatting drive
            if (a == 'Y' || a == 'y')
            {
                tools::fD("ext4", second_disk_name);
                break;
            }
            else if (a == 'N' || a == 'n')
            {
                break;
            }
            else
            {
                cout << "\nERROR Formating drive\n";
            }
        }

        // select name for new drive or volume
        while (true)
        {
            while (true)
            {
                cout << "Select Letter For Your Drive: "; cin >> tmp;
                size_t len = tmp.length();
                if (len < 12)
                {
                    break;
                }
                else if (len > 12)
                {
                    cout << "ERROR : Cant be more then 12 char\n";
                }
            }

            string output = tools::executeCommand(syscommand::lsblk);
            if (output.find(tmp) != string::npos)
            {
                cout << "'" << tmp << "' IS ALREADY TAKEN\n";
            }
            else
            {
                break;
            }
        }

        tools::cmkdir("/mnt/home/" + tmp); tools::D_m(second_disk_name, "/mnt/home/" + tmp);
    }
}

void start_install()
{
    tools::create_file("/mnt/username.txt");
    tools::input_to_file(Username_start, "/mnt", "username.txt");
    tools::create_file("/mnt/pass.txt");
    tools::input_to_file(pass_start, "/mnt", "pass.txt");
    tools::create_file("/mnt/gpu.txt");
    tools::input_to_file(gpu_pick, "/mnt", "gpu.txt");
    tools::create_file("/mnt/de.txt");
    tools::input_to_file(de_pick, "/mnt", "de.txt");
    tools::create_file("/mnt/sddm.txt");
    tools::input_to_file(x_or_w, "/mnt", "sddm.txt");

    tools::runProgramWithInput(syscommand::pacstrap, {"pacstrap", "-i", "/mnt", "base", "linux", "linux-firmware", "sudo", "nano"}, "1\nY");

    c_cp("/etc/pacman.conf", "/mnt/etc/pacman.conf", true);
    c_cp("/etc/pacman.d/mirrorlist", "/mnt/etc/pacman.d/mirrorlist", true);
    c_cp("/etc/nanorc", "/mnt/etc/nanorc", true);
    c_cp("/etc/locale.gen", "/mnt/etc/locale.gen", true);
    c_cp("/etc/hosts", "/mnt/etc/hosts", true);
    c_cp("/etc/default/grub", "/mnt/etc/default/grub", true);

    string arch_chroot = "arch-chroot /mnt ";
    string arch_chroot_input = "pacman-key --init\npacman-key --populate\nlocale-gen\necho 'LANG=en_US.UTF-8' > /etc/locale.conf\n ln -sf /usr/share/zoneinfo/Europe/Stockholm /etc/localtime\nhwclock --systohc --utc\necho archPC > /etc/hostname\nyes y | pacman -S networkmanager\nsystemctl enable NetworkManager\nyes y | pacman -S grub efibootmgr\nmount --mkdir /dev/" + main_drive_name + "1 /boot/efi\ngrub-install --target=x86_64-efi --bootloader-id=GRUB --efi-directory=/boot/efi --removable\ngrub-mkconfig -o /boot/grub/grub.cfg";

    tools::bash(arch_chroot + "/bin/bash", arch_chroot_input);

    std::string pass = rootpass;
    std::string password = pass + "\n" + pass;
    tools::bash(arch_chroot + "/bin/passwd", password);     // Applying root password

    std::string gettyautologin = "mkdir -p /etc/systemd/system/getty@tty1.service.d\necho '[Service]' > /etc/systemd/system/getty@tty1.service.d/override.conf\necho 'ExecStart=' >> /etc/systemd/system/getty@tty1.service.d/override.conf\necho 'ExecStart=-/usr/bin/agetty --autologin root --noclear %I $TERM' >> /etc/systemd/system/getty@tty1.service.d/override.conf";
    tools::bash(arch_chroot + "/bin/bash", gettyautologin);

    tools::bash(arch_chroot + "/bin/bash", "cat /etc/passwd | grep root\nsed -i 's+root:x:0:0::/root:/bin/bash+root:x:0:0::/root:/usr/bin/bash -c archtest+g' /etc/passwd\ncat /etc/passwd | grep root");

    system("cp /usr/bin/archtest /mnt/usr/bin/archtest");

    ofstream config_file("/mnt/config.txt");                // create file so that installer knows it has completed first part after reboot
    if (config_file.is_open())
    {
        config_file << "1" << endl;
        config_file.close();
    }
    else
    {
        cout << "Failed to open config file" << endl;
    }

    tools::runProgramWargs(syscommand::umount, {"umount", "-R", "/mnt"});
    system("sleep 1");
    system("shutdown now");
}

namespace start__2
{
    void nvidia() 
    {
        tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "nvidia"}, "Y\n");
    }
    
    void amd() 
    {
        tools::runProgramWargs(syscommand::pacman, {"pacman", "-S", "lib32-mesa", "vulkan-radeon", "lib32-vulkan-radeon", "vulkan-icd-loader", "lib32-vulkan-icd-loader"});
    }
    
    void sddm_setup() 
    {
        tools::create_file("/etc/sddm.conf");
        tools::input_to_file("[General]", "/etc", "sddm.conf");
        tools::input_to_file("Numlock=on", "/etc", "sddm.conf");
        tools::input_to_file("DisplayServer=x11", "/etc", "sddm.conf");
        tools::input_to_file("[Autologin]", "/etc", "sddm.conf");
        tools::input_to_file("User=" + Username, "/etc", "sddm.conf");
    
        if (tools::search_string(x_or_w, "1"))
        {
            tools::input_to_file("Session=plasma.desktop", "/etc", "sddm.conf");
        }
    
        if (tools::search_string(x_or_w, "2"))
        {
            tools::input_to_file("Session=plasmawayland.desktop", "/etc", "sddm.conf");
        }
    }
    
    void installpkg() 
    {
        // installs must have pkg
        tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "axel", "dnsmasq", "kate", "man", "neofetch", "rsync", "reflector", "wget", "tldr", "linux-headers", "git", "ktorrent", "curl", "base-devel", "multilib-devel", "make", "ranger", "bat", "bash-language-server", "dkms", "gnome-common", "libnvme"}, "1\nY");
    }
    
    void yay_installer() 
    {
        tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "go"}, "\r");
        tools::runProgramWargs(syscommand::git, {"git", "clone", "https://aur.archlinux.org/yay-git.git", "/opt/yay-git"});
        tools::runProgramWargs(syscommand::chown, {"chown", "-R", Username + ":wheel", "/opt/yay-git"});
        tools::cddir("/opt/yay-git");
        //bash(sudo + " -u " + Username + " -i", "makepkg -si\nY\nY");
        tools::runProgramWithInput(syscommand::sudo, {"sudo", "-u", Username, "makepkg", "-si"}, "\r");
    }
}

void start_2()
{
    // Local variabels for start_2
    //string Username;
    std::string Usernametmp; std::string pass; std::string gpu; std::string de;

    tools::runProgram(syscommand::clear);

    // define all config files
    ifstream username_add; username_add.open("/username.txt");
    
    // import username
    if (username_add.is_open())
    {
        if (getline(username_add, Username))
        {
            cout << "successfully read username file" << endl;
        }
        else
        {
            cout << "file is empty\n";
        }

        username_add.close();
    }
    else 
        cout << "cant open file\n";

    ifstream pass_add; pass_add.open("/pass.txt");
    
    // import pass from file
    if (pass_add.is_open())
    {
        if (getline(pass_add, pass))
        {
            cout << "successfully read pass file" << endl;
        }
        else
        {
            cout << "file is empty\n";
        }

        pass_add.close();
    }
    else
    {
        cout << "cant open file\n";
    }

    tools::export_from_file("/gpu.txt", gpu);               // export gpu pick from file to var
    tools::export_from_file("/de.txt", de);                 // export de from file to var
    tools::export_from_file("/sddm.txt", x_or_w);           // export sddm pick from file to var

    string password = pass + "\n" + pass;                   // add Username with password
    string adduser = "useradd -m -g users -G wheel -s /bin/bash " + Username;
    system(adduser.c_str());
    string passwdUser = "passwd " + Username;
    tools::bash(passwdUser, password);                             // add Username with password, end

    system("sed -i 's/# %wheel ALL=(ALL:ALL) NOPASSWD: ALL/ %wheel ALL=(ALL:ALL) NOPASSWD: ALL/g' /etc/sudoers");   // change sudoers so user can run sudo commands
    // change sudoers so user can run sudo commands, End
    system("sed -i 's/# %sudo ALL=(ALL:ALL) ALL/ %sudo ALL=(ALL:ALL) ALL/g' /etc/sudoers");


    tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "pulseaudio", "pulseaudio-alsa", "xorg", "xorg-xinit", "xorg-server"}, "all\n1\nY");
    tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "openssl-1.1", "lib32-openssl-1.1", "crypto++", "zlib"}, "Y");

    // nvidia installer
    if (tools::search_string(gpu, "1"))
    {
        start__2::nvidia();
    }
    
    // amd installer
    if (tools::search_string(gpu, "2"))
    {
        start__2::amd();
    }

    
    // installs kdem if user picked 1 at config
    if (tools::search_string(de, "1"))
    {
        ofstream kdem; kdem.open("/.xinitrc");
        if (kdem.is_open())
        {
            kdem << "exec startkde" << endl;
            kdem.close();
        }
        else
        {
            cout << "failed to open file" << endl;
        }

        // installs kde-plasma
        tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "plasma-desktop", "plasma-pa", "dolphin", "dolphin-plugins", "konsole", "yakuake", "sddm", "kscreen", "plasma-wayland-session", "plasma-wayland-protocols"}, "1\n1\n1\n1\nY");
        // installs discover
        tools::runProgramWithInput(syscommand::pacman, {"pacman", "-S", "discover", "archlinux-appstream-data", "discount", "purpose", "qt5-webview", "flatpak", "fwupd", "packagekit-qt5", "extra-cmake-modules"}, "4\nY");
        tools::runProgramWargs(syscommand::systemctl, {"systemctl", "enable", "sddm.service"});
        tools::runProgramWargs(syscommand::systemctl, {"systemctl", "enable", "NetworkManager.service"});
    }

    start__2::installpkg();
    start__2::yay_installer();
    start__2::sddm_setup();

    system("sed -i 's+root:x:0:0::/root:/usr/bin/bash -c archtest+root:x:0:0::/root:/bin/bash+g' /etc/passwd");
    tools::runProgramWargs(syscommand::sleep, {"sleep", "1"});
    system("reboot");
}

int main()
{
    string config_check = "/config.txt";

    if (fs::exists(config_check))
    {
        start_2();
    }
    else if (!fs::exists(config_check))
    {
        config();
        main_drive();
        start_install();
    }

    return 0;
}
