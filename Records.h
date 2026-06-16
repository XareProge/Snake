#pragma once
#include <vector>
#include "Types.h"

// Загружает рекорды из records.txt, сортирует по убыванию счёта
std::vector<Record> loadRecords();

// Дописывает одну запись в конец records.txt
void saveRecord(const Record& r);
