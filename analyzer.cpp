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
    for (int col = 0; col < line.length(); ++col) {
        QChar ch = line[col];

    }
}

void performLexicalAnalysis(const QStringList& codeLines, QSet<ErrorMessage>& errorSet) {
    QStack<BracketPosition> stack;
    ParserState state = Normal;

    for (int i = 0; i < codeLines.size(); ++i) {
        // Проверка на макрос (прерывание по ТЗ)
        if (codeLines[i].contains("#define")) {
            errorSet.insert(ErrorMessage(MacroFound, i, 0));
            break;
        }
        processLine(codeLines[i], i, state, stack, errorSet);
    }

    // Очистка стека в конце файла
    while (!stack.isEmpty()) {
        BracketPosition p = stack.pop();
        errorSet.insert(ErrorMessage(UnmatchedOpenBracket, p.line, p.column));
    }
}
