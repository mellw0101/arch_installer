#include <iostream>
#include <cstdlib>
#include <vector>

template <typename T>
using vec = std::vector<T>;
using string = std::string;

int pacman(vec<string> & packages) {
    string packages_string = "sudo pacman -S --noconfirm ";
    for (const string & part : packages) {
        packages_string += part + " ";
    }
    packages_string.erase(packages_string.length() - 1);
    std::system(packages_string.c_str());
}

int main() {

}