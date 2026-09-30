#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>

#define HIGHLIGHT_NORMAL 0
#define HIGHLIGHT_NONPRINT 1
#define HIGHLIGHT_COMMENT 2
#define HIGHLIGHT_MULITLINECOMMENT 3
#define HIGHLIGHT_KEYWORD 4
#define HIGHLIGHT_TYPE 5
#define HIGHLIGHT_STRING 6
#define HIGHLIGHT_NUMBER 7
#define HIGHLIGHT_VISABLEMODE 8

/*
class Language {
public:
    std::vector<std::string> extensions;
    std::vector<std::string> keywords;
};
std::vector<Language> Languages = {
    {
        {".c", ".h", ".cpp", ".hpp"},
        {"int", "short"}
    }
};
*/

class SingleRow {
public:
    //int index;
    std::string rawContent;
    std::vector<char> renderedContent;
    std::vector<char> highlightType;
    int hasOpenComment;
    SingleRow(std::string str) {
        rawContent = str;
    }
};

class SyntaxConfig {
public:
    std::vector<std::string> extensions;
    std::vector<std::string> keywords;
    std::vector<std::string> types;
    std::vector<std::string> functions;
    std::vector<char> singleLineCommentStart;
    std::vector<char> multiLineCommentStart;
    std::vector<char> multiLineCommentEnd;
};

class EditorSetting {
public:
    int autoWrap;
};
static EditorSetting ES;

class EditorConfig {
public:
    int dirtyFlag;
    int editorMode; /* 0 for command mode, 1 for input mode, 2 for line command mode. */
    //int rawMode;
    int rowsOffset, colsOffset;
    int windowRows, windowCols;
    int cursor_x, cursor_y; // x down, y right, ** POSITION IN RAWROW **
    std::vector<SingleRow> row;
    std::string promptLine;
    SyntaxConfig *editorSyntax;
};
static EditorConfig EC;

// change to raw mode at start of prog, and recover it when exit.
class TerminalControl {
private:
    struct termios orig_termios;
public:
    TerminalControl() {
        struct termios raw_termios;
        if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) throw std::runtime_error("tcgetattr failed.");
        raw_termios = orig_termios;
        raw_termios.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw_termios.c_oflag &= ~(OPOST);
        raw_termios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
        raw_termios.c_cflag |= (CS8);
        raw_termios.c_cc[VMIN] = 0;
        raw_termios.c_cc[VTIME] = 1;
        if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1) throw std::runtime_error("tcsetattr failed.");
    }
    ~TerminalControl() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
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

void initializeLanguageTable() {}
int readAndTranslate(int fd) {}
int getCursorPosition(int ifd, int ofd, int &x, int &y) {
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;
    std::vector<char> buffer = std::vector<char>(32);
    int tempx, tempy;
    for (int i = 0; i < buffer.size(); i++) {
        if (read(ifd, &buffer[i], 1) != 1) break;
        if (buffer[i] == 'R') break;
    }
    if (buffer[0] != '\x1b') return -1;
    if (sscanf(&buffer[2], "%d;%d",&tempx, &tempy) != 2) return -1;
    x = tempx - 1;
    y = tempy - 1;
    return 0;
}
int getWindowSize(int ifd, int ofd, int &rows, int &cols) {
    struct winsize ws;
    if (ioctl(ifd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) return -1;
    rows = ws.ws_row;
    cols = ws.ws_col;
    return 0;
}

int readFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        if (errno != ENOENT) {
            std::cerr << "Opening File!" << std::endl;
            return -1;
        }
        return 0;
    }
    std::string line;
    while (std::getline(file, line)) {
        EC.row.push_back(SingleRow(line));
    }
    file.close();
}
int writeFile(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Fail to Write File!" << std::endl;
        return -1;
    }
    for (auto &x : EC.row) {
        file.write(x.rawContent.data(), x.rawContent.size());
        file << '\n';
    }
    return 0;
}

bool isSeperate(const char c) {
    return !std::isalnum(c) && c != '_';
}
void renderRow(int rowIndex) {
    SingleRow &c_row = EC.row[rowIndex];
    c_row.renderedContent.clear();
    for (int i = 0; i < c_row.rawContent.size(); i++) {
        if (isprint((unsigned char)c_row.rawContent[i])) {
            c_row.renderedContent.push_back(c_row.rawContent[i]);
        }
        if (c_row.rawContent[i] == '\t') {
            c_row.renderedContent.push_back(' ');
            while ( c_row.renderedContent.size() % 4 != 0) {
                c_row.renderedContent.push_back(' ');
            }
        }
    }
}
void highlightRow(int rowIndex) {
    SingleRow &c_row = EC.row[rowIndex];
    c_row.highlightType.clear();
    std::vector<char> scs = EC.editorSyntax->singleLineCommentStart;
    std::vector<char> mcs = EC.editorSyntax->multiLineCommentStart;
    std::vector<char> mce = EC.editorSyntax->multiLineCommentEnd;
    //int inMultiComment = 0;
    //int inString = 0;
    int i = 0;
    char stringStart;

    if (rowIndex > 0 && EC.row[rowIndex - 1].hasOpenComment) goto INMULTICOMMENT;
    else goto NORMAL;
INSINGLECOMMENT:
    c_row.hasOpenComment = 0;
    while (i < c_row.renderedContent.size()) {
        c_row.highlightType.push_back(HIGHLIGHT_COMMENT);
        i++;
    }
    return;
INMULTICOMMENT:
    while (i < c_row.renderedContent.size()) {
        if (i + mce.size() < c_row.renderedContent.size()) {
            if (equal(
                c_row.renderedContent.begin() + i,
                c_row.renderedContent.begin() + i + mce.size(),
                mce.begin(),
                mce.end())) {
                for (int j = 0; j < mce.size(); j++) {
                    c_row.highlightType.push_back(HIGHLIGHT_MULITLINECOMMENT);
                }
                goto NORMAL;
            }
        }
        c_row.highlightType.push_back(HIGHLIGHT_MULITLINECOMMENT);
        i++;
    }
    c_row.hasOpenComment = 1;
    return;
INSTRING:
    while (i < c_row.renderedContent.size()) {
        if (c_row.renderedContent[i] == stringStart) {
            c_row.highlightType.push_back(HIGHLIGHT_STRING);
            i++;
            goto NORMAL;
        }
        c_row.highlightType.push_back(HIGHLIGHT_STRING);
        i++;
    }
NORMAL:
    while (i < c_row.renderedContent.size()) {
        if (i + scs.size() < c_row.renderedContent.size() &&
            equal(
            c_row.renderedContent.begin() + i,
            c_row.renderedContent.begin() + i + scs.size(),
            scs.begin(),
            scs.end())) goto INSINGLECOMMENT;
        if (i + mcs.size() < c_row.renderedContent.size() && 
            equal(
            c_row.renderedContent.begin() + i,
            c_row.renderedContent.begin() + i + mce.size(),
            mce.begin(),
            mce.end())) goto INMULTICOMMENT;
        if (c_row.renderedContent[i] == '\'' || c_row.renderedContent[i] == '\"') {
            stringStart = c_row.renderedContent[i];
            goto INSTRING;
        }
        c_row.highlightType.push_back(HIGHLIGHT_NORMAL);
        i++;
    }
    return;
}
void syntaxUpdate(int rowIndex) {}

void insertChar() {}
void deleteChar() {}

void insertRow() {}
void deleteRow() {}

void refreshScreen() {}


int main(int argc, char *argv[]) {
    static TerminalControl term;
    if(argc != 2) {
        std::cerr << "" << std::endl;
        return 1;
    }
}