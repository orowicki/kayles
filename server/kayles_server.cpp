#include <iostream>
#include <string>
#include <unistd.h>

#include "args.h"
#include "config.h"

using std::cerr;
using std::cout;
using std::exception;
using std::string;

int main(int argc, char *argv[])
{
    Config cfg;

    try {
        Config cfg = configure_from_args(argc, argv);
    } catch (exception &e) {
        cerr << e.what() << '\n';
        return 1;
    }

    cout << "that's all\n";

}
