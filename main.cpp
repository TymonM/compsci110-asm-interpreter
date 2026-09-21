#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "assembler.hpp"
#include "vm.hpp"

std::vector<short> inputs = {5, 2};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ./this <asm-source>\n";
        exit(1);
    }
    std::ifstream in(argv[1]);
    std::stringstream program;
    program << in.rdbuf();

    Assembler assembler{};
    VM vm;
    Recorder recorder = assembler.assemble(program.str(), vm);
    vm.dump_memory(std::cout, assembler.assembled_size);  // print the assembled machine code
    std::cout << '\n';

    vm.R = 0;
    vm.run(inputs, &recorder);

    for (int16_t x : vm.out) {
        std::cout << x << ' ';
    }
    std::cout << std::endl;

    recorder.pretty_print(std::cout);

    return 0;
}