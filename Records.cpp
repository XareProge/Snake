#include "Records.h"
#include <fstream>
#include <sstream>
#include <algorithm>

static constexpr int MAX_RECORDS = 10;

std::vector<Record> loadRecords() {
    std::vector<Record> recs;
    std::ifstream file("saves/records.txt");
    std::string line;

    // Формат строки: дата|сложность|счёт|аи (4-е поле необязательно — совместимость с v3.0)
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        Record r;
        std::getline(ss, r.date,  '|');
        std::getline(ss, r.diff,  '|');
        std::string scoreStr;
        std::getline(ss, scoreStr, '|');
        r.score = scoreStr.empty() ? 0 : std::stoi(scoreStr);
        std::string aiStr;
        if (std::getline(ss, aiStr, '|'))
            r.isAI = (aiStr == "1");
        recs.push_back(r);
    }

    std::sort(recs.begin(), recs.end(), [](const Record& a, const Record& b) {
        return a.score > b.score;
    });
    if ((int)recs.size() > MAX_RECORDS)
        recs.resize(MAX_RECORDS);
    return recs;
}

void saveRecord(const Record& r) {
    // Загружаем, добавляем, сортируем, обрезаем до MAX_RECORDS, перезаписываем файл
    auto recs = loadRecords();
    recs.push_back(r);
    std::sort(recs.begin(), recs.end(), [](const Record& a, const Record& b) {
        return a.score > b.score;
    });
    if ((int)recs.size() > MAX_RECORDS)
        recs.resize(MAX_RECORDS);

    std::ofstream file("saves/records.txt");
    for (const auto& rec : recs)
        file << rec.date << "|" << rec.diff << "|" << rec.score << "|" << (rec.isAI ? "1" : "0") << "\n";
}
