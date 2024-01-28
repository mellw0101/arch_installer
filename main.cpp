#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

template <typename T>
using vec = std::vector<T>;
using string = std::string; 

namespace tools {
    int pacman(vec<string> packages) {
        string packages_string = "sudo pacman -S --noconfirm ";
        for (const string & part : packages) {
            packages_string += part + " ";
        }
        packages_string.erase(packages_string.length() - 1);
        int status = std::system(packages_string.c_str());
        if (status != 0) {
            std::cerr << "system call failed" << std::endl;
        }
        return status;
    }
    int pacstrap(vec<string> packages) {
        string packages_string = "pacstrap -i /mnt ";
        for (const string & part : packages) {
            packages_string += part + " ";
        }
        packages_string.erase(packages_string.length() - 1);
        int status = std::system(packages_string.c_str());
        if (status != 0) {
            std::cerr << "system call failed" << std::endl;
        }
        return status;
    }
    string get_responce() {
        string tmp;
        std::cin >> tmp;
        return tmp;
    }
    bool connectToWiFi(const string & ssid, const string & password) { // Function to connect to a WiFi network using iwctl with a password
        std::string command = "iwctl station wlan0 connect \"" + ssid + "\"";
        
        if (!password.empty()) {
            command += " passphrase \"" + password + "\"";
        }
        
        // Run the iwctl command to connect
        int result = std::system(command.c_str());

        if (result == 0) {
            std::cout << "Connected to WiFi network: " << ssid << std::endl;
            return true;
        } else {
            std::cerr << "Failed to connect to WiFi network: " << ssid << std::endl;
            return false;
        }
    }
}
namespace drive {
    void selector() {
        std::system("lsblk");
        std::cout << "\nWhere do you want to install arch?: ";
        std::cout << tools::get_responce() << "\n";
    }
}
int main() {
    tools::connectToWiFi("comhem_3E2192", "26849bzs");
}