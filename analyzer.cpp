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
