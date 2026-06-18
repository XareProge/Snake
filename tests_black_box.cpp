// ═══════════════════════════════════════════════════════════════════
// Тесты методом «чёрного ящика»
// ═══════════════════════════════════════════════════════════════════
#include "doctest.h"
#include "Game.h"
#include "AI.h"
#include "Records.h"
#include <filesystem>
#include <fstream>
#include <iostream>

static int countCells(const Game& g, Cell type) {
    int n = 0;
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            if (g.grid[y][x] == type) n++;
    return n;
}

// ════════════════════════════════════════════════════════════════════
// 1. Начальное состояние игры
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-01: начальный счёт равен нулю") {
    std::cout << "  [BB-01] Создаём новую игру (HARMLESS).\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: score == 0\n";
    CHECK(g.score == 0);
}

TEST_CASE("BB-02: при создании змейка жива") {
    std::cout << "  [BB-02] Создаём новую игру.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: alive == true\n";
    CHECK(g.alive == true);
}

TEST_CASE("BB-03: начальная длина змейки равна 3") {
    std::cout << "  [BB-03] Создаём новую игру.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: snake.size() == 3\n";
    CHECK(g.snake.size() == 3);
}

TEST_CASE("BB-04: на поле есть еда после создания") {
    std::cout << "  [BB-04] Создаём новую игру.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: количество клеток FOOD == 1\n";
    CHECK(countCells(g, Cell::FOOD) == 1);
}

TEST_CASE("BB-05: на Безобидном уровне нет стен") {
    std::cout << "  [BB-05] Сложность HARMLESS.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: количество клеток WALL == 0\n";
    CHECK(countCells(g, Cell::WALL) == 0);
}

TEST_CASE("BB-06: на Среднем уровне есть яд") {
    std::cout << "  [BB-06] Сложность MEDIUM.\n";
    Game g(Diff::MEDIUM);
    std::cout << "    CHECK: количество клеток POISON == 1\n";
    CHECK(countCells(g, Cell::POISON) == 1);
}

TEST_CASE("BB-07: на Сложном уровне есть телепорт и инверсия") {
    std::cout << "  [BB-07] Сложность HARD.\n";
    Game g(Diff::HARD);
    std::cout << "    CHECK: количество клеток TELEPORT == 1\n";
    CHECK(countCells(g, Cell::TELEPORT) == 1);
    std::cout << "    CHECK: количество клеток INVERT == 1\n";
    CHECK(countCells(g, Cell::INVERT) == 1);
}

// ════════════════════════════════════════════════════════════════════
// 2. Движение
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-08: змейка двигается вправо по умолчанию") {
    std::cout << "  [BB-08] Убираем еду, вызываем step().\n";
    Game g(Diff::HARMLESS);
    Pt headBefore = g.snake.front();
    for (auto& row : g.grid) for (auto& c : row) if (c == Cell::FOOD) c = Cell::EMPTY;
    g.step();
    Pt headAfter = g.snake.front();
    std::cout << "    CHECK: голова сместилась на +1 по x (headAfter.x == headBefore.x + 1)\n";
    CHECK(headAfter.x == headBefore.x + 1);
    std::cout << "    CHECK: y головы не изменился (headAfter.y == headBefore.y)\n";
    CHECK(headAfter.y == headBefore.y);
}

TEST_CASE("BB-09: длина змейки без еды не растёт") {
    std::cout << "  [BB-09] Убираем еду, делаем шаг.\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) if (c == Cell::FOOD) c = Cell::EMPTY;
    size_t lenBefore = g.snake.size();
    g.step();
    std::cout << "    CHECK: snake.size() не изменился\n";
    CHECK(g.snake.size() == lenBefore);
}

// ════════════════════════════════════════════════════════════════════
// 3. Еда
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-10: поедание еды даёт +10 очков") {
    std::cout << "  [BB-10] Кладём FOOD прямо перед головой, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::FOOD;
    g.step();
    std::cout << "    CHECK: score == 10\n";
    CHECK(g.score == 10);
}

TEST_CASE("BB-11: поедание еды удлиняет змейку") {
    std::cout << "  [BB-11] Кладём FOOD перед головой, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::FOOD;
    size_t lenBefore = g.snake.size();
    g.step();
    std::cout << "    CHECK: snake.size() == lenBefore + 1\n";
    CHECK(g.snake.size() == lenBefore + 1);
}

TEST_CASE("BB-12: после поедания еды новая еда появляется на поле") {
    std::cout << "  [BB-12] Съедаем единственную FOOD.\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::FOOD;
    g.step();
    std::cout << "    CHECK: количество клеток FOOD == 1 (спавн новой)\n";
    CHECK(countCells(g, Cell::FOOD) == 1);
}

TEST_CASE("BB-35: счёт накапливается от нескольких порций еды") {
    std::cout << "  [BB-35] Змейка съедает 2 еды подряд.\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.dir = g.nextDir = Dir::RIGHT;
    g.grid[g.snake.front().y][g.snake.front().x + 1] = Cell::FOOD;
    g.step();
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.grid[g.snake.front().y][g.snake.front().x + 1] = Cell::FOOD;
    g.step();
    std::cout << "    CHECK: score == 20 (два раза по +10)\n";
    CHECK(g.score == 20);
}

// ════════════════════════════════════════════════════════════════════
// 4. Яд
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-13: поедание яда уменьшает счёт на 15") {
    std::cout << "  [BB-13] score=20, кладём POISON перед головой, вызываем step().\n";
    Game g(Diff::MEDIUM);
    g.score = 20;
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::POISON;
    g.step();
    std::cout << "    CHECK: score == 5 (20 - 15)\n";
    CHECK(g.score == 5);
}

TEST_CASE("BB-14: поедание яда при достаточной длине укорачивает змейку") {
    std::cout << "  [BB-14] Удлиняем змейку до 5, кладём POISON перед головой.\n";
    Game g(Diff::MEDIUM);
    for (int i = 0; i < 2; i++) g.snake.push_back(g.snake.back());
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::POISON;
    size_t lenBefore = g.snake.size();
    g.step();
    std::cout << "    CHECK: snake.size() == lenBefore - 1\n";
    CHECK(g.snake.size() == lenBefore - 1);
}

TEST_CASE("BB-15: счёт не уходит ниже нуля от яда") {
    std::cout << "  [BB-15] score=0, кладём POISON перед головой.\n";
    Game g(Diff::MEDIUM);
    g.score = 0;
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::POISON;
    g.step();
    std::cout << "    CHECK: score >= 0 (не уходит в отрицательные значения)\n";
    CHECK(g.score >= 0);
}

TEST_CASE("BB-36: несколько поеданий яда снижает счёт и длину каждый раз") {
    std::cout << "  [BB-36] Змейка длиной 7, score=50, поедает яд дважды подряд.\n";
    Game g(Diff::MEDIUM);
    g.score = 50;
    for (int i = 0; i < 4; i++) g.snake.push_back(g.snake.back());
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.dir = g.nextDir = Dir::RIGHT;
    g.grid[g.snake.front().y][g.snake.front().x + 1] = Cell::POISON;
    g.step();
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.grid[g.snake.front().y][g.snake.front().x + 1] = Cell::POISON;
    g.step();
    std::cout << "    CHECK: score == 20 (50 - 15 - 15)\n";
    CHECK(g.score == 20);
    std::cout << "    CHECK: snake.size() == 5 (7 - 1 - 1)\n";
    CHECK(g.snake.size() == 5);
}

// ════════════════════════════════════════════════════════════════════
// 5. Смерть
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-16: столкновение с границей убивает змейку") {
    std::cout << "  [BB-16] Голова у правой стены, направление RIGHT, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{COLS - 1, 5}, {COLS - 2, 5}, {COLS - 3, 5}};
    g.dir = g.nextDir = Dir::RIGHT;
    g.step();
    std::cout << "    CHECK: alive == false\n";
    CHECK(g.alive == false);
}

TEST_CASE("BB-17: столкновение со стеной убивает змейку") {
    std::cout << "  [BB-17] Клетка WALL прямо перед головой, вызываем step().\n";
    Game g(Diff::EASY);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5, 5}, {4, 5}, {3, 5}};
    g.dir = g.nextDir = Dir::RIGHT;
    g.grid[5][6] = Cell::WALL;
    g.step();
    std::cout << "    CHECK: alive == false\n";
    CHECK(g.alive == false);
}

TEST_CASE("BB-18: столкновение с собой убивает змейку") {
    std::cout << "  [BB-18] Голова движется в клетку, занятую телом.\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5,5},{5,6},{6,6},{6,5},{6,4},{5,4},{4,4},{4,5},{4,6},{4,7}};
    g.dir = g.nextDir = Dir::UP;
    g.step();
    std::cout << "    CHECK: alive == false\n";
    CHECK(g.alive == false);
}

// ════════════════════════════════════════════════════════════════════
// 6. Управление — стрелки
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-19: стрелка Up меняет направление на UP") {
    std::cout << "  [BB-19] dir=RIGHT, нажимаем стрелку Up.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.handleKey(sf::Keyboard::Up);
    std::cout << "    CHECK: nextDir == UP\n";
    CHECK(g.nextDir == Dir::UP);
}

TEST_CASE("BB-20: разворот назад невозможен (RIGHT → LEFT)") {
    std::cout << "  [BB-20] dir=RIGHT, нажимаем стрелку Left, вызываем step().\n";
    Game g(Diff::HARMLESS);
    g.handleKey(sf::Keyboard::Left);
    g.step();
    std::cout << "    CHECK: dir == RIGHT (разворот не произошёл)\n";
    CHECK(g.dir == Dir::RIGHT);
}

// ════════════════════════════════════════════════════════════════════
// 7. Управление — клавиши WASD без инверсии
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-31: клавиша W без инверсии → UP") {
    std::cout << "  [BB-31] dir=RIGHT, нажимаем W.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.handleKey(sf::Keyboard::W);
    std::cout << "    CHECK: nextDir == UP\n";
    CHECK(g.nextDir == Dir::UP);
}

TEST_CASE("BB-32: клавиша S без инверсии → DOWN") {
    std::cout << "  [BB-32] dir=RIGHT, нажимаем S.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.handleKey(sf::Keyboard::S);
    std::cout << "    CHECK: nextDir == DOWN\n";
    CHECK(g.nextDir == Dir::DOWN);
}

TEST_CASE("BB-33: клавиша A без инверсии → LEFT") {
    std::cout << "  [BB-33] dir=DOWN, нажимаем A.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::DOWN;
    g.handleKey(sf::Keyboard::A);
    std::cout << "    CHECK: nextDir == LEFT\n";
    CHECK(g.nextDir == Dir::LEFT);
}

TEST_CASE("BB-34: клавиша D без инверсии → RIGHT") {
    std::cout << "  [BB-34] dir=DOWN, нажимаем D.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::DOWN;
    g.handleKey(sf::Keyboard::D);
    std::cout << "    CHECK: nextDir == RIGHT\n";
    CHECK(g.nextDir == Dir::RIGHT);
}

TEST_CASE("BB-39: стрелка Down меняет направление на DOWN") {
    std::cout << "  [BB-39] dir=RIGHT, нажимаем стрелку Down.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.handleKey(sf::Keyboard::Down);
    std::cout << "    CHECK: nextDir == DOWN\n";
    CHECK(g.nextDir == Dir::DOWN);
}

TEST_CASE("BB-40: стрелка Right меняет направление на RIGHT") {
    std::cout << "  [BB-40] dir=DOWN, нажимаем стрелку Right.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::DOWN;
    g.handleKey(sf::Keyboard::Right);
    std::cout << "    CHECK: nextDir == RIGHT\n";
    CHECK(g.nextDir == Dir::RIGHT);
}

// ════════════════════════════════════════════════════════════════════
// 8. Движение в разных направлениях
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-41: змейка двигается вверх при dir=UP") {
    std::cout << "  [BB-41] Ставим змейку вертикально, dir=UP, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5, 5}, {5, 6}, {5, 7}};
    g.dir = g.nextDir = Dir::UP;
    Pt headBefore = g.snake.front();
    g.step();
    std::cout << "    CHECK: голова сместилась на -1 по y\n";
    CHECK(g.snake.front().y == headBefore.y - 1);
    std::cout << "    CHECK: x головы не изменился\n";
    CHECK(g.snake.front().x == headBefore.x);
}

TEST_CASE("BB-42: змейка двигается вниз при dir=DOWN") {
    std::cout << "  [BB-42] Ставим змейку вертикально, dir=DOWN, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5, 5}, {5, 4}, {5, 3}};
    g.dir = g.nextDir = Dir::DOWN;
    Pt headBefore = g.snake.front();
    g.step();
    std::cout << "    CHECK: голова сместилась на +1 по y\n";
    CHECK(g.snake.front().y == headBefore.y + 1);
    std::cout << "    CHECK: x головы не изменился\n";
    CHECK(g.snake.front().x == headBefore.x);
}

TEST_CASE("BB-43: змейка двигается влево при dir=LEFT") {
    std::cout << "  [BB-43] Ставим змейку горизонтально, dir=LEFT, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{10, 5}, {11, 5}, {12, 5}};
    g.dir = g.nextDir = Dir::LEFT;
    Pt headBefore = g.snake.front();
    g.step();
    std::cout << "    CHECK: голова сместилась на -1 по x\n";
    CHECK(g.snake.front().x == headBefore.x - 1);
    std::cout << "    CHECK: y головы не изменился\n";
    CHECK(g.snake.front().y == headBefore.y);
}

// ════════════════════════════════════════════════════════════════════
// 9. Начальное состояние — направление
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-44: начальное направление движения == RIGHT") {
    std::cout << "  [BB-44] Создаём новую игру.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: dir == RIGHT\n";
    CHECK(g.dir == Dir::RIGHT);
}

// ════════════════════════════════════════════════════════════════════
// 10. Сохранение и загрузка
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-21: сохранение создаёт файл") {
    std::cout << "  [BB-21] Вызываем saveToFile().\n";
    std::filesystem::create_directories("saves");
    Game g(Diff::EASY);
    bool ok = g.saveToFile("saves/test_slot_bb.txt");
    std::cout << "    CHECK: возвращаемое значение == true\n";
    CHECK(ok == true);
    std::cout << "    CHECK: файл существует на диске\n";
    CHECK(std::filesystem::exists("saves/test_slot_bb.txt"));
    std::filesystem::remove("saves/test_slot_bb.txt");
}

TEST_CASE("BB-22: загрузка восстанавливает счёт и сложность") {
    std::cout << "  [BB-22] Сохраняем игру (MEDIUM, score=50), загружаем в новый объект.\n";
    std::filesystem::create_directories("saves");
    Game g1(Diff::MEDIUM);
    g1.score = 50;
    g1.saveToFile("saves/test_slot_bb2.txt");
    Game g2(Diff::HARMLESS);
    bool ok = g2.loadFromFile("saves/test_slot_bb2.txt");
    std::cout << "    CHECK: loadFromFile вернул true\n";
    CHECK(ok == true);
    std::cout << "    CHECK: score == 50\n";
    CHECK(g2.score == 50);
    std::cout << "    CHECK: diff == MEDIUM\n";
    CHECK(g2.diff == Diff::MEDIUM);
    std::filesystem::remove("saves/test_slot_bb2.txt");
}

TEST_CASE("BB-23: загрузка восстанавливает тело змейки") {
    std::cout << "  [BB-23] Сохраняем и загружаем — сравниваем положение и длину змейки.\n";
    std::filesystem::create_directories("saves");
    Game g1(Diff::HARMLESS);
    g1.saveToFile("saves/test_slot_bb3.txt");
    Game g2(Diff::EASY);
    g2.loadFromFile("saves/test_slot_bb3.txt");
    std::cout << "    CHECK: голова змейки совпадает с сохранённой\n";
    CHECK(g2.snake.front() == g1.snake.front());
    std::cout << "    CHECK: длина змейки совпадает с сохранённой\n";
    CHECK(g2.snake.size() == g1.snake.size());
    std::filesystem::remove("saves/test_slot_bb3.txt");
}

TEST_CASE("BB-24: загрузка несуществующего файла возвращает false") {
    std::cout << "  [BB-24] loadFromFile() на несуществующий путь.\n";
    Game g(Diff::HARMLESS);
    bool ok = g.loadFromFile("saves/nonexistent_bb.txt");
    std::cout << "    CHECK: возвращаемое значение == false\n";
    CHECK(ok == false);
}

TEST_CASE("BB-25: загрузка повреждённого файла возвращает false") {
    std::cout << "  [BB-25] Файл содержит мусор вместо данных.\n";
    std::filesystem::create_directories("saves");
    { std::ofstream f("saves/corrupt_bb.txt"); f << "diff:GARBAGE\nscore:not_a_number\n"; }
    Game g(Diff::HARMLESS);
    bool ok = g.loadFromFile("saves/corrupt_bb.txt");
    std::cout << "    CHECK: возвращаемое значение == false\n";
    CHECK(ok == false);
    std::filesystem::remove("saves/corrupt_bb.txt");
}

// ════════════════════════════════════════════════════════════════════
// 9. Рекорды
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-26: сохранённый рекорд можно загрузить") {
    std::cout << "  [BB-26] Сохраняем рекорд score=999, загружаем список.\n";
    std::filesystem::create_directories("saves");
    std::filesystem::remove("saves/records.txt");
    Record r{"01.01.2025 12:00", "Лёгкий", 999, false};
    saveRecord(r);
    auto recs = loadRecords();
    std::cout << "    REQUIRE: список не пуст\n";
    REQUIRE(recs.size() >= 1);
    std::cout << "    CHECK: recs[0].score == 999\n";
    CHECK(recs[0].score == 999);
    std::cout << "    CHECK: recs[0].diff == \"Лёгкий\"\n";
    CHECK(recs[0].diff == "Лёгкий");
    std::filesystem::remove("saves/records.txt");
}

TEST_CASE("BB-27: рекорды сортируются по убыванию счёта") {
    std::cout << "  [BB-27] Сохраняем 3 рекорда в произвольном порядке.\n";
    std::filesystem::create_directories("saves");
    std::filesystem::remove("saves/records.txt");
    saveRecord({"d", "Лёгкий",  100, false});
    saveRecord({"d", "Средний", 300, false});
    saveRecord({"d", "Сложный", 200, false});
    auto recs = loadRecords();
    std::cout << "    REQUIRE: список содержит 3 записи\n";
    REQUIRE(recs.size() == 3);
    std::cout << "    CHECK: recs[0].score >= recs[1].score\n";
    CHECK(recs[0].score >= recs[1].score);
    std::cout << "    CHECK: recs[1].score >= recs[2].score\n";
    CHECK(recs[1].score >= recs[2].score);
    std::filesystem::remove("saves/records.txt");
}

TEST_CASE("BB-28: рекордов хранится не более 10") {
    std::cout << "  [BB-28] Сохраняем 15 рекордов.\n";
    std::filesystem::create_directories("saves");
    std::filesystem::remove("saves/records.txt");
    for (int i = 0; i < 15; i++)
        saveRecord({"d", "Лёгкий", i * 10, false});
    auto recs = loadRecords();
    std::cout << "    CHECK: recs.size() <= 10\n";
    CHECK(recs.size() <= 10);
    std::filesystem::remove("saves/records.txt");
}

TEST_CASE("BB-45: загрузка восстанавливает направление движения") {
    std::cout << "  [BB-45] Сохраняем игру с dir=RIGHT, загружаем, делаем step().\n";
    std::filesystem::create_directories("saves");
    Game g1(Diff::HARMLESS);
    for (auto& row : g1.grid) for (auto& c : row) c = Cell::EMPTY;
    g1.snake = {{5, 11}, {4, 11}, {3, 11}};
    g1.dir = g1.nextDir = Dir::RIGHT;
    g1.saveToFile("saves/test_dir_bb.txt");

    Game g2(Diff::EASY);
    g2.loadFromFile("saves/test_dir_bb.txt");
    for (auto& row : g2.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt headBefore = g2.snake.front();
    g2.step();
    std::cout << "    CHECK: alive == true (шаг прошёл без аварии)\n";
    CHECK(g2.alive == true);
    std::cout << "    CHECK: голова сместилась (змейка движется)\n";
    CHECK(!(g2.snake.front() == headBefore));
    std::filesystem::remove("saves/test_dir_bb.txt");
}

TEST_CASE("BB-37: loadRecords при отсутствующем файле возвращает пустой список") {
    std::cout << "  [BB-37] Файл records.txt удалён перед вызовом loadRecords().\n";
    std::error_code ec;
    std::filesystem::remove("saves/records.txt", ec);
    auto recs = loadRecords();
    std::cout << "    CHECK: список пуст\n";
    CHECK(recs.empty() == true);
}

TEST_CASE("BB-46: флаг isAI=false сохраняется и загружается из рекорда") {
    std::cout << "  [BB-46] Сохраняем рекорд с isAI=false, загружаем.\n";
    std::filesystem::create_directories("saves");
    std::filesystem::remove("saves/records.txt");
    saveRecord({"01.01.2025", "Лёгкий", 50, false});
    auto recs = loadRecords();
    std::cout << "    REQUIRE: список содержит 1 запись\n";
    REQUIRE(recs.size() == 1);
    std::cout << "    CHECK: recs[0].isAI == false\n";
    CHECK(recs[0].isAI == false);
    std::filesystem::remove("saves/records.txt");
}

TEST_CASE("BB-38: флаг isAI сохраняется и загружается из рекорда") {
    std::cout << "  [BB-38] Сохраняем рекорд с isAI=true, загружаем.\n";
    std::filesystem::create_directories("saves");
    std::filesystem::remove("saves/records.txt");
    saveRecord({"01.01.2025", "Сложный", 100, true});
    auto recs = loadRecords();
    std::cout << "    REQUIRE: список содержит 1 запись\n";
    REQUIRE(recs.size() == 1);
    std::cout << "    CHECK: recs[0].isAI == true\n";
    CHECK(recs[0].isAI == true);
    std::filesystem::remove("saves/records.txt");
}

// ════════════════════════════════════════════════════════════════════
// 10. ИИ
// ════════════════════════════════════════════════════════════════════
TEST_CASE("BB-29: computeAI возвращает допустимое направление") {
    std::cout << "  [BB-29] Вызываем computeAI() на стандартном поле.\n";
    Game g(Diff::HARMLESS);
    AIResult res = computeAI(g);
    bool valid = (res.direction == Dir::UP   || res.direction == Dir::DOWN ||
                  res.direction == Dir::LEFT || res.direction == Dir::RIGHT);
    std::cout << "    CHECK: результат — одно из четырёх направлений\n";
    CHECK(valid == true);
}

TEST_CASE("BB-30: в режиме ИИ клавиатура не меняет направление") {
    std::cout << "  [BB-30] aiMode=true, нажимаем Up.\n";
    Game g(Diff::HARMLESS, true);
    Dir before = g.nextDir;
    g.handleKey(sf::Keyboard::Up);
    std::cout << "    CHECK: nextDir не изменился\n";
    CHECK(g.nextDir == before);
}
