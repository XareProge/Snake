#include "Records.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::vector<Record> loadRecords() {
    std::vector<Record> recs;
    std::ifstream file("records.txt");
    std::string line;

    // Формат строки: дата|сложность|счёт
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        Record r;
        std::getline(ss, r.date, '|');
        std::getline(ss, r.diff, '|');
        ss >> r.score;
        recs.push_back(r);
    }

    // Сортировка по убыванию счёта
    std::sort(recs.begin(), recs.end(), [](const Record& a, const Record& b) {
        return a.score > b.score;
    });
    return recs;
}

void saveRecord(const Record& r) {
    std::ofstream file("records.txt", std::ios::app);
    file << r.date << "|" << r.diff << "|" << r.score << "\n";
}
