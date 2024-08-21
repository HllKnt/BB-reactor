#include <iostream>
#include <string>

#include "sockpp/unix_connector.h"

using namespace sockpp;

int main(int argc, char* argv[]) {
    sockpp::initialize();
    unix_connector conn;
    conn.connect(unix_address("\0sock"));
    string s, sret;
    while (getline(std::cin, s) && !s.empty()) {
        const size_t N = s.length();
        if (auto res = conn.write(s); !res || res != N) {
            break;
        }
        sret.resize(N);
        if (auto res = conn.read_n(&sret[0], N); !res || res != N) {
            break;
        }
        std::cout << sret << std::endl;
    }
    return 0;
}
