#include <iostream>
#include <vector>
#include <termios.h>

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
    int index;
    std::vector<char> rawContent;
    std::vector<char> renderedContent;
    std::vector<char> highlightType;
    int hasOpenComment;
};

class SyntaxConfig {
public:
    std::vector<std::string> extensions;
    std::vector<std::string> keywords;
    std::vector<char> singleLineCommentStart;
    std::vector<char> multiLineCommentStart;
    std::vector<char> multiLineCommentEnd;
};

class EditorSetting {
public:
    int autoWrap;
};

class EditorConfig {
public:
    int dirtyFlag;
    int editorMode; /* 0 for command mode, 1 for input mode, 2 for line command mode. */
    int rawMode;
    int windowRows, windowCols;
    int cursor_x, cursor_y;
    std::string promptLine;
};
// change to raw mode at start of prog, and recover it when exit.
class TerminalControl {
private:
    struct termios orig_termios;
public:
    TerminalControl() {}
    ~TerminalControl() {}
};

enum KeyAction {

};

void initializeLanguageTable() {}
int readAndTranslate(int fd) {}
int getCursorPosition(int ifd, int ofd, int &cursor_x, int &cursor_y) {}
int getWindowSize(int ifd, int ofd, int &rows, int &cols) {}

void readFile() {}
void writeFile() {}
void renderRow() {}
void writeBackRawRow() {}

void insertChar() {}
void deleteChar() {}

void insertRow() {}
void deleteRow() {}

void refreshScreen() {}