#include <QCoreApplication>
#include <QFile>
#include <iostream>
#include "analyzer.h"
#include "testanalyzer.h"

int main(int argc, char *argv[]) {
    // Инициализация консольного приложения Qt
    QCoreApplication a(argc, argv);

    if (argc == 2 && QString(argv[1]) == "--test") {
        TestAnalyzer tc;
        return QTest::qExec(&tc);
    }


    // 1. Проверка аргументов командной строки
    if (argc != 3) {
        std::cerr << "Ошибка: неверное количество аргументов!" << std::endl;
        std::cerr << "Использование: BracketAnalyzer <входной_файл.cpp> <файл_результатов.txt>" << std::endl;
        return 1; // Завершение с кодом ошибки
    }

    QString inputFilePath = argv[1];
    QString outputFilePath = argv[2];

    // 2. Открытие файлов с проверкой на ошибки
    QFile inputFile(inputFilePath);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Ошибка: Не удалось открыть входной файл для чтения: "
                  << inputFilePath.toStdString() << std::endl;
        return 1;
    }

    QFile outputFile(outputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Ошибка: Не удалось открыть выходной файл для записи: "
                  << outputFilePath.toStdString() << std::endl;
        inputFile.close();
        return 1;
    }

    // 3. Инициализация хранилища ошибок
    QSet<ErrorMessage> errorSet;

    // 4. Запуск конвейера обработки
    QStringList codeLines = readCodeFromFile(inputFile, errorSet);
    performLexicalAnalysis(codeLines, errorSet);
    writeResultsToFile(outputFile, errorSet);

    // 5. Корректное закрытие ресурсов
    inputFile.close();
    outputFile.close();

    // std::cout << "Анализ успешно завершен. Проверьте выходной файл." << std::endl;

    return 0; // Успешное завершение
}
