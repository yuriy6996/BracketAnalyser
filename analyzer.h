#ifndef ANALYZER_H
#define ANALYZER_H

#include <QString>
#include <QStringList>
#include <QFile>
#include <QSet>
#include <QStack>
#include <QTextStream>

/**
 * @file analyzer.h
 * @brief Заголовочный файл модуля лексического анализа закрытия скобок.
 *
 * Содержит объявления структур данных, классов и функций, необходимых
 * для автоматической проверки корректности вложенности скобок в коде C++[cite: 3].
 */

/**
 * @brief Перечисление состояний автомата (парсера) при построчном разборе текста.
 */
enum ParserState {
    Normal,                  ///< Базовое состояние: поиск управляющих символов, кавычек и комментариев
    InString,                ///< Режим обработки строковой константы
    InChar,                  ///< Режим обработки символьного литерала
    InSingleLineComment,     ///< Пропуск текста внутри однострочного комментария (//)
    InMultiLineComment,      ///< Пропуск текста внутри многострочного комментария (/* ... */)
    InIncludeAngleBrackets,  ///< Пропуск скобок внутри стандартных директив #include <...>
    InIncludeQuotes          ///< Пропуск скобок внутри пользовательских директив #include "..."
};

/**
 * @brief Перечисление типов проверяемых скобок[cite: 7].
 */
enum BracketType {
    Round,   ///< Круглая скобка '(' или ')' [cite: 7]
    Square,  ///< Квадратная скобка '[' или ']' [cite: 7]
    Curly,   ///< Фигурная скобка '{' или '}' [cite: 7]
    Unknown  ///< Неизвестный тип символа [cite: 7]
};

/**
 * @brief Перечисление типов обнаруживаемых кодов ошибок.
 */
enum ErrorType {
    NoError,               ///< Ошибок в коде не обнаружено
    InvalidArgs,           ///< Неверное количество аргументов командной строки при запуске
    FileOpenError,         ///< Не удалось открыть входной файл на чтение
    FileWriteError,        ///< Не удалось открыть выходной файл на запись
    LineLimitExceeded,     ///< Превышено критическое ограничение в 10 000 строк
    CharLimitExceeded,     ///< Превышена максимальная длина строки в 1024 символа
    MacroFound,            ///< В коде обнаружен макрос #define (аварийная остановка)
    UnmatchedOpenBracket,  ///< Открывающая скобка осталась незакрытой до конца файла/блока
    UnmatchedCloseBracket  ///< Избыточная закрывающая скобка или несоответствие типов
};

/**
 * @brief Структура для фиксации позиции открывающей скобки в тексте[cite: 9].
 */
struct BracketPosition {
    BracketType type;      ///< Тип скобки (Round, Square, Curly) [cite: 9]
    QChar character;       ///< Непосредственный символ скобки [cite: 9]
    int line;              ///< Номер строки в файле (индексация строго с 0) [cite: 9]
    int column;            ///< Номер позиции символа внутри строки (с 0) [cite: 9]

    /**
     * @brief Оператор сравнения для обеспечения корректной работы структуры в контейнерах.
     */
    bool operator==(const BracketPosition& other) const {
        return line == other.line && column == other.column && character == other.character;
    }
};

/**
 * @brief Класс, инкапсулирующий информацию об обнаруженной ошибке[cite: 10].
 */
class ErrorMessage {
public:
    ErrorType type;       ///< Тип зафиксированной ошибки [cite: 10]
    int line;             ///< Номер строки, в которой локализована ошибка [cite: 10]
    int column;           ///< Номер позиции (колонка) ошибки в строке [cite: 10]
    QString details;      ///< Дополнительное текстовое описание (какой символ ожидался) [cite: 10]

    /**
     * @brief Конструктор инициализации сообщения об ошибке.
     */
    ErrorMessage(ErrorType t, int l, int c, QString d = "")
        : type(t), line(l), column(c), details(d) {}

    /**
     * @brief Формирует готовую строку сообщения об ошибке для записи в файл результатов[cite: 10].
     * @return QString Текст ошибки в читаемом формате.
     */
    QString generateText() const;

    /**
     * @brief Оператор сравнения для уникального хранения ошибок во множестве QSet.
     */
    bool operator==(const ErrorMessage& other) const {
        return type == other.type && line == other.line && column == other.column;
    }
};

/**
 * @brief Функция хэширования для работы структуры ErrorMessage внутри QSet.
 */
inline uint qHash(const ErrorMessage& key, uint seed = 0) {
    return qHash(key.line, seed) ^ qHash(key.column, seed) ^ qHash(static_cast<int>(key.type), seed);
}

// --- Описание алгоритмов и функций ---

/**
 * @brief Функция построчного чтения исходного кода из файла[cite: 26].
 * * Выполняет последовательное чтение строк и валидацию на лимиты размера проекта[cite: 4].
 * При превышении 10000 строк или длины строки в 1024 символа прерывает работу[cite: 35, 36].
 * * @param inputFile Ссылка на открытый и валидный файл для чтения[cite: 27].
 * @param errorSet Ссылка на множество ошибок, куда помещаются критические исключения лимитов[cite: 27].
 * @return QStringList Список строк считанного исходного кода[cite: 28].
 */
QStringList readCodeFromFile(QFile& inputFile, QSet<ErrorMessage>& errorSet);

/**
 * @brief Основная функция лексического анализа структуры скобок в коде[cite: 40].
 * * Управляет глобальным циклом обхода строк, отслеживает появление макросов #define [cite: 47]
 * и выполняет финальную валидацию оставшегося стека на незакрытые элементы[cite: 50].
 * * @param codeLines Массив строк исходного кода для анализа[cite: 41].
 * @param errorSet Ссылка на результирующее множество ошибок для записи результатов[cite: 41].
 */
void performLexicalAnalysis(const QStringList& codeLines, QSet<ErrorMessage>& errorSet);

/**
 * @brief Функция выгрузки результатов анализа в выходной текстовый файл[cite: 51].
 * * Если множество ошибок пусто — гарантированно выводит сообщение об успешной проверке[cite: 55].
 * В противном случае генерирует упорядоченный список ошибок[cite: 57, 58].
 * * @param outputFile Ссылка на открытый файл для записи отчета[cite: 52].
 * @param errorSet Множество обнаруженных в процессе парсинга ошибок[cite: 52].
 */
void writeResultsToFile(QFile& outputFile, const QSet<ErrorMessage>& errorSet);

/**
 * @brief Посимвольный разбор одной строки кода и управление автоматом состояний[cite: 59].
 * * Сканирует строку, переключает контексты (строки, комментарии) и координирует работу со стеком[cite: 63, 65].
 * * @param line Текст текущей анализируемой строки[cite: 60].
 * @param lineIndex Индекс строки в документе (начиная с 0)[cite: 60].
 * @param state Текущее состояние конечного автомата[cite: 60].
 * @param stack Ссылка на стек отслеживания открытых скобок[cite: 60].
 * @param errors Ссылка на хранилище ошибок для записи несоответствий[cite: 60].
 */
void processLine(const QString& line, int lineIndex, ParserState& state, QStack<BracketPosition>& stack, QSet<ErrorMessage>& errors);

/**
 * @brief Перечисление исходов сопоставления закрывающей скобки со стеком.
 */
enum BracketCheckStatus {
    Match,        ///< Скобки совпали по типам
    Mismatch,     ///< Тип закрывающей скобки отличается от верхней в стеке
    StackEmpty    ///< Стек пуст, открывающая пара отсутствует
};

/**
 * @brief Вспомогательная функция валидации закрывающей скобки на вершине стека[cite: 74].
 * * Извлекает элемент и сопоставляет его тип с типом встреченного закрывающего символа[cite: 80, 81].
 * * @param stack Стек текущих открытых скобок[cite: 75].
 * @param ch Символ проверяемой закрывающей скобки[cite: 75].
 * @param poppedPos Ссылка на структуру для возврата извлеченных координат скобки.
 * @return BracketCheckStatus Статус проверки (Match, Mismatch или StackEmpty).
 */
BracketCheckStatus checkBracketInStack(QStack<BracketPosition>& stack, QChar ch, BracketPosition& poppedPos);

#endif // ANALYZER_H
