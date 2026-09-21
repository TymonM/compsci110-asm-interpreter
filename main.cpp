#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "assembler.hpp"
#include "vm.hpp"

std::vector<short> inputs = {5, 2};
std::string source = "source.txt";

int main() {
    std::ifstream in(source);
    std::stringstream program;
    program << in.rdbuf();

    Assembler assembler;
    VM vm;
    Recorder recorder = assembler.assemble(program.str(), vm);
    vm.R = 0;
    vm.run(inputs, &recorder);

    for (int16_t x : vm.out) {
        std::cout << x << ' ';
    }
    std::cout << std::endl;

    recorder.pretty_print(std::cout);

    return 0;
}