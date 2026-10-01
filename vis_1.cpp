#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>

enum Mode {
    COMMAND = 0,
    INPUT = 1,
    LINECMD = 2
};
enum HLType {
    NORMAL,
    NONPRINT,
    SLCOMMENT,
    MLCOMMENT,
    KEYWORD,
    TYPE,
    STRING,
    NUMBER,
    FUNCTION,
    VISBLE
};
enum KeyAction {
    KEY_NULL = 0,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_F = 6,
    CTRL_H = 8,
    TAB = 9,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_Q = 17,
    CTRL_S = 19,
    CTRL_U = 27,
    ESC = 27,
    BACKSPACE = 127,

    ARROW_LEFT = 1000,
    ARROW_DOWN = 1001,
    ARROW_RIGHT = 1002,
    ARROW_UP = 1003,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

class SyntaxConfig {
public:
    std::vector<std::string> extenstions;
    std::vector<std::string> keywords;
    std::vector<std::string> types;
    //std::vector<std::string> functions;
    std::vector<char> scs; // Single line Comment Start;
    std::vector<char> mcs; // Multi line Comment Start;
    std::vector<char> mce; // Multi line Comment End;
};
class SingleRow {
private:
    std::vector<char> rawRow;
    std::vector<char> renderRow;
    std::vector<HLType> hlType;
    int hasOpenComment = 0;
public:
    SingleRow(const std::string &str) {
        rawRow = std::vector<char> (str.begin(), str.end());
    }
    SingleRow(const std::vector<char> &vec) {
        rawRow = vec;
    }
    const int hasOC() const {
        return hasOpenComment;
    }
    const std::vector<char> &getRawRow() const {
        return rawRow;
    }
    void render() {
        renderRow.clear();
        for (auto &x : rawRow) {
            if (isprint(x)) {
                renderRow.push_back(x);
            } else if (x == '\t') {
                do {
                    renderRow.push_back(' ');
                } while (renderRow.size() % 4 != 0);
            }
        }
    }
    void findFunc() {
        std::string funcName;
        for (auto &c : renderRow) {
            if (c == '(') {
                EC.syntaxAddFunc(funcName);
                funcName.clear();
            } else if (isSeperate(c)) {
                funcName.clear();
            } else {
                funcName.push_back(c);
            }
        }
    };
    void highlight(const std::vector<std::string> &funcTable) {
        SyntaxConfig &SC = EC.getSC();
        std::vector<char> scs = SC.scs;
        std::vector<char> mcs = SC.mcs;
        std::vector<char> mce = SC.mce;
        hlType = std::vector<HLType>(renderRow.size(), NORMAL);
        int inSLComment = 0;
        int inMLComment = 0;
        int inString = 0;
        char stringStart;
        std::string cStr;
        int startAt = 0;
        if (EC.lastHasOC()) inMLComment = 1;
        for (int i = 0; i < renderRow.size(); i++) {
            if (inSLComment) {
                
            }
        }
    }
    void syntaxUpdate(const std::vector<std::string> &funcTable) {
        render();
        findFunc();
        highlight(funcTable);
    }
    void insertChar(char c, int colIdx) {
        rawRow.insert(rawRow.begin() + colIdx, c);
    }
    void deleteChar(int colIdx) {
        rawRow.erase(rawRow.begin() + colIdx);
    }
    void deleteRight(int colIdx) {
        rawRow.erase(rawRow.begin() + colIdx, rawRow.end());
    }
};

class EditorConfig {
private:
    int dirtyFlag;
    Mode editorMode;
    int rowOff, colOff;
    int windowRows, windowCols;
    int rowIdx, colIdx;
    std::vector<SingleRow> row;
    std::string promptLine;
    SyntaxConfig *editorSyntax;
    std::vector<std::string> funcTable;
    std::vector<int> funcAt;
public:
    EditorConfig() {
        row = std::vector<SingleRow>(1);
    }
    int lastHasOC() {
        return row[rowIdx - 1].hasOC();
    }
    SyntaxConfig &getSC() {
        return *editorSyntax;
    }
    void getWindowSize() {
        struct winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            throw std::runtime_error("get window size failed.");
        } else {
        windowRows = ws.ws_row;
        windowCols = ws.ws_col;
        }
    }
    void readFile(const std::string &fileName) {
        std::ifstream file(fileName);
        if (!file.is_open()) {
            if (errno != ENOENT) {
                throw std::runtime_error("fail to open file.");
            }
        } else {
            std::string line;
            while (std::getline(file, line)) {
                row.push_back(SingleRow(line));
            }
        }
        file.close();
    }
    void writefile(const std::string &fileName) {
        std::ofstream file(fileName);
        if (!file.is_open()) {
            throw std::runtime_error("fail to write file.");
        } else {
            for (auto &x : row) {
                file.write(x.getRawRow().data(), x.getRawRow().size());
                file << '\n';
            }
        }
    }
    void syntaxAddFunc(const std::string &funcName) {
        auto at = find(funcTable.begin(), funcTable.end(), funcName);
        if (at == funcTable.end()) { //fine a new function
            funcTable.push_back(funcName);
            funcAt.push_back(rowIdx);
            syntaxUpdateAll();
        }
    }
    void clearFunc() { //clear functable and funcat that in current rowIdx
        for (int i = funcTable.size() - 1; i >= 0; i--) {
            if (funcAt[i] == rowIdx) {
                funcAt.erase(funcAt.begin() + i);
                funcTable.erase(funcTable.begin() + i);
            }
        }
    }
    void syntaxUpdateAll() {
        for (auto &x : row) x.syntaxUpdate(funcTable);
    }
    void updateNextRow() {
        if (rowIdx + 1 < row.size()) {
            rowIdx++;
            row[rowIdx].syntaxUpdate(funcTable);
            rowIdx--;
        }
    }
    void inserChar(char c) {
        row[rowIdx].insertChar(c, colIdx);
        colIdx++;
        clearFunc();
        row[rowIdx].syntaxUpdate(funcTable);
    }
    void deleteChar() {
        std::vector<char> thisrow = row[rowIdx].getRawRow();
        if (colIdx >= thisrow.size()) {
            if (rowIdx == row.size() - 1) {
                return;
            } else {
                std::vector<char> nextrow = row[rowIdx + 1].getRawRow();
                std::vector<char> newrow;
                for (auto &x : thisrow) newrow.push_back(x);
                for (auto &x : nextrow) newrow.push_back(x);
                row[rowIdx] = SingleRow(newrow);
                rowIdx++;
                deleteRow();
                rowIdx--;
            }
        } else {
            row[rowIdx].deleteChar(colIdx);
            clearFunc();
        }
        row[rowIdx].syntaxUpdate(funcTable);
    }
    void backSpace() {
        if (colIdx > 0) {
            colIdx--;
            deleteChar();
        } else if (rowIdx == 0) {
            return;
        } else {
            rowIdx--;
            colIdx = row[rowIdx].getRawRow().size();
            deleteChar();
        }
    }
    void insertRow() {
        std::vector<char> cRow = row[rowIdx].getRawRow();
        std::vector<char> newLine;
        for (auto it = cRow.begin() + colIdx; it < cRow.end(); it++) {
            newLine.push_back(*it);
        }
        row[rowIdx].deleteRight(colIdx);
        for (auto &x : funcAt) {
            if (x > rowIdx) x++;
        }
        clearFunc();
        row[rowIdx].syntaxUpdate(funcTable);
        rowIdx++;
        clearFunc();
        colIdx = 0;
        row.insert(row.begin() + rowIdx, SingleRow(newLine));
        row[rowIdx].syntaxUpdate(funcTable);
    }
    void deleteRow() {
        if (row.size() == 1) {
            row = std::vector<SingleRow>(1);
            clearFunc();
            row[rowIdx].syntaxUpdate(funcTable);
            colIdx = 0;
        } else {
            clearFunc();
            for (auto &x : funcAt) {
                if (x > rowIdx) x--;
            }
            row.erase(row.begin() + rowIdx);
        }
    }
    void refreshScreen() {}
};
static EditorConfig EC;

class TerminalControl {
private:
    struct termios origTermios;
public:
    TerminalControl() {
        struct termios rawTermios;
        if (tcgetattr(STDIN_FILENO, &rawTermios) == -1) throw std::runtime_error("tcgetattr failed.");
        rawTermios = origTermios;
        rawTermios.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        rawTermios.c_oflag &= ~(OPOST);
        rawTermios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
        rawTermios.c_cflag |= (CS8);
        rawTermios.c_cc[VMIN] = 0;
        rawTermios.c_cc[VTIME] = 1;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios) == -1) throw std::runtime_error("tcsetattr failed.");
    }
    ~TerminalControl() {
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios) == -1) throw std::runtime_error("recover rawTermios failed.");
    }
};

bool isSeperate(const char c) {
    return !std::isalnum(c) && c != '_'; // letter, number, or _
}

SyntaxConfig syntaxTable = {};

int main(int argc, char *argv[]) {
    
}