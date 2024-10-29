#include <iostream>
#include "NetworkInterface.hpp"

int main() {
    auto const interfaces = NetworkInterface::getAllInterfaces();

    for (auto const& intf : interfaces) {
        std::cout << intf.getName() << std::endl;

        auto addresses = intf.getAddresses();
        for (auto const& addr : addresses) {
            std::cout << "    " << addr.toString() << std::endl;
        }

        std::cout << std::endl << std::endl;
    }

    return 0;
}