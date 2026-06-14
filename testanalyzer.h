#ifndef TESTANALYZER_H
#define TESTANALYZER_H

#include <QtTest>
#include "analyzer.h"

class TestAnalyzer : public QObject {
    Q_OBJECT
private slots:
    // 1. Тесты низкоуровневой работы со стеком
    void testCheckBracketInStack_Match();
    void testCheckBracketInStack_Mismatch();
    void testCheckBracketInStack_Empty();

    // 2. Тесты автомата состояний (лексического анализатора)
    void testPerformLexicalAnalysis_ValidCode();
    void testPerformLexicalAnalysis_UnmatchedOpen();
    void testPerformLexicalAnalysis_UnmatchedClose();
    void testPerformLexicalAnalysis_MismatchTypes();

    // 3. Тесты игнорирования (строки, символы, комментарии, include)
    void testPerformLexicalAnalysis_IgnoreStringsAndChars();
    void testPerformLexicalAnalysis_IgnoreComments();
    void testPerformLexicalAnalysis_IncludeDirectives();

    // 4. Тест прерывания по макросу
    void testPerformLexicalAnalysis_MacroFound();
};

#endif // TESTANALYZER_H
