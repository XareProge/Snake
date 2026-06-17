#include "Game.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>

// ─── Вспомогательные функции (объявлены в Types.h) ───────────────
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
Game::Game(Diff d, bool ai) : diff(d), aiMode(ai), rng(std::random_device{}()) {
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

        // 50% — добавляем соседний блок (кластеры)
        if (rng() % 2) {
            static const int dx[] = { 1,-1, 0, 0 };
            static const int dy[] = { 0, 0, 1,-1 };
            int idx = rng() % 4;
            Pt n = { p.x + dx[idx], p.y + dy[idx] };
            if (validPt(n) && grid[n.y][n.x] == Cell::EMPTY && !onSnake(n)) {
                grid[n.y][n.x] = Cell::WALL;
                i++;
            }
        }
    }
}

// ─── Вспомогательные методы ──────────────────────────────────────
bool Game::validPt(Pt p) const {
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
    // Не создаём, если объект уже есть на поле
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            if (grid[y][x] == type) return;
    Pt p = rndEmpty();
    grid[p.y][p.x] = type;
}

// ─── Обработка клавиши ───────────────────────────────────────────
void Game::handleKey(sf::Keyboard::Key k) {
    if (aiMode) return; // в ИИ-режиме клавиатура не управляет змейкой

    Dir d = nextDir;
    if      (k == sf::Keyboard::Up    || k == sf::Keyboard::W) d = Dir::UP;
    else if (k == sf::Keyboard::Down  || k == sf::Keyboard::S) d = Dir::DOWN;
    else if (k == sf::Keyboard::Left  || k == sf::Keyboard::A) d = Dir::LEFT;
    else if (k == sf::Keyboard::Right || k == sf::Keyboard::D) d = Dir::RIGHT;
    else return;

    // При активной инверсии переворачиваем команду
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

    Pt nh = snake.front();
    switch (dir) {
        case Dir::UP:    nh.y--; break;
        case Dir::DOWN:  nh.y++; break;
        case Dir::LEFT:  nh.x--; break;
        case Dir::RIGHT: nh.x++; break;
    }

    // Столкновение с границей
    if (!validPt(nh)) { alive = false; return; }

    // Столкновение с собой (хвост не считается — он сдвинется)
    for (size_t i = 0; i + 1 < snake.size(); i++)
        if (snake[i] == nh) { alive = false; return; }

    Cell c = grid[nh.y][nh.x];
    if (c == Cell::WALL) { alive = false; return; }

    bool grow = false;
    switch (c) {
        case Cell::FOOD:
            score += 10; grow = true;
            grid[nh.y][nh.x] = Cell::EMPTY;
            spawnOne(Cell::FOOD);
            break;
        case Cell::POISON:
            score = std::max(0, score - 15);
            if (snake.size() > 3) snake.pop_back();
            grid[nh.y][nh.x] = Cell::EMPTY;
            spawnOne(Cell::POISON);
            break;
        case Cell::TELEPORT:
            grid[nh.y][nh.x] = Cell::EMPTY;
            nh = rndEmpty();
            spawnOne(Cell::TELEPORT);
            break;
        case Cell::INVERT:
            invCtrl = true; invTick = 30;
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
    // В ИИ-режиме чуть быстрее для зрелищности
    if (aiMode) base *= 0.75f;
    return std::max(0.05f, base - score * 0.00025f);
}

// ─── Сериализация направления ─────────────────────────────────────
static std::string dirToStr(Dir d) {
    switch (d) {
        case Dir::UP:    return "UP";
        case Dir::DOWN:  return "DOWN";
        case Dir::LEFT:  return "LEFT";
        case Dir::RIGHT: return "RIGHT";
    }
    return "RIGHT";
}

static Dir strToDir(const std::string& s) {
    if (s == "UP")   return Dir::UP;
    if (s == "DOWN") return Dir::DOWN;
    if (s == "LEFT") return Dir::LEFT;
    return Dir::RIGHT;
}

static std::string cellToStr(Cell c) {
    switch (c) {
        case Cell::FOOD:     return "FOOD";
        case Cell::POISON:   return "POISON";
        case Cell::WALL:     return "WALL";
        case Cell::TELEPORT: return "TELEPORT";
        case Cell::INVERT:   return "INVERT";
        default:             return "EMPTY";
    }
}

static Cell strToCell(const std::string& s) {
    if (s == "FOOD")     return Cell::FOOD;
    if (s == "POISON")   return Cell::POISON;
    if (s == "WALL")     return Cell::WALL;
    if (s == "TELEPORT") return Cell::TELEPORT;
    if (s == "INVERT")   return Cell::INVERT;
    return Cell::EMPTY;
}

// ─── Сохранение игры ─────────────────────────────────────────────
bool Game::saveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "# Сохранение игры Змейка\n";
    f << "diff:"     << (int)diff      << "\n";
    f << "score:"    << score          << "\n";
    f << "alive:"    << (int)alive     << "\n";
    f << "aiMode:"   << (int)aiMode    << "\n";
    f << "invCtrl:"  << (int)invCtrl   << "\n";
    f << "invTick:"  << invTick        << "\n";
    f << "dir:"      << dirToStr(dir)  << "\n";
    f << "nextDir:"  << dirToStr(nextDir) << "\n";

    // Сегменты змейки: x,y;x,y;...
    f << "snake:";
    for (size_t i = 0; i < snake.size(); i++) {
        if (i) f << ";";
        f << snake[i].x << "," << snake[i].y;
    }
    f << "\n";

    // Непустые клетки поля: тип:x,y;x,y;...
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            if (grid[y][x] != Cell::EMPTY)
                f << cellToStr(grid[y][x]) << ":" << x << "," << y << "\n";
        }
    }
    return true;
}

// ─── Загрузка игры ───────────────────────────────────────────────
bool Game::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    // Сбрасываем поле
    grid.assign(ROWS, std::vector<Cell>(COLS, Cell::EMPTY));
    snake.clear();

    try {
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;

            auto sep = line.find(':');
            if (sep == std::string::npos) continue;
            std::string key = line.substr(0, sep);
            std::string val = line.substr(sep + 1);

            if      (key == "diff")     diff     = static_cast<Diff>(std::stoi(val));
            else if (key == "score")    score    = std::stoi(val);
            else if (key == "alive")    alive    = (bool)std::stoi(val);
            else if (key == "aiMode")   aiMode   = (bool)std::stoi(val);
            else if (key == "invCtrl")  invCtrl  = (bool)std::stoi(val);
            else if (key == "invTick")  invTick  = std::stoi(val);
            else if (key == "dir")      dir      = strToDir(val);
            else if (key == "nextDir")  nextDir  = strToDir(val);
            else if (key == "snake") {
                std::istringstream ss(val);
                std::string seg;
                while (std::getline(ss, seg, ';')) {
                    auto comma = seg.find(',');
                    if (comma != std::string::npos)
                        snake.push_back({ std::stoi(seg.substr(0, comma)),
                                          std::stoi(seg.substr(comma + 1)) });
                }
            }
            else {
                Cell c = strToCell(key);
                if (c != Cell::EMPTY) {
                    auto comma = val.find(',');
                    if (comma != std::string::npos) {
                        int x = std::stoi(val.substr(0, comma));
                        int y = std::stoi(val.substr(comma + 1));
                        if (x >= 0 && x < COLS && y >= 0 && y < ROWS)
                            grid[y][x] = c;
                    }
                }
            }
        }
    } catch (...) {
        return false;
    }

    return !snake.empty();
}
