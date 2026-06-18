// ═══════════════════════════════════════════════════════════════════
// Тесты методом «белого ящика»
// ═══════════════════════════════════════════════════════════════════
#include "doctest.h"
#include "Game.h"
#include "AI.h"
#include "Records.h"
#include <filesystem>
#include <fstream>
#include <iostream>

// ═══════════════════════════════════════════════════════════════════
// 1. Game::validPt — граничные условия
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-01: validPt — левая граница (x=-1)") {
    std::cout << "  [WB-01] Проверяем точку за левым краем поля.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({-1, 0}) == false\n";
    CHECK(g.validPt({-1, 0}) == false);
}

TEST_CASE("WB-02: validPt — верхняя граница (y=-1)") {
    std::cout << "  [WB-02] Проверяем точку за верхним краем поля.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({0, -1}) == false\n";
    CHECK(g.validPt({0, -1}) == false);
}

TEST_CASE("WB-03: validPt — правая граница (x=COLS)") {
    std::cout << "  [WB-03] Проверяем точку за правым краем (x == ширина поля).\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({COLS, 0}) == false\n";
    CHECK(g.validPt({COLS, 0}) == false);
}

TEST_CASE("WB-04: validPt — нижняя граница (y=ROWS)") {
    std::cout << "  [WB-04] Проверяем точку за нижним краем (y == высота поля).\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({0, ROWS}) == false\n";
    CHECK(g.validPt({0, ROWS}) == false);
}

TEST_CASE("WB-05: validPt — угол (0,0) допустим") {
    std::cout << "  [WB-05] Проверяем левый верхний угол.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({0, 0}) == true\n";
    CHECK(g.validPt({0, 0}) == true);
}

TEST_CASE("WB-06: validPt — угол (COLS-1, ROWS-1) допустим") {
    std::cout << "  [WB-06] Проверяем правый нижний угол.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({COLS-1, ROWS-1}) == true\n";
    CHECK(g.validPt({COLS - 1, ROWS - 1}) == true);
}

TEST_CASE("WB-07: validPt — центр поля допустим") {
    std::cout << "  [WB-07] Проверяем центральную клетку поля.\n";
    Game g(Diff::HARMLESS);
    std::cout << "    CHECK: validPt({COLS/2, ROWS/2}) == true\n";
    CHECK(g.validPt({COLS / 2, ROWS / 2}) == true);
}

// ═══════════════════════════════════════════════════════════════════
// 2. Game::handleKey — запрет разворота
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-08: запрет разворота RIGHT→LEFT через handleKey") {
    std::cout << "  [WB-08] dir=RIGHT, нажимаем Left.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.handleKey(sf::Keyboard::Left);
    std::cout << "    CHECK: nextDir остался RIGHT (разворот заблокирован)\n";
    CHECK(g.nextDir == Dir::RIGHT);
}

TEST_CASE("WB-09: запрет разворота UP→DOWN через handleKey") {
    std::cout << "  [WB-09] dir=UP, нажимаем Down.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::UP;
    g.handleKey(sf::Keyboard::Down);
    std::cout << "    CHECK: nextDir остался UP (разворот заблокирован)\n";
    CHECK(g.nextDir == Dir::UP);
}

TEST_CASE("WB-10: запрет разворота DOWN→UP через handleKey") {
    std::cout << "  [WB-10] dir=DOWN, нажимаем Up.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::DOWN;
    g.handleKey(sf::Keyboard::Up);
    std::cout << "    CHECK: nextDir остался DOWN (разворот заблокирован)\n";
    CHECK(g.nextDir == Dir::DOWN);
}

TEST_CASE("WB-11: запрет разворота LEFT→RIGHT через handleKey") {
    std::cout << "  [WB-11] dir=LEFT, нажимаем Right.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::LEFT;
    g.handleKey(sf::Keyboard::Right);
    std::cout << "    CHECK: nextDir остался LEFT (разворот заблокирован)\n";
    CHECK(g.nextDir == Dir::LEFT);
}

TEST_CASE("WB-35: разворот DOWN→UP не происходит после step()") {
    std::cout << "  [WB-35] dir=DOWN, пытаемся нажать Up (разворот), вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5, 5}, {5, 4}, {5, 3}};
    g.dir = g.nextDir = Dir::DOWN;
    g.handleKey(sf::Keyboard::Up);
    g.step();
    std::cout << "    CHECK: dir == DOWN (разворот не случился)\n";
    CHECK(g.dir == Dir::DOWN);
    std::cout << "    CHECK: alive == true (змейка не врезалась в себя)\n";
    CHECK(g.alive == true);
}

TEST_CASE("WB-36: разворот LEFT→RIGHT не происходит после step()") {
    std::cout << "  [WB-36] dir=LEFT, пытаемся нажать Right (разворот), вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{10, 5}, {11, 5}, {12, 5}};
    g.dir = g.nextDir = Dir::LEFT;
    g.handleKey(sf::Keyboard::Right);
    g.step();
    std::cout << "    CHECK: dir == LEFT (разворот не случился)\n";
    CHECK(g.dir == Dir::LEFT);
    std::cout << "    CHECK: alive == true (змейка не врезалась в себя)\n";
    CHECK(g.alive == true);
}

TEST_CASE("WB-12: перпендикулярный поворот RIGHT→UP разрешён") {
    std::cout << "  [WB-12] dir=RIGHT, нажимаем Up (перпендикулярный поворот).\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.handleKey(sf::Keyboard::Up);
    std::cout << "    CHECK: nextDir == UP\n";
    CHECK(g.nextDir == Dir::UP);
}

// ═══════════════════════════════════════════════════════════════════
// 3. Game::tickInterval — скорость и сложность
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-13: HARMLESS медленнее HARD при нулевом счёте") {
    std::cout << "  [WB-13] Сравниваем базовые интервалы HARMLESS и HARD.\n";
    Game h(Diff::HARMLESS);
    Game d(Diff::HARD);
    std::cout << "    CHECK: tickInterval(HARMLESS) > tickInterval(HARD)\n";
    CHECK(h.tickInterval() > d.tickInterval());
}

TEST_CASE("WB-14: скорость растёт со счётом (интервал убывает)") {
    std::cout << "  [WB-14] Сравниваем интервал при score=0 и score=200.\n";
    Game g(Diff::EASY);
    g.score = 0;   float t0   = g.tickInterval();
    g.score = 200; float t200 = g.tickInterval();
    std::cout << "    CHECK: tickInterval(score=200) < tickInterval(score=0)\n";
    CHECK(t200 < t0);
}

TEST_CASE("WB-15: tickInterval не уходит ниже минимума 0.05f") {
    std::cout << "  [WB-15] Устанавливаем score=999999.\n";
    Game g(Diff::HARD);
    g.score = 999999;
    std::cout << "    CHECK: tickInterval() >= 0.05f\n";
    CHECK(g.tickInterval() >= 0.05f);
}

// ═══════════════════════════════════════════════════════════════════
// 4. Инверсия управления (INVERT)
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-16: поедание инверсии включает invCtrl и устанавливает invTick=30") {
    std::cout << "  [WB-16] Кладём INVERT перед головой, вызываем step().\n";
    Game g(Diff::HARD);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::INVERT;
    g.dir = g.nextDir = Dir::RIGHT;
    g.step();
    std::cout << "    CHECK: invCtrl == true\n";
    CHECK(g.invCtrl == true);
    std::cout << "    CHECK: invTick == 30\n";
    CHECK(g.invTick == 30);
}

TEST_CASE("WB-17a: invTick убывает на 1 каждый тик") {
    std::cout << "  [WB-17a] invTick=10, вызываем step() дважды.\n";
    Game g(Diff::HARMLESS);
    g.invCtrl = true; g.invTick = 10;
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5, 11}, {4, 11}, {3, 11}};
    g.dir = g.nextDir = Dir::RIGHT;
    g.step();
    std::cout << "    CHECK: invTick == 9 после первого step()\n";
    CHECK(g.invTick == 9);
    g.step();
    std::cout << "    CHECK: invTick == 8 после второго step()\n";
    CHECK(g.invTick == 8);
}

TEST_CASE("WB-17b: invCtrl сбрасывается когда invTick достигает нуля") {
    std::cout << "  [WB-17b] invTick=1, вызываем step() один раз.\n";
    Game g(Diff::HARMLESS);
    g.invCtrl = true; g.invTick = 1;
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5, 11}, {4, 11}, {3, 11}};
    g.dir = g.nextDir = Dir::RIGHT;
    g.step();
    std::cout << "    CHECK: invTick == 0\n";
    CHECK(g.invTick == 0);
    std::cout << "    CHECK: invCtrl == false\n";
    CHECK(g.invCtrl == false);
}

// ═══════════════════════════════════════════════════════════════════
// 5. Телепорт
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-18: телепорт перемещает голову в другое место") {
    std::cout << "  [WB-18] Кладём TELEPORT прямо перед головой, вызываем step().\n";
    Game g(Diff::HARD);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::TELEPORT;
    g.dir = g.nextDir = Dir::RIGHT;
    g.step();
    std::cout << "    CHECK: голова НЕ находится на позиции head.x+1 (телепортировалась)\n";
    CHECK(!(g.snake.front() == Pt{head.x + 1, head.y}));
}

TEST_CASE("WB-19: после телепорта на поле появляется новый телепорт") {
    std::cout << "  [WB-19] Съедаем TELEPORT, проверяем спавн нового.\n";
    Game g(Diff::HARD);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    Pt head = g.snake.front();
    g.grid[head.y][head.x + 1] = Cell::TELEPORT;
    g.dir = g.nextDir = Dir::RIGHT;
    g.step();
    int cnt = 0;
    for (auto& row : g.grid) for (auto& c : row) if (c == Cell::TELEPORT) cnt++;
    std::cout << "    CHECK: количество клеток TELEPORT == 1\n";
    CHECK(cnt == 1);
}

TEST_CASE("WB-34: телепорт не приземляет голову на тело змейки") {
    std::cout << "  [WB-34] Пустое поле, короткая змейка. После телепорта нет самопересечения.\n";
    Game g(Diff::HARD);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{14, 11}, {13, 11}, {12, 11}};
    g.dir = g.nextDir = Dir::RIGHT;
    g.grid[11][15] = Cell::TELEPORT;
    g.step();
    std::cout << "    CHECK: alive == true (телепорт не убил змейку)\n";
    CHECK(g.alive == true);
}

// ═══════════════════════════════════════════════════════════════════
// 6. Game::buildObstacles — количество стен
// ═══════════════════════════════════════════════════════════════════
static int wallCount(Diff d) {
    Game g(d);
    int n = 0;
    for (auto& row : g.grid) for (auto& c : row) if (c == Cell::WALL) n++;
    return n;
}

TEST_CASE("WB-20: HARMLESS — нет стен") {
    std::cout << "  [WB-20] Сложность HARMLESS, считаем стены.\n";
    std::cout << "    CHECK: количество WALL == 0\n";
    CHECK(wallCount(Diff::HARMLESS) == 0);
}

TEST_CASE("WB-21: EASY — от 8 до 16 стен") {
    std::cout << "  [WB-21] Сложность EASY, считаем стены.\n";
    int n = wallCount(Diff::EASY);
    std::cout << "    CHECK: количество WALL >= 8\n";
    CHECK(n >= 8);
    std::cout << "    CHECK: количество WALL <= 16\n";
    CHECK(n <= 16);
}

TEST_CASE("WB-22: MEDIUM — от 14 до 28 стен") {
    std::cout << "  [WB-22] Сложность MEDIUM, считаем стены.\n";
    int n = wallCount(Diff::MEDIUM);
    std::cout << "    CHECK: количество WALL >= 14\n";
    CHECK(n >= 14);
    std::cout << "    CHECK: количество WALL <= 28\n";
    CHECK(n <= 28);
}

TEST_CASE("WB-23: HARD — от 20 до 40 стен") {
    std::cout << "  [WB-23] Сложность HARD, считаем стены.\n";
    int n = wallCount(Diff::HARD);
    std::cout << "    CHECK: количество WALL >= 20\n";
    CHECK(n >= 20);
    std::cout << "    CHECK: количество WALL <= 40\n";
    CHECK(n <= 40);
}

// ═══════════════════════════════════════════════════════════════════
// 7. Инверсия клавиш WASD — все четыре направления
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-24: при invCtrl=true W → DOWN") {
    std::cout << "  [WB-24] invCtrl=true, dir=RIGHT, нажимаем W.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.invCtrl = true;
    g.handleKey(sf::Keyboard::W);
    std::cout << "    CHECK: nextDir == DOWN (W инвертировано)\n";
    CHECK(g.nextDir == Dir::DOWN);
}

TEST_CASE("WB-25: при invCtrl=true A → RIGHT") {
    std::cout << "  [WB-25] invCtrl=true, dir=DOWN, нажимаем A.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::DOWN;
    g.invCtrl = true;
    g.handleKey(sf::Keyboard::A);
    std::cout << "    CHECK: nextDir == RIGHT (A инвертировано)\n";
    CHECK(g.nextDir == Dir::RIGHT);
}

TEST_CASE("WB-31: при invCtrl=true S → UP") {
    std::cout << "  [WB-31] invCtrl=true, dir=RIGHT, нажимаем S.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::RIGHT;
    g.invCtrl = true;
    g.handleKey(sf::Keyboard::S);
    std::cout << "    CHECK: nextDir == UP (S инвертировано)\n";
    CHECK(g.nextDir == Dir::UP);
}

TEST_CASE("WB-32: при invCtrl=true D → LEFT") {
    std::cout << "  [WB-32] invCtrl=true, dir=DOWN, нажимаем D.\n";
    Game g(Diff::HARMLESS);
    g.dir = g.nextDir = Dir::DOWN;
    g.invCtrl = true;
    g.handleKey(sf::Keyboard::D);
    std::cout << "    CHECK: nextDir == LEFT (D инвертировано)\n";
    CHECK(g.nextDir == Dir::LEFT);
}

// ═══════════════════════════════════════════════════════════════════
// 8. Яд при минимальной длине
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-26: яд при длине == 3 не укорачивает змейку") {
    std::cout << "  [WB-26] Длина = 3 (минимум), кладём POISON перед головой.\n";
    Game g(Diff::MEDIUM);
    g.score = 20;
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{5,5},{4,5},{3,5}};
    g.dir = g.nextDir = Dir::RIGHT;
    g.grid[5][6] = Cell::POISON;
    g.step();
    std::cout << "    CHECK: snake.size() == 3 (не уменьшилась)\n";
    CHECK(g.snake.size() == 3);
}

// ═══════════════════════════════════════════════════════════════════
// 9. step() при мёртвой змейке
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-33: step() при alive=false не изменяет состояние игры") {
    std::cout << "  [WB-33] Устанавливаем alive=false, score=42, кладём FOOD, вызываем step().\n";
    Game g(Diff::HARMLESS);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.grid[g.snake.front().y][g.snake.front().x + 1] = Cell::FOOD;
    g.alive = false;
    g.score = 42;
    g.step();
    std::cout << "    CHECK: score == 42 (не изменился)\n";
    CHECK(g.score == 42);
    std::cout << "    CHECK: alive == false (по-прежнему мёртв)\n";
    CHECK(g.alive == false);
}

// ═══════════════════════════════════════════════════════════════════
// 10. AI — избегание стен
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-27: AI не выбирает направление прямо в стену") {
    std::cout << "  [WB-27] Стены слева и сверху от головы, еда справа.\n";
    Game g(Diff::HARD);
    for (auto& row : g.grid) for (auto& c : row) c = Cell::EMPTY;
    g.snake = {{1, 1}, {1, 2}, {1, 3}};
    g.dir = g.nextDir = Dir::UP;
    g.grid[1][0] = Cell::WALL;
    g.grid[0][1] = Cell::WALL;
    g.grid[1][14] = Cell::FOOD;
    AIResult res = computeAI(g);
    std::cout << "    CHECK: AI не выбрал LEFT (там стена)\n";
    CHECK(res.direction != Dir::LEFT);
    std::cout << "    CHECK: AI не выбрал UP (там стена)\n";
    CHECK(res.direction != Dir::UP);
}

// ═══════════════════════════════════════════════════════════════════
// 11. Records — топ-10
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-28: при >10 записях сохраняются только топ-10") {
    std::cout << "  [WB-28] Сохраняем 15 записей (10..150), загружаем список.\n";
    std::filesystem::create_directories("saves");
    std::filesystem::remove("saves/records.txt");
    for (int i = 1; i <= 15; i++)
        saveRecord({"d", "Лёгкий", i * 10, false});
    auto recs = loadRecords();
    std::cout << "    REQUIRE: список содержит ровно 10 записей\n";
    REQUIRE(recs.size() == 10);
    std::cout << "    CHECK: первый рекорд == 150 (максимальный)\n";
    CHECK(recs.front().score == 150);
    std::cout << "    CHECK: последний рекорд == 60 (10-й наибольший)\n";
    CHECK(recs.back().score == 60);
    std::filesystem::remove("saves/records.txt");
}

// ═══════════════════════════════════════════════════════════════════
// 12. saveToFile / loadFromFile — дополнительные поля
// ═══════════════════════════════════════════════════════════════════
TEST_CASE("WB-29: saveToFile сохраняет aiMode") {
    std::cout << "  [WB-29] Сохраняем игру с aiMode=true, загружаем в новый объект.\n";
    std::filesystem::create_directories("saves");
    Game g1(Diff::EASY, true);
    g1.saveToFile("saves/test_wb_ai.txt");
    Game g2(Diff::HARMLESS, false);
    g2.loadFromFile("saves/test_wb_ai.txt");
    std::cout << "    CHECK: g2.aiMode == true\n";
    CHECK(g2.aiMode == true);
    std::filesystem::remove("saves/test_wb_ai.txt");
}

TEST_CASE("WB-30: saveToFile сохраняет invCtrl и invTick") {
    std::cout << "  [WB-30] Сохраняем invCtrl=true, invTick=15, загружаем.\n";
    std::filesystem::create_directories("saves");
    Game g1(Diff::HARD);
    g1.invCtrl = true;
    g1.invTick = 15;
    g1.saveToFile("saves/test_wb_inv.txt");
    Game g2(Diff::HARMLESS);
    g2.loadFromFile("saves/test_wb_inv.txt");
    std::cout << "    CHECK: g2.invCtrl == true\n";
    CHECK(g2.invCtrl == true);
    std::cout << "    CHECK: g2.invTick == 15\n";
    CHECK(g2.invTick == 15);
    std::filesystem::remove("saves/test_wb_inv.txt");
}
