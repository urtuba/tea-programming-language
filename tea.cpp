/*
 * AUTHOR: Samed Kahyaoglu
 * GITHUB: urtuba
 * Restored: 2026
 *
 * Tea is a tiny language with functions and five variables (a to e) per call.
 * This file is its interpreter: it checks a program, then runs it from main.
 * The program comes from the file given as argument, or from standard input.
 *
 * The trace ("Executing <statement> at line N") goes to stderr. The result
 * goes to stdout. An error goes to stderr as "error: line N: <problem>" and
 * ends the program with exit status 1.
 */

#include <array>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

const int VARIABLE_COUNT = 5;

// Tea has no conditionals, so a recursive function never stops by itself.
// This limit ends such a program with an error.
const size_t MAX_CALL_DEPTH = 10000;

// A problem in the program. Line 0 means it does not belong to one line.
struct TeaError {
    int line;
    std::string message;
};

// The variables of one call, and where to go when it returns.
struct Frame {
    std::array<int, VARIABLE_COUNT> variables{};
    int callAddress = 0;  // address of the call statement that started it
};

enum class Op { Inc, Dec, Mul, Div, Function, Call, Return };

struct Statement {
    Op op;
    std::string keyword;  // as written in the program, shown in the trace
    int line;             // line in the program file, counted from 1
    std::string name;     // function and call: the function name
    int variable = 0;     // inc, dec, mul, div: the target variable;
                          // call: the passed variable; return: the result
    bool operandIsVariable = false;  // inc, dec, mul, div: kind of operand
    int operand = 0;      // variable index or number
    int target = 0;       // call: address of the called "function" statement
};

// Maps the names a to e to 0 to 4. Any other name gives -1.
int variableIndex(const std::string& name)
{
    if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'e') {
        return name[0] - 'a';
    }
    return -1;
}

int parseVariable(const std::string& word, int line)
{
    int index = variableIndex(word);
    if (index < 0) {
        throw TeaError{line, "'" + word + "' is not a variable (use a, b, c, d or e)"};
    }
    return index;
}

// Parses a whole number: digits with an optional sign, in the range of int.
int parseNumber(const std::string& word, int line)
{
    size_t digits = (!word.empty() && (word[0] == '-' || word[0] == '+')) ? 1 : 0;
    bool valid = word.size() > digits;
    for (size_t i = digits; i < word.size(); i++) {
        valid = valid && word[i] >= '0' && word[i] <= '9';
    }
    if (!valid) {
        throw TeaError{line, "'" + word + "' is not a variable or a whole number"};
    }

    errno = 0;
    long long value = std::strtoll(word.c_str(), nullptr, 10);
    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        throw TeaError{line, "number '" + word + "' is out of range"};
    }
    return static_cast<int>(value);
}

std::string countOf(size_t count)
{
    return std::to_string(count) + (count == 1 ? " argument" : " arguments");
}

class Interpreter {
public:
    // Reads a program and checks it. Throws TeaError if it is not valid.
    void load(std::istream& in);

    // Runs the program from the statement after "function main".
    void run();

private:
    // Fills in one statement from the words of its line.
    Statement parse(const std::vector<std::string>& words, int line) const;

    // Checks that the function that started at the given address ends with return.
    void checkFunctionEnd(int header) const;

    // Executes the statement at the given address and returns the address
    // of the next one.
    int step(int address);

    void callFunction(const Statement& statement, int address);
    int returnFromFunction(const Statement& statement);
    void finishMain(const Statement& statement);
    void calculate(const Statement& statement);

    std::vector<Statement> statements_;
    int mainAddress_ = 0;

    // One frame per active call. The first one belongs to main.
    std::vector<Frame> frames_;
    bool done_ = false;
};

Statement Interpreter::parse(const std::vector<std::string>& words, int line) const
{
    static const std::map<std::string, Op> keywords = {
        {"inc", Op::Inc},     {"dec", Op::Dec},   {"mul", Op::Mul},
        {"div", Op::Div},     {"call", Op::Call}, {"return", Op::Return},
        {"function", Op::Function},
    };

    auto keyword = keywords.find(words[0]);
    if (keyword == keywords.end()) {
        throw TeaError{line, "unknown statement '" + words[0] + "'"};
    }

    Statement statement;
    statement.op = keyword->second;
    statement.keyword = words[0];
    statement.line = line;

    // function and return take one argument, the others take two.
    bool oneArgument = statement.op == Op::Function || statement.op == Op::Return;
    size_t expected = oneArgument ? 1 : 2;
    if (words.size() - 1 != expected) {
        throw TeaError{line, "'" + words[0] + "' takes " + countOf(expected) +
                                 ", got " + std::to_string(words.size() - 1)};
    }

    switch (statement.op) {
    case Op::Function:
        statement.name = words[1];
        break;
    case Op::Call:
        statement.name = words[1];
        statement.variable = parseVariable(words[2], line);
        break;
    case Op::Return:
        statement.variable = parseVariable(words[1], line);
        break;
    case Op::Inc:
    case Op::Dec:
    case Op::Mul:
    case Op::Div:
        statement.variable = parseVariable(words[1], line);
        statement.operandIsVariable = variableIndex(words[2]) >= 0;
        statement.operand = statement.operandIsVariable
                                ? variableIndex(words[2])
                                : parseNumber(words[2], line);
        break;
    }
    return statement;
}

void Interpreter::checkFunctionEnd(int header) const
{
    int last = static_cast<int>(statements_.size()) - 1;
    if (last == header || statements_[last].op != Op::Return) {
        throw TeaError{statements_[header].line,
                       "function '" + statements_[header].name +
                           "' does not end with return"};
    }
}

void Interpreter::load(std::istream& in)
{
    std::map<std::string, int> functions;  // name to address of its statement
    int header = -1;                       // address of the current function
    std::string text;
    int line = 0;

    while (std::getline(in, text)) {
        line++;
        std::istringstream lineWords(text);
        std::vector<std::string> words;
        for (std::string word; lineWords >> word;) {
            words.push_back(word);
        }
        if (words.empty()) {
            continue;
        }

        Statement statement = parse(words, line);
        if (statement.op != Op::Function && header < 0) {
            throw TeaError{line, "statement outside a function"};
        }

        if (statement.op == Op::Function) {
            auto known = functions.find(statement.name);
            if (known != functions.end()) {
                throw TeaError{line, "function '" + statement.name +
                                         "' is already defined at line " +
                                         std::to_string(statements_[known->second].line)};
            }
            if (header >= 0) {
                checkFunctionEnd(header);
            }
            header = static_cast<int>(statements_.size());
            functions[statement.name] = header;
        }
        statements_.push_back(statement);
    }

    if (header >= 0) {
        checkFunctionEnd(header);
    }

    // Every call needs a function to go to.
    for (size_t i = 0; i < statements_.size(); i++) {
        if (statements_[i].op == Op::Call) {
            auto function = functions.find(statements_[i].name);
            if (function == functions.end()) {
                throw TeaError{statements_[i].line,
                               "unknown function '" + statements_[i].name + "'"};
            }
            statements_[i].target = function->second;
        }
    }

    auto mainFunction = functions.find("main");
    if (mainFunction == functions.end()) {
        throw TeaError{0, "no function main"};
    }
    mainAddress_ = mainFunction->second;
}

void Interpreter::run()
{
    frames_.assign(1, Frame());
    int address = mainAddress_ + 1;
    while (!done_) {
        address = step(address);
    }
}

int Interpreter::step(int address)
{
    const Statement& statement = statements_[address];
    std::cerr << "Executing " << statement.keyword << " at line " << statement.line << '\n';

    switch (statement.op) {
    case Op::Call:
        callFunction(statement, address);
        return statement.target + 1;
    case Op::Return:
        if (frames_.size() > 1) {
            return returnFromFunction(statement);
        }
        finishMain(statement);
        return 0;
    case Op::Inc:
    case Op::Dec:
    case Op::Mul:
    case Op::Div:
        calculate(statement);
        break;
    case Op::Function:  // Not reached: every function ends with return.
        break;
    }
    return address + 1;
}

// Starts a new call. All its variables are 0 except the one passed in.
void Interpreter::callFunction(const Statement& statement, int address)
{
    if (frames_.size() > MAX_CALL_DEPTH) {
        throw TeaError{statement.line, "call depth over 10000"};
    }
    Frame frame;
    frame.variables[statement.variable] = frames_.back().variables[statement.variable];
    frame.callAddress = address;
    frames_.push_back(frame);
}

// Copies the returned variable into the same variable of the caller and
// returns the address after the call statement.
int Interpreter::returnFromFunction(const Statement& statement)
{
    Frame callee = frames_.back();
    frames_.pop_back();
    frames_.back().variables[statement.variable] = callee.variables[statement.variable];
    return callee.callAddress + 1;
}

// "return" in main prints the variable and ends the program.
void Interpreter::finishMain(const Statement& statement)
{
    std::cout << frames_.front().variables[statement.variable] << '\n';
    done_ = true;
}

// Runs inc, dec, mul or div on the variables of the current call.
void Interpreter::calculate(const Statement& statement)
{
    std::array<int, VARIABLE_COUNT>& variables = frames_.back().variables;
    int left = variables[statement.variable];
    int right = statement.operandIsVariable ? variables[statement.operand]
                                            : statement.operand;
    int result = 0;
    bool overflow = false;

    switch (statement.op) {
    case Op::Inc:
        overflow = __builtin_add_overflow(left, right, &result);
        break;
    case Op::Dec:
        overflow = __builtin_sub_overflow(left, right, &result);
        break;
    case Op::Mul:
        overflow = __builtin_mul_overflow(left, right, &result);
        break;
    case Op::Div:
        if (right == 0) {
            throw TeaError{statement.line, "division by zero"};
        }
        overflow = (left == INT_MIN && right == -1);
        if (!overflow) {
            result = left / right;
        }
        break;
    default:
        break;
    }

    if (overflow) {
        throw TeaError{statement.line, "integer overflow"};
    }
    variables[statement.variable] = result;
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc > 2) {
        std::cerr << "usage: tea [FILE]\n";
        return 2;
    }

    // Without a file name, the program comes from standard input.
    std::ifstream file;
    if (argc == 2) {
        file.open(argv[1]);
        if (!file) {
            std::cerr << "error: cannot open '" << argv[1] << "'\n";
            return 1;
        }
    }
    std::istream& source = (argc == 2) ? static_cast<std::istream&>(file) : std::cin;

    try {
        Interpreter interpreter;
        interpreter.load(source);
        interpreter.run();
    } catch (const TeaError& error) {
        std::cerr << "error: ";
        if (error.line > 0) {
            std::cerr << "line " << error.line << ": ";
        }
        std::cerr << error.message << '\n';
        return 1;
    }
    return 0;
}
