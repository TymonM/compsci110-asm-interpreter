#ifndef E79F6D1E_B8D1_4C98_A8C3_54984007213C
#define E79F6D1E_B8D1_4C98_A8C3_54984007213C

#include <array>
#include <bitset>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

struct VM;
struct ExecutionRecord {
    std::unordered_map<std::string, std::vector<int16_t>> hist;
    size_t len;

    ExecutionRecord() : hist({}), len(0) {}
};

struct Recorder {
    ExecutionRecord record;
    std::unordered_map<std::string, int16_t> symbol_mappings;

    Recorder() : record({}), symbol_mappings({}) {}

    void snap(const VM& vm);
    void pretty_print(std::ostream& f);
};

struct VM {
    std::array<int16_t, 4096> mem;
    int16_t R;
    std::vector<int16_t> out;

    int16_t pc;
    bool lt : 1;
    bool eq : 1;
    bool gt : 1;

    void run(const std::vector<int16_t>& input, Recorder* recorder = nullptr);
    void dump_memory(std::ostream& f, size_t words) const;
};

enum class Operation {
    Load,
    Store,
    Clear,
    Add,
    Increment,
    Subtract,
    Decrement,
    Compare,
    Jump,
    JumpGt,
    JumpEq,
    JumpLt,
    JumpNeq,
    In,
    Out,
    Halt
};

std::pair<Operation, int16_t> decode(int16_t word) {
    return {(Operation)((uint16_t)word >> 12), word & ((1 << 12) - 1)};
}

void VM::run(const std::vector<int16_t>& input, Recorder* recorder) {
    size_t inp = 0;

    while (1) {
        int16_t instruction = mem[pc++];
        auto [opcode, dest] = decode(instruction);
        if (opcode == Operation::Load) R = mem[dest];
        if (opcode == Operation::Store) mem[dest] = R;
        if (opcode == Operation::Clear) mem[dest] = 0;
        if (opcode == Operation::Add) R += mem[dest];
        if (opcode == Operation::Increment) ++mem[dest];
        if (opcode == Operation::Subtract) R -= mem[dest];
        if (opcode == Operation::Decrement) --mem[dest];
        if (opcode == Operation::Compare) lt = mem[dest]<R, gt = mem[dest]> R, eq = 1 - lt - gt;
        if (opcode == Operation::Jump) pc = dest;
        if (opcode == Operation::JumpGt && gt) pc = dest;
        if (opcode == Operation::JumpEq && eq) pc = dest;
        if (opcode == Operation::JumpLt && lt) pc = dest;
        if (opcode == Operation::JumpNeq && !eq) pc = dest;
        if (opcode == Operation::In) mem[dest] = input.at(inp++);
        if (opcode == Operation::Out) out.push_back(mem[dest]);
        if (opcode == Operation::Halt) break;

        if (recorder) {
            recorder->snap(*this);
        }
    }
}

void VM::dump_memory(std::ostream& f, size_t words) const {
    for (size_t i = 0; i < words; ++i) {
        int16_t cur_word = mem[i];
        f << std::format("{:#05x} |", i);
        for (int block = 12; block >= 0; block -= 4) {
            f << ' ' << std::bitset<4>{(cur_word >> block) & 0xFull};
        }
        f << '\n';
    }
}

void Recorder::snap(const VM& vm) {
    record.hist["<R>"].push_back(vm.R);
    record.hist["<LT>"].push_back(vm.lt);
    record.hist["<EQ>"].push_back(vm.eq);
    record.hist["<GT>"].push_back(vm.gt);
    for (auto [s, loc] : symbol_mappings) {
        record.hist[s].push_back(vm.mem[loc]);
    }
    ++record.len;
}

void Recorder::pretty_print(std::ostream& f) {
    size_t longestw = 0;
    for (auto [s, hist] : record.hist) longestw = std::max(longestw, s.size());

    std::unordered_map<std::string, std::stringstream> individual_hists;
    for (auto [s, hist] : record.hist) {
        individual_hists[s] << std::setw(longestw) << s << " |";
    }
    std::unordered_map<std::string, std::optional<int16_t>> last_value;
    for (size_t t = 0; t < record.len; ++t) {
        for (auto& [s, hist] : record.hist) {
            individual_hists[s] << ' ';
            std::optional<int16_t> cur_value = {hist[t]};
            if (cur_value == last_value[s]) {
                individual_hists[s] << "    ";
            } else {
                individual_hists[s] << std::setw(4) << cur_value.value();
            }
            last_value[s] = cur_value;
        }
    }

    for (auto& [s, ss] : individual_hists) {
        f << ss.str() << '\n';
    }
}

#endif /* E79F6D1E_B8D1_4C98_A8C3_54984007213C */
