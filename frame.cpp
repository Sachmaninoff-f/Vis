#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>

class SingleRow {
public:
    //int index;
    std::vector<char> rawContent;
    std::vector<char> renderedContent;
    std::vector<char> highlightType;
    int hasOpenComment;
};

class SyntaxConfig {
public:
    std::vector<std::string> extensions;
    std::vector<std::string> keywords;
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

class TerminalControl {
private:
    struct termios orig_termios;
public:
    TerminalControl() {}
    ~TerminalControl() {}
};

enum KeyAction {};

void initializeLanguageTable() {}
int readAndTranslate(int fd) {}

int getCursorPosition(int ifd, int ofd, int &x, int &y) {}
int getWindowSize(int ifd, int ofd, int &rows, int &cols) {}

int readFile(const std::string &filename) {}
int writeFile(const std::string &filename) {}

void renderRow() {}

void inserChar() {}
void deleteChar() {}
void inserRow() {}
void deleteRow() {}

void refreshScreen() {}

