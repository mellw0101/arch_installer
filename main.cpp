#include <iostream>
#include <cstdlib>
#include <vector>

template <typename T>
using vec = std::vector<T>;
using string = std::string; 

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
int main() {
    pacstrap({"base", "linux", "linux-firmware", "sudo", "nano"});
}