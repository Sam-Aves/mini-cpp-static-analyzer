
#include <bits/stdc++.h>
using namespace std;

// ---------- Token ----------
enum TokenType
{
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    SEPARATOR,
    STRING_LITERAL,
    CHAR_LITERAL,
    INVALID
};
struct Token
{
    string value;
    TokenType type;
    int line;
    int col;
};

// ---------- Symbol ----------
struct Symbol
{
    string type;
    bool initialized = false;
    bool used = false;
    int scopeLevel = 0;
    int size = -1; // arrays
};

map<string, Symbol> symbolTable;
int currentScope = 0;

// ---------- Error Containers ----------
struct Issue
{
    int line;
    string message;
    string codeLine;
    int col;
};
vector<Issue> lexicalErrors, syntaxErrors, semanticErrors, runtimeErrors, warnings;

// ---------- Settings ----------
set<string> keywords = {"int", "float", "double", "char", "string", "if", "else", "while", "for", "do", "return", "void"};
set<string> predefined = {"cout", "cin", "printf", "scanf", "main"};

bool isKeyword(const string &s) { return keywords.count(s); }
bool isPredefined(const string &s) { return predefined.count(s); }

// ---------- Error Reporter ----------
void report(vector<Issue> &container, const string &msg, int line, const string &codeLine, int col = -1)
{
    container.push_back({line, msg, codeLine, col});
}

// ---------- Lexer ----------
vector<Token> lexer(const vector<string> &lines)
{
    vector<Token> tokens;

    for (int lineNo = 0; lineNo < lines.size(); lineNo++)
    {
        string line = lines[lineNo];
        string originalLine = line;

        // Remove comments
        size_t commentPos = line.find("//");
        if (commentPos != string::npos)
            line = line.substr(0, commentPos);

        // Trim trailing spaces
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty())
            continue;

        if (!line.empty() && line[0] == '#')
            continue;

        string word = "";

        for (int i = 0; i <= (int)line.size(); i++)
        {
            char ch = (i < line.size()) ? line[i] : ' ';

            // ---------- STRING LITERAL ----------
            if (ch == '"')
            {
                string str = "";
                int startCol = i + 1;
                i++;

                while (i < line.size() && line[i] != '"')
                    str += line[i++];

                if (i >= line.size())
                    report(lexicalErrors, "Unterminated string literal", lineNo + 1, originalLine, startCol);
                else
                    tokens.push_back({str, STRING_LITERAL, lineNo + 1, startCol});

                continue;
            }

            // ---------- CHAR LITERAL ----------
            if (ch == '\'')
            {
                string str = "";
                int startCol = i + 1;
                i++;

                if (i < line.size() && i + 1 < line.size() && line[i + 1] == '\'')
                {
                    str += line[i];
                    tokens.push_back({str, CHAR_LITERAL, lineNo + 1, startCol});
                    i += 2;
                }
                else
                {
                    report(lexicalErrors, "Invalid char literal", lineNo + 1, originalLine, startCol);
                }

                continue;
            }

            // ---------- IDENTIFIER / NUMBER BUILDING ----------
            if (isalnum(ch) || ch == '_' || ch == '.')
            {
                word += ch;
            }
            else
            {
                if (!word.empty())
                {
                    int startCol = i - word.size() + 1;

                    bool isNumber = true;
                    int dotCount = 0;

                    for (char c : word)
                    {
                        if (c == '.')
                            dotCount++;
                        else if (!isdigit(c))
                            isNumber = false;
                    }

                    if (dotCount > 1)
                        isNumber = false;

                    if (isNumber && (isdigit(word[0]) || word[0] == '.'))
                    {
                        tokens.push_back({word, NUMBER, lineNo + 1, startCol});
                    }
                    else if (isKeyword(word))
                    {
                        tokens.push_back({word, KEYWORD, lineNo + 1, startCol});
                    }
                    else
                    {
                        tokens.push_back({word, IDENTIFIER, lineNo + 1, startCol});
                    }

                    word = "";
                }

                // ---------- OPERATORS ----------
                if (ch == '<' || ch == '>' || ch == '=' || ch == '!' ||
                    ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%')
                {
                    string op(1, ch);

                    if (i + 1 < line.size())
                    {
                        char next = line[i + 1];

                        if ((ch == '<' && next == '=') ||
                            (ch == '>' && next == '=') ||
                            (ch == '=' && next == '=') ||
                            (ch == '!' && next == '=') ||
                            (ch == '+' && next == '+') ||
                            (ch == '-' && next == '-') ||
                            (ch == '<' && next == '<') ||
                            (ch == '>' && next == '>'))
                        {
                            op += next;
                            i++;
                        }
                    }

                    tokens.push_back({op, OPERATOR, lineNo + 1, i + 1});
                }

                // ---------- SEPARATORS ----------
                else if (ch == ';' || ch == '{' || ch == '}' ||
                         ch == '(' || ch == ')' || ch == ',' ||
                         ch == '[' || ch == ']')
                {
                    tokens.push_back({string(1, ch), SEPARATOR, lineNo + 1, i + 1});
                }

                // ---------- INVALID CHAR ----------
                else if (!isspace(ch) && ch != '\0')
                {
                    report(lexicalErrors,
                           "Invalid character '" + string(1, ch) + "'",
                           lineNo + 1,
                           originalLine,
                           i + 1);
                }
            }
        }
    }

    return tokens;
}
// ---------- Type Utilities ----------
string inferLiteralType(const Token &t)
{
    if (t.type == NUMBER)
    {
        if (t.value.find('.') != string::npos)
            return "float";
        return "int";
    }
    if (t.type == STRING_LITERAL)
        return "string";
    if (t.type == CHAR_LITERAL)
        return "char";
    return "unknown";
}

bool isTypeCompatible(const string &lhs, const string &rhs)
{
    if (lhs == rhs)
        return true;
    if (lhs == "float" && rhs == "int")
        return true;
    if (lhs == "double" && (rhs == "int" || rhs == "float"))
        return true;
    // int can take char
    if (lhs == "int" && rhs == "char")
        return true;

    // char only accepts char
    if (lhs == "char" && rhs == "char")
        return true;
    return false;
}
bool isOpening(const string &s)
{
    return s == "{" || s == "(" || s == "[";
}

bool isClosing(const string &s)
{
    return s == "}" || s == ")" || s == "]";
}

bool isMatching(const string &open, const string &close)
{
    return (open == "{" && close == "}") ||
           (open == "(" && close == ")") ||
           (open == "[" && close == "]");
}

// ---------- Parser + Semantic Analyzer ----------
void parse(vector<Token> &tokens, const vector<string> &lines)
{
    // stack<Token> braceStack;
    stack<Token> delimiterStack;

    vector<pair<int, int>> ifBraces;
    // Track which lines have braces
    vector<int> openingBraceLines;
    vector<int> closingBraceLines;

    // First, collect all brace information
    for (int i = 0; i < tokens.size(); i++)
    {

        Token t = tokens[i];
        if (isOpening(t.value))
        {
            delimiterStack.push(t);
            if (t.value == "{")
                currentScope++;
        }
        else if (isClosing(t.value))
        {
            if (delimiterStack.empty())
            {
                report(syntaxErrors,
                       "Unmatched closing '" + t.value + "'",
                       t.line,
                       lines[t.line - 1],
                       t.col);
            }
            else
            {
                Token open = delimiterStack.top();
                delimiterStack.pop();

                if (!isMatching(open.value, t.value))
                {
                    report(syntaxErrors,
                           "Mismatched '" + open.value + "' and '" + t.value + "'",
                           t.line,
                           lines[t.line - 1],
                           t.col);
                }

                if (t.value == "}")
                    currentScope--;
            }
        }
    }

    // Now parse for other errors
    for (int i = 0; i < tokens.size(); i++)
    {
        Token t = tokens[i];

        // Variable declarations
        if (t.type == KEYWORD && (t.value == "int" || t.value == "float" || t.value == "double" || t.value == "char" || t.value == "string"))
        {
            if (i + 1 >= tokens.size() || tokens[i + 1].type != IDENTIFIER)
            {
                report(syntaxErrors,
                       "Invalid declaration: expected identifier after type",
                       t.line,
                       lines[t.line - 1],
                       t.col);
            }
            if (i + 1 < tokens.size() && tokens[i + 1].type == IDENTIFIER)
            {
                // Check if this is a function declaration (has parentheses)
                bool isFunction = false;
                for (int j = i + 2; j < tokens.size() && tokens[j].line == t.line; j++)
                {
                    if (tokens[j].value == "(")
                    {
                        isFunction = true;
                        break;
                    }
                    if (tokens[j].value == ";" || tokens[j].value == "=" || tokens[j].value == "[")
                        break;
                }

                if (!isFunction)
                {
                    string varName = tokens[i + 1].value;
                    int size = -1;

                    // Array declaration
                    if (i + 2 < tokens.size() && tokens[i + 2].value == "[")
                    {
                        if (i + 3 < tokens.size() && tokens[i + 3].type == NUMBER)
                        {
                            size = stoi(tokens[i + 3].value);
                        }
                        if (i + 4 >= tokens.size() || tokens[i + 4].value != "]")
                        {
                            report(syntaxErrors, "Expected closing bracket for array", tokens[i + 2].line,
                                   lines[tokens[i + 2].line - 1], tokens[i + 2].col);
                        }
                    }

                    // Check redeclaration
                    if (symbolTable.count(varName) && symbolTable[varName].scopeLevel == currentScope)
                    {
                        report(semanticErrors, "Redeclaration of variable '" + varName + "'",
                               tokens[i + 1].line, lines[tokens[i + 1].line - 1], tokens[i + 1].col);
                    }
                    else
                    {
                        symbolTable[varName] = {t.value, false, false, currentScope, size};
                    }

                    string line = lines[t.line - 1];

                    // trim whitespace
                    line.erase(line.find_last_not_of(" \t\n\r") + 1);

                    if (!line.empty() && line.back() != ';' &&
                        line.back() != '{' &&
                        line.back() != '}' &&
                        line.find("if") != 0 &&
                        line.find("while") != 0 &&
                        line.find("for") != 0)
                    {
                        report(syntaxErrors,
                               "Missing semicolon",
                               t.line,
                               lines[t.line - 1],
                               t.col);
                    }
                }
            }
        }

        // Type checking on assignment
        if (t.type == IDENTIFIER && i + 1 < tokens.size() && tokens[i + 1].value == "=")
        {
            string varName = t.value;
            // 🔴 UNDECLARED VARIABLE CHECK (ADD THIS)
            if (!symbolTable.count(varName))
            {
                report(semanticErrors,
                       "Use of undeclared variable '" + varName + "'",
                       t.line,
                       lines[t.line - 1],
                       t.col);
                // stop further checks for this line
            }

            if (symbolTable.count(varName))
            {
                symbolTable[varName].used = true;
                symbolTable[varName].initialized = true;

                // Find the RHS value
                for (int j = i + 2; j < tokens.size() && tokens[j].line == t.line; j++)
                {
                    if (tokens[j].type == STRING_LITERAL || tokens[j].type == NUMBER || tokens[j].type == CHAR_LITERAL)
                    {
                        string lhsType = symbolTable[varName].type;
                        string rhsType = inferLiteralType(tokens[j]);

                        if (rhsType != "unknown" && !isTypeCompatible(lhsType, rhsType))
                        {
                            report(semanticErrors, "Type mismatch: cannot assign '" + rhsType + "' to '" + lhsType + "'",
                                   t.line, lines[t.line - 1], t.col);
                        }
                        break;
                    }
                }
            }
        }

        // Division by zero check
        if (t.value == "/" && i + 1 < tokens.size())
        {
            for (int j = i + 1; j < tokens.size() && tokens[j].line == t.line; j++)
            {
                if (tokens[j].type == NUMBER && tokens[j].value == "0")
                {
                    report(runtimeErrors, "Division by zero", t.line, lines[t.line - 1], t.col);
                    break;
                }
                if (tokens[j].value == ";" || tokens[j].value == ",")
                    break;
            }
        }

        // Array bounds check - ONLY on array access, not declaration
        if (t.type == IDENTIFIER && i + 1 < tokens.size() && tokens[i + 1].value == "[")
        {
            string varName = t.value;

            // Skip if this is an array declaration
            bool isDeclaration = false;
            if (i > 0 && tokens[i - 1].type == KEYWORD &&
                (tokens[i - 1].value == "int" || tokens[i - 1].value == "float" ||
                 tokens[i - 1].value == "double" || tokens[i - 1].value == "char"))
            {
                isDeclaration = true;
            }

            if (!isDeclaration && symbolTable.count(varName) && symbolTable[varName].size > 0)
            {
                // Find the index
                for (int j = i + 2; j < tokens.size() && tokens[j].line == t.line && tokens[j].value != "]"; j++)
                {
                    if (tokens[j].type == NUMBER)
                    {
                        int idx = stoi(tokens[j].value);
                        if (idx >= symbolTable[varName].size)
                        {
                            warnings.push_back({t.line, "Array index out-of-bounds for '" + varName + "'",
                                                lines[t.line - 1], t.col});
                        }
                        break;
                    }
                }
            }
        }
    }

    // Also check general unmatched braces (your existing code)
    while (!delimiterStack.empty())
    {
        Token open = delimiterStack.top();
        delimiterStack.pop();

        report(syntaxErrors,
               "Missing closing for '" + open.value + "'",
               open.line,
               lines[open.line - 1],
               open.col);
    }
}

// ---------- Display Function ----------
void displayIssues()
{
    auto printSection = [](const string &title, vector<Issue> &v, const string &color)
    {
        if (v.empty())
            return;
        cout << color << "\n==== " << title << " ====\033[0m\n";
        for (auto &e : v)
        {
            cout << "Line " << e.line << ": " << e.message << "\n";
            if (!e.codeLine.empty())
            {
                // Clean up the code line
                string cleanLine = e.codeLine;
                size_t commentPos = cleanLine.find("//");
                if (commentPos != string::npos)
                {
                    cleanLine = cleanLine.substr(0, commentPos);
                }
                // Trim
                cleanLine.erase(cleanLine.find_last_not_of(" \t") + 1);
                cout << "  >> " << cleanLine << "\n";

                if (e.col > 0)
                {
                    cout << "     ";
                    for (int i = 1; i < e.col; i++)
                        cout << " ";
                    cout << "^\n";
                }
            }
        }
    };

    printSection("Lexical Errors", lexicalErrors, "\033[1;31m");
    printSection("Syntax Errors", syntaxErrors, "\033[1;31m");
    printSection("Semantic Errors", semanticErrors, "\033[1;31m");
    printSection("Runtime Errors", runtimeErrors, "\033[1;31m");
    printSection("Warnings", warnings, "\033[1;33m");

    int totalErrors = lexicalErrors.size() + syntaxErrors.size() + semanticErrors.size() + runtimeErrors.size();
    cout << "\n\033[1;36m==== Summary ====\033[0m\n";
    cout << "Total Errors: " << totalErrors << "\n";
    cout << "Total Warnings: " << warnings.size() << "\n";
}

// ---------- Main ----------
int main()
{
    cout << "\033[1;36m==== MINI C++ STATIC ANALYZER ====\033[0m\n\n";
    cout << "\033[1;32mEnter C++ code line by line (type END to finish):\033[0m\n";
    cout << "\033[1;32m............INPUT.............:\033[0m\n";

    vector<string> lines;
    string line;
    while (getline(cin, line))
    {
        if (line == "END")
            break;
        lines.push_back(line);
    }

    cout << "\033[1;32m............OUTPUT.............:\033[0m\n";
    vector<Token> tokens = lexer(lines);
    parse(tokens, lines);
    displayIssues();

    if (lexicalErrors.empty() && syntaxErrors.empty() && semanticErrors.empty() && runtimeErrors.empty())
        cout << "\n\033[1;32mNo errors detected. \033[0m\n";
    else
        cout << "\n\033[1;31mError detection complete. \033[0m\n";

    return 0;
}
