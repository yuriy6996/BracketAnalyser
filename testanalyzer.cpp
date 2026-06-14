#include "testanalyzer.h"

// --- 1. Тесты работы со стеком ---

void TestAnalyzer::testCheckBracketInStack_Match() {
    QStack<BracketPosition> stack;
    stack.push({Round, '(', 0, 5});
    BracketPosition popped;
    BracketCheckStatus status = checkBracketInStack(stack, ')', popped);

    QCOMPARE(status, Match);
    QCOMPARE(popped.character, QChar('('));
}

void TestAnalyzer::testCheckBracketInStack_Mismatch() {
    QStack<BracketPosition> stack;
    stack.push({Square, '[', 0, 2});
    BracketPosition popped;
    BracketCheckStatus status = checkBracketInStack(stack, ')', popped);

    QCOMPARE(status, Mismatch);
    QCOMPARE(popped.type, Square);
}

void TestAnalyzer::testCheckBracketInStack_Empty() {
    QStack<BracketPosition> stack;
    BracketPosition popped;
    BracketCheckStatus status = checkBracketInStack(stack, '}', popped);

    QCOMPARE(status, StackEmpty);
}

// --- 2. Основные тесты лексического анализа ---

void TestAnalyzer::testPerformLexicalAnalysis_ValidCode() {
    QStringList code = {
        "int main() {",
        "    int arr[10];",
        "    return 0;",
        "}"
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    QVERIFY(errors.isEmpty()); // Ошибок быть не должно
}

void TestAnalyzer::testPerformLexicalAnalysis_UnmatchedOpen() {
    QStringList code = {
        "int main() {" // Нет закрывающей '}'
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.contains(ErrorMessage(UnmatchedOpenBracket, 0, 11)));
}

void TestAnalyzer::testPerformLexicalAnalysis_UnmatchedClose() {
    QStringList code = {
        "int main() }", // Лишняя закрывающая '}'
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.contains(ErrorMessage(UnmatchedCloseBracket, 0, 11)));
}

void TestAnalyzer::testPerformLexicalAnalysis_MismatchTypes() {
    QStringList code = {
        "int main() {",
        "    int a = (5];", // Ожидалась ')', а пришла ']'
        "}"
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    // По нашему алгоритму генерируются 2 ошибки: для открывающей и для закрывающей
    QCOMPARE(errors.size(), 2);
    QVERIFY(errors.contains(ErrorMessage(UnmatchedCloseBracket, 1, 14)));
    QVERIFY(errors.contains(ErrorMessage(UnmatchedOpenBracket, 1, 12)));
}

// --- 3. Тесты состояний пропуска (строки, комментарии) ---

void TestAnalyzer::testPerformLexicalAnalysis_IgnoreStringsAndChars() {
    QStringList code = {
        "int main() {",
        "    const char* s = \"{\";", // Игнорируем скобку в строке
        "    char c = '}';",         // Игнорируем скобку в char
        "    const char* esc = \"\\\"\";", // Экранированная кавычка
        "}"
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    QVERIFY(errors.isEmpty());
}

void TestAnalyzer::testPerformLexicalAnalysis_IgnoreComments() {
    QStringList code = {
        "int main() {",
        "    // ( этот комментарий ломает парсер, если его не игнорировать",
        "    /* и этот многострочный",
        "       тоже ] */",
        "}"
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    QVERIFY(errors.isEmpty());
}

void TestAnalyzer::testPerformLexicalAnalysis_IncludeDirectives() {
    QStringList code = {
        "#include <iostream> // Угловые скобки не должны считаться",
        "#include \"my_lib.h\"",
        "int main() {}"
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    QVERIFY(errors.isEmpty());
}

// --- 4. Тест макросов ---

void TestAnalyzer::testPerformLexicalAnalysis_MacroFound() {
    QStringList code = {
        "int main() {",
        "#define CONST_VAL 10", // Строка индекса 1
        "    int x = (5;", // Незакрытая скобка, но парсер уже прервется
        "}"
    };
    QSet<ErrorMessage> errors;
    performLexicalAnalysis(code, errors);

    // Должна быть только ошибка макроса, так как парсинг остановился
    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.contains(ErrorMessage(MacroFound, 1, 0)));
}
