#include "../ada/WindowsStack.h"

#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(condition) do { if (!(condition)) { \
    std::cerr << "Windows stack probe check failed at line " << __LINE__ << '\n'; std::abort(); \
} } while (0)

int main(int argc, char** argv)
{
    if (argc == 2) {
        return prepareWindowsAssembly(argv[1]) ? 0 : 1;
    }
    const std::string small = "func:\n\tsubq $4095, %rsp\n\tsubq $-8192, %rsp\n\tsubq $8192, %rax\n\tsubq %rax, %rsp\n";
    CHECK(probeWindowsStack(small) == small);
    const std::string page = "\tsubq $4096, %rsp\n\ttestb $0, (%rsp)\n";
    CHECK(probeWindowsStack("\tsubq $4096, %rsp\n") == page);
    CHECK(probeWindowsStack("\tsubq $4097, %rsp\n") == page + "\tsubq $1, %rsp\n\ttestb $0, (%rsp)\n");
    CHECK(probeWindowsStack("  subq\t$8192 ,  %rsp\r\n") == page + page);
    CHECK(probeWindowsStack("\tsubq $8192, %rsp # frame\n") == page + page);
    CHECK(probeWindowsStack("\tsubq $8192, %rspurious\n") == "\tsubq $8192, %rspurious\n");
    CHECK(probeWindowsStack("\tsubq $18446744073709551616, %rsp\n") == "\tsubq $18446744073709551616, %rsp\n");
    CHECK(probeWindowsStack("\tsubq $18446744073709551615, %rsp\n") == "\tsubq $18446744073709551615, %rsp\n");
    const std::string prefix = "f:\n\tpushq %rbp\n\tmovq %rsp, %rbp\n";
    const std::string suffix = "\tpushq %rbx\n\tmovq %rcx, %r12\n\tleave\n\tret\n";
    std::string expected = prefix;
    for (int i = 0; i < 6; ++i) {
        expected += page;
    }
    expected += "\tsubq $3608, %rsp\n\ttestb $0, (%rsp)\n";
    expected += suffix;
    CHECK(probeWindowsStack(prefix + "\tsubq $28184, %rsp\n" + suffix) == expected);
    CHECK(probeWindowsStack("\tsubq $4096, %rsp") == page.substr(0, page.size() - 1));
    return 0;
}
