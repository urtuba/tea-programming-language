/*
 * AUTHOR: Samed Kahyaoglu
 * GITHUB: urtuba
 * Restored: 2026
 *
 * Tea is a tiny language with functions and five variables (a to e) per call.
 * This file is its interpreter: it loads a program and runs it from main.
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace {

// Limits set by the instructor in 2019.
const int MAX_STATEMENTS = 100;
const int MAX_DEPTH = 5;  // main plus 4 nested calls
const int VARIABLE_COUNT = 5;

struct Statement {
    std::string type;  // inc, dec, mul, div, function, call or return
    std::string arg1;
    std::string arg2;  // empty for function and return
};

// Maps the names a to e to 0 to 4. Any other name gives -1.
int variableIndex(const std::string& name)
{
    if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'e') {
        return name[0] - 'a';
    }
    return -1;
}

class Interpreter {
public:
    // Reads the statements of a program from a file.
    void load(const char* path);

    // Runs the program from the statement after "function main".
    void run();

private:
    // Executes the statement at the given address and returns the address
    // of the next one.
    int step(int address);

    // Returns the address of the "function NAME" statement, or the statement
    // count if there is none.
    int findFunction(const std::string& name) const;

    void callFunction(const Statement& statement, int address);
    int returnFromFunction(const Statement& statement);
    void finishMain(const Statement& statement);
    void calculate(const Statement& statement);

    Statement statements_[MAX_STATEMENTS];
    int statementCount_ = 0;

    // One row of variables per active call. Row 0 belongs to main.
    int variables_[MAX_DEPTH][VARIABLE_COUNT] = {};
    // Address of the call statement that started each active call.
    int callAddress_[MAX_DEPTH] = {};
    int depth_ = 0;
    bool done_ = false;
};

void Interpreter::load(const char* path)
{
    std::ifstream file(path);
    std::string word;
    while (statementCount_ < MAX_STATEMENTS && file >> word) {
        Statement& statement = statements_[statementCount_++];
        statement.type = word;
        // function and return take one argument, the others take two.
        file >> statement.arg1;
        if (word != "function" && word != "return") {
            file >> statement.arg2;
        }
    }
}

void Interpreter::run()
{
    // Start after "function main".
    int address = 0;
    while (address < statementCount_ && statements_[address].arg1 != "main") {
        address++;
    }
    address++;

    while (!done_ && address < statementCount_) {
        address = step(address);
    }
}

int Interpreter::step(int address)
{
    const Statement& statement = statements_[address];
    std::cout << "Executing " << statement.type << " at line " << address << '\n';

    if (statement.type == "call") {
        callFunction(statement, address);
        return findFunction(statement.arg1) + 1;
    }
    if (statement.type == "return") {
        if (depth_ > 0) {
            return returnFromFunction(statement);
        }
        finishMain(statement);
        return 0;
    }
    calculate(statement);
    return address + 1;
}

int Interpreter::findFunction(const std::string& name) const
{
    int address = 0;
    while (address < statementCount_ &&
           !(statements_[address].type == "function" &&
             statements_[address].arg1 == name)) {
        address++;
    }
    return address;
}

// Starts a new call. All its variables are 0 except the one passed in.
void Interpreter::callFunction(const Statement& statement, int address)
{
    int passed = variableIndex(statement.arg2);
    depth_++;
    for (int i = 0; i < VARIABLE_COUNT; i++) {
        variables_[depth_][i] = (i == passed) ? variables_[depth_ - 1][i] : 0;
    }
    callAddress_[depth_] = address;
}

// Copies the returned variable into the same variable of the caller and
// returns the address after the call statement.
int Interpreter::returnFromFunction(const Statement& statement)
{
    int returned = variableIndex(statement.arg1);
    if (returned >= 0) {
        variables_[depth_ - 1][returned] = variables_[depth_][returned];
    }
    return callAddress_[depth_--] + 1;
}

// "return" in main prints the variable and ends the program.
void Interpreter::finishMain(const Statement& statement)
{
    int returned = variableIndex(statement.arg1);
    if (returned >= 0) {
        std::cout << variables_[0][returned];
    }
    std::cout << '\n';
    done_ = true;
}

// Runs inc, dec, mul or div on the variables of the current call.
void Interpreter::calculate(const Statement& statement)
{
    int target = variableIndex(statement.arg1);
    int* variables = variables_[depth_];
    // The second argument is a variable or a number.
    int operandVariable = variableIndex(statement.arg2);
    int operand = operandVariable >= 0 ? variables[operandVariable]
                                       : std::atoi(statement.arg2.c_str());

    if (target < 0) {
        return;
    }
    if (statement.type == "inc") {
        variables[target] += operand;
    } else if (statement.type == "dec") {
        variables[target] -= operand;
    } else if (statement.type == "mul") {
        variables[target] *= operand;
    } else if (statement.type == "div") {
        variables[target] /= operand;
    }
}

}  // namespace

int main(int argc, char const* argv[])
{
    (void)argc;
    Interpreter interpreter;
    interpreter.load(argv[1]);
    interpreter.run();
    return 0;
}
