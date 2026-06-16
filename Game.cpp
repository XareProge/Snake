#include "Game.h"
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <ctime>

// ─── Вспомогательные функции (Types.h) ───────────────────────────
std::string diffName(Diff d) {
    switch (d) {
        case Diff::HARMLESS: return "Безобидный";
        case Diff::EASY:     return "Лёгкий";
        case Diff::MEDIUM:   return "Средний";
        case Diff::HARD:     return "Сложный";
    }
    return "";
}

std::string currentDate() {
    time_t t = time(nullptr);
    tm tm_info{};
    localtime_s(&tm_info, &t);
    char buf[20];
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M", &tm_info);
    return buf;
}

// ─── Конструктор ─────────────────────────────────────────────────
Game::Game(Diff d) : diff(d), rng(std::random_device{}()) {
    grid.assign(ROWS, std::vector<Cell>(COLS, Cell::EMPTY));

    // Начальное положение: три сегмента в центре, голова смотрит вправо
    int mx = COLS / 2, my = ROWS / 2;
    snake = { {mx, my}, {mx - 1, my}, {mx - 2, my} };

    buildObstacles();
    spawnOne(Cell::FOOD);
    if (d >= Diff::MEDIUM) spawnOne(Cell::POISON);
    if (d == Diff::HARD)  { spawnOne(Cell::TELEPORT); spawnOne(Cell::INVERT); }
}

// ─── Расстановка стен ────────────────────────────────────────────
void Game::buildObstacles() {
    if (diff == Diff::HARMLESS) return;

    int count = (diff == Diff::EASY ? 8 : diff == Diff::MEDIUM ? 14 : 20);

    for (int i = 0; i < count; ) {
        Pt p = rndEmpty();
        // Оставляем зону старта свободной
        if (abs(p.x - COLS / 2) < 4 && abs(p.y - ROWS / 2) < 4) continue;

        grid[p.y][p.x] = Cell::WALL;
        i++;

        // 50% — добавляем соседний блок для образования кластеров
        if (rng() % 2) {
            static const int dx[] = { 1,-1, 0, 0 };
            static const int dy[] = { 0, 0, 1,-1 };
            int idx = rng() % 4;
            Pt n = { p.x + dx[idx], p.y + dy[idx] };
            if (valid(n) && grid[n.y][n.x] == Cell::EMPTY && !onSnake(n)) {
                grid[n.y][n.x] = Cell::WALL;
                i++;
            }
        }
    }
}

// ─── Вспомогательные методы ──────────────────────────────────────
bool Game::valid(Pt p) const {
    return p.x >= 0 && p.x < COLS && p.y >= 0 && p.y < ROWS;
}

bool Game::onSnake(Pt p) const {
    for (auto& s : snake) if (s == p) return true;
    return false;
}

Pt Game::rndEmpty() {
    Pt p;
    do { p = { (int)(rng() % COLS), (int)(rng() % ROWS) }; }
    while (grid[p.y][p.x] != Cell::EMPTY || onSnake(p));
    return p;
}

void Game::spawnOne(Cell type) {
    // Не создаём объект, если он уже есть на поле
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            if (grid[y][x] == type) return;
    Pt p = rndEmpty();
    grid[p.y][p.x] = type;
}

// ─── Обработка клавиши ───────────────────────────────────────────
void Game::handleKey(sf::Keyboard::Key k) {
    Dir d = nextDir;
    if      (k == sf::Keyboard::Up    || k == sf::Keyboard::W) d = Dir::UP;
    else if (k == sf::Keyboard::Down  || k == sf::Keyboard::S) d = Dir::DOWN;
    else if (k == sf::Keyboard::Left  || k == sf::Keyboard::A) d = Dir::LEFT;
    else if (k == sf::Keyboard::Right || k == sf::Keyboard::D) d = Dir::RIGHT;
    else return;

    // При активной инверсии переворачиваем введённое направление
    if (invCtrl) {
        if      (d == Dir::UP)    d = Dir::DOWN;
        else if (d == Dir::DOWN)  d = Dir::UP;
        else if (d == Dir::LEFT)  d = Dir::RIGHT;
        else if (d == Dir::RIGHT) d = Dir::LEFT;
    }

    // Запрет разворота на 180 градусов
    if (d == Dir::UP    && dir == Dir::DOWN)  return;
    if (d == Dir::DOWN  && dir == Dir::UP)    return;
    if (d == Dir::LEFT  && dir == Dir::RIGHT) return;
    if (d == Dir::RIGHT && dir == Dir::LEFT)  return;
    nextDir = d;
}

// ─── Игровой шаг ─────────────────────────────────────────────────
void Game::step() {
    if (!alive) return;
    dir = nextDir;

    // Таймер инверсии
    if (invTick > 0 && --invTick == 0) invCtrl = false;

    // Вычисляем новое положение головы
    Pt nh = snake.front();
    switch (dir) {
        case Dir::UP:    nh.y--; break;
        case Dir::DOWN:  nh.y++; break;
        case Dir::LEFT:  nh.x--; break;
        case Dir::RIGHT: nh.x++; break;
    }

    // Столкновение с границей поля
    if (!valid(nh)) { alive = false; return; }

    // Столкновение с собой (хвост не считается — он сдвинется)
    for (size_t i = 0; i + 1 < snake.size(); i++)
        if (snake[i] == nh) { alive = false; return; }

    Cell c = grid[nh.y][nh.x];

    // Столкновение со стеной
    if (c == Cell::WALL) { alive = false; return; }

    bool grow = false;
    switch (c) {
        case Cell::FOOD:
            score += 10;
            grow = true;
            grid[nh.y][nh.x] = Cell::EMPTY;
            spawnOne(Cell::FOOD);
            break;

        case Cell::POISON:
            // Уменьшаем счёт и укорачиваем змейку
            score = std::max(0, score - 15);
            if (snake.size() > 3) snake.pop_back();
            grid[nh.y][nh.x] = Cell::EMPTY;
            spawnOne(Cell::POISON);
            break;

        case Cell::TELEPORT:
            // Перемещаем голову в случайное пустое место
            grid[nh.y][nh.x] = Cell::EMPTY;
            nh = rndEmpty();
            spawnOne(Cell::TELEPORT);
            break;

        case Cell::INVERT:
            // Включаем инверсию управления на 30 тиков
            invCtrl = true;
            invTick = 30;
            grid[nh.y][nh.x] = Cell::EMPTY;
            spawnOne(Cell::INVERT);
            break;

        default: break;
    }

    snake.push_front(nh);
    if (!grow) snake.pop_back();
}

// ─── Скорость ────────────────────────────────────────────────────
float Game::tickInterval() const {
    float base = (diff == Diff::HARMLESS ? 0.19f :
                  diff == Diff::EASY     ? 0.16f :
                  diff == Diff::MEDIUM   ? 0.14f : 0.12f);
    // Скорость растёт со счётом, но не быстрее 0.07 с/тик
    return std::max(0.07f, base - score * 0.00025f);
}
