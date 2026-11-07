#include "../../support/HostPaths.h"

#include <cstdlib>
#include <iostream>

int main()
{
    using HostPaths::directoryOf;
    using HostPaths::splitList;
    if (directoryOf("C:\\Program Files\\AIDA\\bin\\ada.exe") != "C:\\Program Files\\AIDA\\bin"
        || directoryOf("/opt/AIDA tools/bin/ada") != "/opt/AIDA tools/bin"
        || directoryOf("ada").size() != 0
        || splitList("C:\\AIDA tools;D:/compiler/bin;;", ';')
            != std::vector<std::string> {"C:\\AIDA tools", "D:/compiler/bin", "", ""}
        || splitList(":/tmp/one directory:/usr/bin:", ':')
            != std::vector<std::string> {"", "/tmp/one directory", "/usr/bin", ""}) {
        std::cerr << "host path parsing failed\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
