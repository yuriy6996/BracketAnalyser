#ifndef ANALYZER_H
#define ANALYZER_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QStack>
#include <QFile>
#include <QTextStream>
#include <QHash>

// Состояния автомата
enum ParserState {
    Normal, InString, InChar, InSingleLineComment,
    InMultiLineComment, InIncludeAngleBrackets, InIncludeQuotes
};

enum BracketType { Round, Square, Curly, Unknown };

enum ErrorType {
    NoError, InvalidArgs, FileOpenError, FileWriteError,
    LineLimitExceeded, CharLimitExceeded, MacroFound,
    UnmatchedOpenBracket, UnmatchedCloseBracket
};

// Статусы проверки закрывающей скобки
enum BracketCheckStatus { Match, Mismatch, StackEmpty };

// Позиция скобки в тексте
struct BracketPosition {
    BracketType type;
    QChar character;
    int line;
    int column;
};

// Класс ошибки для QSet
class ErrorMessage {
public:
    ErrorType type;
    int line;
    int column;
    QString details;

    ErrorMessage(ErrorType t, int l, int c, QString d = "")
        : type(t), line(l), column(c), details(d) {}


    bool operator==(const ErrorMessage& other) const {
        return type == other.type && line == other.line && column == other.column;
    }

    // Генерация готового текста для файла
    QString generateText() const {
        QString typeStr;
        switch(type) {
            case LineLimitExceeded: typeStr = "Превышен лимит строк"; break;
            case CharLimitExceeded: typeStr = "Превышена длина строки"; break;
            case MacroFound: typeStr = "Обнаружен макрос #define"; break;
            case UnmatchedOpenBracket: typeStr = "Незакрытая открывающая скобка"; break;
            case UnmatchedCloseBracket: typeStr = "Лишняя или несоответствующая закрывающая скобка"; break;
            default: typeStr = "Неизвестная ошибка";
        }
        return QString("[Строка %1, Колонка %2] Ошибка: %3 %4").arg(line).arg(column).arg(typeStr).arg(details);
    }
};

// Хэш-функция, необходимая для QSet, чтобы он мог отсеивать дубликаты
inline size_t qHash(const ErrorMessage& key, size_t seed = 0) {
    return qHash(key.type, seed) ^ qHash(key.line, seed) ^ qHash(key.column, seed);
}

// Сигнатуры всех функций
QStringList readCodeFromFile(QFile& inputFile, QSet<ErrorMessage>& errorSet);
void writeResultsToFile(QFile& outputFile, const QSet<ErrorMessage>& errorSet);
BracketCheckStatus checkBracketInStack(QStack<BracketPosition>& stack, QChar ch, BracketPosition& poppedPos);
void processLine(const QString& line, int lineIndex, ParserState& state, QStack<BracketPosition>& stack, QSet<ErrorMessage>& errors);
void performLexicalAnalysis(const QStringList& codeLines, QSet<ErrorMessage>& errorSet);

#endif // ANALYZER_H
