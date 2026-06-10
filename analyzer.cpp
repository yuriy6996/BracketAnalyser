#include "analyzer.h"

QStringList readCodeFromFile(QFile& inputFile, QSet<ErrorMessage>& errorSet) {
    QStringList codeLines;
    QTextStream stream(&inputFile);
    int currentLine = 0; // Нумерация строк с 0

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (currentLine >= 10000) {
            errorSet.insert(ErrorMessage(LineLimitExceeded, currentLine, 0));
            break;
        }

        if (line.length() > 1024) {
            errorSet.insert(ErrorMessage(CharLimitExceeded, currentLine, 0));
            break;
        }

        codeLines.append(line);
        currentLine++;
    }

    return codeLines;
}

void writeResultsToFile(QFile& outputFile, const QSet<ErrorMessage>& errorSet) {
    QTextStream out(&outputFile);

    if (errorSet.isEmpty()) {
        out << "Ошибок не обнаружено\n";
        return;
    }

    QString outputBuffer = "Обнаружены ошибки:\n";

    for (const ErrorMessage& error : errorSet) {
        outputBuffer += error.generateText() + "\n";
    }

    out << outputBuffer;
}

BracketCheckStatus checkBracketInStack(QStack<BracketPosition>& stack, QChar ch, BracketPosition& poppedPos) {
    if (stack.isEmpty()) return StackEmpty;

    poppedPos = stack.pop();

    if ((ch == ')' && poppedPos.type == Round) ||
        (ch == ']' && poppedPos.type == Square) ||
        (ch == '}' && poppedPos.type == Curly)) {
        return Match;
    }

    return Mismatch;
}


void processLine(const QString& line, int lineIndex, ParserState& state, QStack<BracketPosition>& stack, QSet<ErrorMessage>& errors) {
    // Однострочный комментарий и include-режимы сбрасываются в начале каждой новой строки
    if (state == InSingleLineComment || state == InIncludeAngleBrackets || state == InIncludeQuotes) {
        state = Normal;
    }

    // Быстрая проверка: является ли строка директивой include
    QString trimmed = line.trimmed();
    bool isIncludeLine = trimmed.startsWith("#include");
    bool escaped = false;

    for (int col = 0; col < line.length(); ++col) {
        QChar ch = line[col];

        // 1. Обработка состояний пропуска (комментарии, строки, литералы)
        if (state == InSingleLineComment) {
            break; // Остаток строки полностью игнорируется
        }

        if (state == InMultiLineComment) {
            if (ch == '/' && col > 0 && line[col - 1] == '*') {
                state = Normal;
            }
            continue;
        }

        if (state == InString) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                continue;
            }
            if (ch == '"') {
                state = Normal;
            }
            continue;
        }

        if (state == InChar) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                continue;
            }
            if (ch == '\'') {
                state = Normal;
            }
            continue;
        }

        if (state == InIncludeAngleBrackets) {
            if (ch == '>') state = Normal;
            continue;
        }

        if (state == InIncludeQuotes) {
            if (ch == '"') state = Normal;
            continue;
        }

        // 2. Базовое состояние (Normal) — поиск управляющих символов
        if (ch == '/' && col < line.length() - 1 && line[col + 1] == '/') {
            state = InSingleLineComment;
            break;
        }
        if (ch == '/' && col < line.length() - 1 && line[col + 1] == '*') {
            state = InMultiLineComment;
            col++; // Пропускаем звездочку, чтобы не анализировать её на следующем шаге
            continue;
        }

        if (ch == '"') {
            state = isIncludeLine ? InIncludeQuotes : InString;
            continue;
        }

        if (ch == '\'') {
            state = InChar;
            continue;
        }

        if (isIncludeLine && ch == '<') {
            state = InIncludeAngleBrackets;
            continue;
        }

        // 3. Анализ скобок
        if (ch == '(' || ch == '[' || ch == '{') {
            BracketType type = Unknown;
            if (ch == '(') type = Round;
            else if (ch == '[') type = Square;
            else if (ch == '{') type = Curly;

            stack.push({type, ch, lineIndex, col});
            continue;
        }

        if (ch == ')' || ch == ']' || ch == '}') {
            BracketPosition poppedPos;
            BracketCheckStatus status = checkBracketInStack(stack, ch, poppedPos);

            if (status == StackEmpty) {
                errors.insert(ErrorMessage(UnmatchedCloseBracket, lineIndex, col,
                    QString("(нет открывающей пары для '%1')").arg(ch)));
            }
            else if (status == Mismatch) {
                errors.insert(ErrorMessage(UnmatchedCloseBracket, lineIndex, col,
                    QString("(ожидалась пара для '%1', но встречена '%2')").arg(poppedPos.character).arg(ch)));
                errors.insert(ErrorMessage(UnmatchedOpenBracket, poppedPos.line, poppedPos.column,
                    QString("(пересекается с закрывающей '%1' на строке %2)").arg(ch).arg(lineIndex)));
            }
            continue;
        }
    }
}

void performLexicalAnalysis(const QStringList& codeLines, QSet<ErrorMessage>& errorSet) {
    // Если на этапе чтения файла уже зафиксированы критические ошибки лимитов, парсинг не запускаем
    if (errorSet.contains(ErrorMessage(LineLimitExceeded, 0, 0)) ||
        errorSet.contains(ErrorMessage(CharLimitExceeded, 0, 0))) {
        return;
    }

    QStack<BracketPosition> stack;
    ParserState state = Normal;

    for (int i = 0; i < codeLines.size(); ++i) {
        // Проверка на макрос #define
        if (codeLines[i].contains("#define")) {
            errorSet.insert(ErrorMessage(MacroFound, i, 0));
            break;
        }
        processLine(codeLines[i], i, state, stack, errorSet);
    }

    // Если файл закончился, а в стеке что-то осталось — это незакрытые открывающие скобки
    while (!stack.isEmpty()) {
        BracketPosition p = stack.pop();
        errorSet.insert(ErrorMessage(UnmatchedOpenBracket, p.line, p.column,
            QString("(скобка '%1' не была закрыта до конца файла)").arg(p.character)));
    }
}
