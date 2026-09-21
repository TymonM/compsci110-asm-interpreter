#ifndef FE88CD60_C3AD_4418_A180_8C9EA4AEAD31
#define FE88CD60_C3AD_4418_A180_8C9EA4AEAD31

#include <cstdint>
#include <vector>

#include "vm.hpp"

enum class TokenType {
    Symbol,
    Colon,
    Dot,
    Numeric,
    Comment,
    Newline
};
struct Token {
    virtual bool push(char c) = 0;
    virtual TokenType get_type() = 0;
    virtual void* get_data() {
        return nullptr;
    }
    virtual ~Token() = default;
};
struct SymbolToken : Token {
    std::string name;

    SymbolToken(const std::string& name) : name(name) {}

    bool push(char c) {
        if (!isalnum(c)) return false;
        name += c;
        return true;
    }

    TokenType get_type() {
        return TokenType::Symbol;
    }

    void* get_data() {
        return &name;
    }
};
struct ColonToken : Token {
    bool push(char c) {
        return false;
    }

    TokenType get_type() {
        return TokenType::Colon;
    }
};
struct DotToken : Token {
    bool push(char c) {
        return false;
    }

    TokenType get_type() {
        return TokenType::Dot;
    }
};
struct NumericToken : Token {
    int16_t value;

    explicit NumericToken(int16_t value) : value(value) {}

    bool push(char c) {
        if (!isdigit(c)) return false;
        value *= 10;
        value += (c - '0');
        return true;
    }

    TokenType get_type() {
        return TokenType::Numeric;
    }

    void* get_data() {
        return &value;
    }
};
struct CommentToken : Token {
    bool push(char c) {
        return c != '\n';
    }

    TokenType get_type() {
        return TokenType::Comment;
    }
};
struct NewlineToken : Token {
    bool push(char c) {
        return isspace(c);
    }

    TokenType get_type() {
        return TokenType::Newline;
    }
};

struct TokenStream {
    std::vector<Token*> tokens;
    size_t p;

    TokenStream() : tokens({}), p(0) {}

    void reset();
    Token* peek(size_t offset = 0);
    Token* consume();
};

void TokenStream::reset() {
    p = 0;
}

Token* TokenStream::peek(size_t offset) {
    if (p + offset >= tokens.size()) return nullptr;
    return tokens[p + offset];
}

Token* TokenStream::consume() {
    if (p >= tokens.size()) return nullptr;
    return tokens[p++];
}

struct Tokenizer {
    Token* newToken(char c);
    TokenStream tokenize(const std::string& program);
};

Token* Tokenizer::newToken(char c) {
    if (isalpha(c)) {
        return new SymbolToken(std::string{c});
    }
    if (isdigit(c)) {
        return new NumericToken(c - '0');
    }
    if (c == ':') {
        return new ColonToken{};
    }
    if (c == '.') {
        return new DotToken{};
    }
    if (c == '-') {
        return new CommentToken{};
    }
    if (c == '\n') {
        return new NewlineToken{};
    }
    return nullptr;
}

TokenStream Tokenizer::tokenize(const std::string& program) {
    TokenStream res_stream;
    Token* buffer = nullptr;

    for (char c : program) {
        if (!buffer) {
            buffer = newToken(c);
        } else {
            bool can_push = buffer->push(c);
            if (!can_push) {
                res_stream.tokens.push_back(buffer);
                buffer = newToken(c);
            }
        }
    }
    if (buffer) res_stream.tokens.push_back(buffer);

    return std::move(res_stream);
}

struct Assembler {
    TokenStream token_stream;
    std::unordered_map<std::string, int16_t> symbol_mappings;
    size_t assembled_size;

    Assembler() : assembled_size(0) {}

    void build_symbol_mappings(Recorder& recorder);
    int16_t assemble_instruction(std::string opcode, int16_t loc, int16_t dest);
    Recorder assemble(const std::string& program, VM& vm);
};

struct AssemblerError : std::exception {
    std::string message;
    std::string formatted_message;
    int16_t vm_loc;
    AssemblerError(std::string message, int16_t vm_loc) : message(std::move(message)), vm_loc(vm_loc) {
        std::stringstream res;
        res << '[' << vm_loc << "] " << this->message;
        formatted_message = res.str();
    }
    const char* what() const noexcept override {
        return formatted_message.c_str();
    }
};

void Assembler::build_symbol_mappings(Recorder& recorder) {
    size_t loc = 0;
    int cur_line_cnt = 0;
    while (token_stream.peek()) {
        Token* cur_token = token_stream.consume();
        ++cur_line_cnt;
        if (cur_token->get_type() == TokenType::Newline) {
            loc += (cur_line_cnt > 1);
            cur_line_cnt = 0;
            continue;
        }
        if (cur_token->get_type() == TokenType::Comment) {
            --cur_line_cnt;
            continue;
        }
        if (cur_token->get_type() == TokenType::Dot) {
            cur_line_cnt -= 2;
        }
        if (cur_token->get_type() != TokenType::Symbol) {
            continue;
        }
        Token* ahead = token_stream.peek();
        if (ahead && ahead->get_type() == TokenType::Colon) {
            std::string symbol_name = *(std::string*)cur_token->get_data();
            symbol_mappings[symbol_name] = loc;
            // only push .DATAs to the recorder
            if (token_stream.peek(1) && token_stream.peek(1)->get_type() == TokenType::Dot) recorder.symbol_mappings[symbol_name] = loc;
        }
    }
}

int16_t Assembler::assemble_instruction(std::string opcode, int16_t loc, int16_t dest) {
    if (opcode == "LOAD") {
        return 0b0000'0000'0000'0000 | dest;
    }
    if (opcode == "STORE") {
        return 0b0001'0000'0000'0000 | dest;
    }
    if (opcode == "CLEAR") {
        return 0b0010'0000'0000'0000 | dest;
    }
    if (opcode == "ADD") {
        return 0b0011'0000'0000'0000 | dest;
    }
    if (opcode == "INCREMENT") {
        return 0b0100'0000'0000'0000 | dest;
    }
    if (opcode == "SUBTRACT") {
        return 0b0101'0000'0000'0000 | dest;
    }
    if (opcode == "DECREMENT") {
        return 0b0110'0000'0000'0000 | dest;
    }
    if (opcode == "COMPARE") {
        return 0b0111'0000'0000'0000 | dest;
    }
    if (opcode == "JUMP") {
        return 0b1000'0000'0000'0000 | dest;
    }
    if (opcode == "JUMPGT") {
        return 0b1001'0000'0000'0000 | dest;
    }
    if (opcode == "JUMPEQ") {
        return 0b1010'0000'0000'0000 | dest;
    }
    if (opcode == "JUMPLT") {
        return 0b1011'0000'0000'0000 | dest;
    }
    if (opcode == "JUMPNEQ") {
        return 0b1100'0000'0000'0000 | dest;
    }
    if (opcode == "IN") {
        return 0b1101'0000'0000'0000 | dest;
    }
    if (opcode == "OUT") {
        return 0b1110'0000'0000'0000 | dest;
    }

    std::stringstream message;
    message << "Unrecognised opcode: " << opcode;
    throw AssemblerError(message.str(), loc);
}

Recorder Assembler::assemble(const std::string& program, VM& vm) {
    Tokenizer tokenizer;
    token_stream = tokenizer.tokenize(program);

    Recorder recorder{};
    build_symbol_mappings(recorder);
    token_stream.reset();

    size_t loc = 0;
    while (token_stream.peek()) {
        bool valid_line = false;
        while (token_stream.peek()) {
            Token* cur_token = token_stream.consume();
            if (cur_token->get_type() == TokenType::Newline) {
                break;
            }
            if (cur_token->get_type() == TokenType::Comment) {
                continue;
            }
            if (cur_token->get_type() == TokenType::Dot) {
                // pseudo op
                cur_token = token_stream.consume();
                if (!cur_token) throw AssemblerError("Expected symbol, found EOF after dot token", loc);
                if (cur_token->get_type() != TokenType::Symbol) throw AssemblerError("Expected symbol after dot token", loc);
                std::string op = *(std::string*)cur_token->get_data();
                if (op == "BEGIN") {
                    vm.pc = loc;
                }
                if (op == "DATA") {
                    cur_token = token_stream.consume();
                    if (!cur_token) throw AssemblerError("Expected tokens, found EOF after .DATA", loc);
                    if (cur_token->get_type() != TokenType::Numeric) throw AssemblerError("Expected numeric after .DATA", loc);
                    int16_t val = *(int16_t*)cur_token->get_data();
                    vm.mem[loc] = val;
                    valid_line = true;
                }
            } else if (cur_token->get_type() == TokenType::Colon)
                continue;
            else if (cur_token->get_type() == TokenType::Numeric) {
                throw AssemblerError("Expected symbol or pseudo op, found numeric", loc);
            } else if (cur_token->get_type() == TokenType::Symbol) {
                Token* ahead = token_stream.peek();
                if (ahead && ahead->get_type() == TokenType::Colon) continue;
                std::string name = *(std::string*)cur_token->get_data();
                if (name == "HALT") {
                    vm.mem[loc] = 0b1111'0000'0000'0000;
                    valid_line = true;
                    continue;
                }
                cur_token = token_stream.consume();
                if (!cur_token) throw AssemblerError("Expected token, found EOF after opcode symbol", loc);
                int16_t dest_loc;
                if (cur_token->get_type() == TokenType::Symbol) {
                    std::string dest = *(std::string*)cur_token->get_data();
                    if (!symbol_mappings.count(dest)) {
                        std::stringstream message;
                        message << "Unknown symbol: " << dest;
                        throw AssemblerError(message.str(), loc);
                    }
                    dest_loc = symbol_mappings[dest];
                } else if (cur_token->get_type() == TokenType::Numeric) {
                    dest_loc = *(int16_t*)cur_token->get_data();
                } else {
                    throw AssemblerError("Expected numeric or symbol after opcode symbol", loc);
                }
                vm.mem[loc] = assemble_instruction(name, loc, dest_loc);
                valid_line = true;
            }
        }
        if (valid_line) ++loc;
    }

    assembled_size = loc;
    return std::move(recorder);
}

#endif /* FE88CD60_C3AD_4418_A180_8C9EA4AEAD31 */
