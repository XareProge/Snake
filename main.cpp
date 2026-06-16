#include <SFML/Graphics.hpp>
#include <deque>
#include <vector>
#include <string>
#include <random>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

// ─── Размеры поля и окна ─────────────────────────────────────────
constexpr int CELL  = 26;
constexpr int COLS  = 28;
constexpr int ROWS  = 22;
constexpr int UI_W  = 220;
constexpr int WIN_W = COLS * CELL + UI_W;
constexpr int WIN_H = ROWS * CELL;

// ─── Состояния и перечисления ────────────────────────────────────
enum class State    { MENU, DIFFICULTY, PLAYING, GAME_OVER, RECORDS };
enum class Dir      { UP, DOWN, LEFT, RIGHT };
enum class Diff     { HARMLESS = 0, EASY, MEDIUM, HARD };
enum class Cell     { EMPTY, FOOD, POISON, WALL, TELEPORT, INVERT };

struct Pt {
    int x, y;
    bool operator==(const Pt& o) const { return x == o.x && y == o.y; }
};

struct Record {
    std::string date;
    std::string diff;
    int score = 0;
};

// ─── Вспомогательные функции ─────────────────────────────────────
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

sf::Color lerpColor(sf::Color a, sf::Color b, float t) {
    return {
        (uint8_t)(a.r + t * (b.r - a.r)),
        (uint8_t)(a.g + t * (b.g - a.g)),
        (uint8_t)(a.b + t * (b.b - a.b))
    };
}

void drawRect(sf::RenderWindow& w, float x, float y, float wd, float ht,
              sf::Color fill, sf::Color outline = sf::Color::Transparent, float ot = 0.f)
{
    sf::RectangleShape r({wd, ht});
    r.setPosition(x, y);
    r.setFillColor(fill);
    if (ot > 0.f) { r.setOutlineColor(outline); r.setOutlineThickness(ot); }
    w.draw(r);
}

void drawCircle(sf::RenderWindow& w, float cx, float cy, float r, sf::Color fill) {
    sf::CircleShape s(r);
    s.setOrigin(r, r); s.setPosition(cx, cy); s.setFillColor(fill);
    w.draw(s);
}

// fromUtf8 нужен для корректного отображения кириллицы
void drawText(sf::RenderWindow& w, const sf::Font& f, const std::string& txt,
              float x, float y, unsigned sz, sf::Color col, bool center = false)
{
    sf::Text t(sf::String::fromUtf8(txt.begin(), txt.end()), f, sz);
    t.setFillColor(col);
    if (center) {
        auto b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    }
    t.setPosition(x, y);
    w.draw(t);
}

// ─── Рекорды ─────────────────────────────────────────────────────
std::vector<Record> loadRecords() {
    std::vector<Record> recs;
    std::ifstream f("records.txt");
    std::string line;
    while (std::getline(f, line)) {
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
    std::ofstream f("records.txt", std::ios::app);
    f << r.date << "|" << r.diff << "|" << r.score << "\n";
}

// ─── Игровая логика ──────────────────────────────────────────────
struct Game {
    std::deque<Pt>              snake;
    Dir                         dir     = Dir::RIGHT;
    Dir                         nextDir = Dir::RIGHT;
    std::vector<std::vector<Cell>> grid;
    int                         score   = 0;
    bool                        alive   = true;
    bool                        invCtrl = false;  // инверсия управления активна
    int                         invTick = 0;      // таймер инверсии
    Diff                        diff;
    std::mt19937                rng;

    explicit Game(Diff d) : diff(d), rng(std::random_device{}()) {
        grid.assign(ROWS, std::vector<Cell>(COLS, Cell::EMPTY));

        // Начальное положение змейки в центре
        int mx = COLS / 2, my = ROWS / 2;
        snake = { {mx, my}, {mx - 1, my}, {mx - 2, my} };

        buildObstacles();
        spawnOne(Cell::FOOD);
        if (d >= Diff::MEDIUM) spawnOne(Cell::POISON);
        if (d == Diff::HARD)  { spawnOne(Cell::TELEPORT); spawnOne(Cell::INVERT); }
    }

    // Расстановка стен в зависимости от сложности
    void buildObstacles() {
        if (diff == Diff::HARMLESS) return;
        int count = (diff == Diff::EASY ? 8 : diff == Diff::MEDIUM ? 14 : 20);
        for (int i = 0; i < count; ) {
            Pt p = rndEmpty();
            // Оставляем центр поля свободным для старта
            if (abs(p.x - COLS / 2) < 4 && abs(p.y - ROWS / 2) < 4) continue;
            grid[p.y][p.x] = Cell::WALL;
            i++;
            // С вероятностью 50% добавляем соседний блок (формируем кластеры)
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

    bool valid(Pt p) const {
        return p.x >= 0 && p.x < COLS && p.y >= 0 && p.y < ROWS;
    }

    bool onSnake(Pt p) const {
        for (auto& s : snake) if (s == p) return true;
        return false;
    }

    Pt rndEmpty() {
        Pt p;
        do { p = { (int)(rng() % COLS), (int)(rng() % ROWS) }; }
        while (grid[p.y][p.x] != Cell::EMPTY || onSnake(p));
        return p;
    }

    // Размещает один объект заданного типа, если его ещё нет на поле
    void spawnOne(Cell type) {
        for (int y = 0; y < ROWS; y++)
            for (int x = 0; x < COLS; x++)
                if (grid[y][x] == type) return;
        Pt p = rndEmpty();
        grid[p.y][p.x] = type;
    }

    void handleKey(sf::Keyboard::Key k) {
        Dir d = nextDir;
        if      (k == sf::Keyboard::Up    || k == sf::Keyboard::W) d = Dir::UP;
        else if (k == sf::Keyboard::Down  || k == sf::Keyboard::S) d = Dir::DOWN;
        else if (k == sf::Keyboard::Left  || k == sf::Keyboard::A) d = Dir::LEFT;
        else if (k == sf::Keyboard::Right || k == sf::Keyboard::D) d = Dir::RIGHT;
        else return;

        // При инверсии переворачиваем команду
        if (invCtrl) {
            if (d == Dir::UP)    d = Dir::DOWN;
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

    void step() {
        if (!alive) return;
        dir = nextDir;

        // Отсчёт таймера инверсии
        if (invTick > 0 && --invTick == 0) invCtrl = false;

        Pt nh = snake.front();
        switch (dir) {
            case Dir::UP:    nh.y--; break;
            case Dir::DOWN:  nh.y++; break;
            case Dir::LEFT:  nh.x--; break;
            case Dir::RIGHT: nh.x++; break;
        }

        // Столкновение с границей
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
                // Обычная еда: +10 очков, змейка растёт
                score += 10;
                grow = true;
                grid[nh.y][nh.x] = Cell::EMPTY;
                spawnOne(Cell::FOOD);
                break;

            case Cell::POISON:
                // Отравленная еда: -15 очков, змейка укорачивается
                score = std::max(0, score - 15);
                if (snake.size() > 3) snake.pop_back();
                grid[nh.y][nh.x] = Cell::EMPTY;
                spawnOne(Cell::POISON);
                break;

            case Cell::TELEPORT:
                // Телепорт: змейка перемещается в случайное место
                grid[nh.y][nh.x] = Cell::EMPTY;
                nh = rndEmpty();
                spawnOne(Cell::TELEPORT);
                break;

            case Cell::INVERT:
                // Инверсия: управление переворачивается на 30 тиков
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

    float tickInterval() const {
        // Базовая скорость зависит от сложности, ускоряется со счётом
        float base = (diff == Diff::HARMLESS ? 0.19f :
                      diff == Diff::EASY     ? 0.16f :
                      diff == Diff::MEDIUM   ? 0.14f : 0.12f);
        return std::max(0.07f, base - score * 0.00025f);
    }
};

// ─── Декоративная сетка на фоне ──────────────────────────────────
void renderBgGrid(sf::RenderWindow& w, int width, int height) {
    sf::Color col{18, 20, 36};
    for (int x = 0; x <= width;  x += CELL) {
        sf::Vertex ln[] = { {{(float)x, 0}, col}, {{(float)x, (float)height}, col} };
        w.draw(ln, 2, sf::Lines);
    }
    for (int y = 0; y <= height; y += CELL) {
        sf::Vertex ln[] = { {{0, (float)y}, col}, {{(float)width, (float)y}, col} };
        w.draw(ln, 2, sf::Lines);
    }
}

// ─── Сетка игрового поля ─────────────────────────────────────────
void renderGrid(sf::RenderWindow& w) {
    int gw = COLS * CELL;
    drawRect(w, 0, 0, (float)gw, (float)WIN_H, {10, 12, 22});
    sf::Color grid{20, 22, 40};
    for (int x = 0; x <= COLS; x++) {
        sf::Vertex ln[] = { {{(float)(x*CELL), 0}, grid}, {{(float)(x*CELL), (float)WIN_H}, grid} };
        w.draw(ln, 2, sf::Lines);
    }
    for (int y = 0; y <= ROWS; y++) {
        sf::Vertex ln[] = { {{0, (float)(y*CELL)}, grid}, {{(float)gw, (float)(y*CELL)}, grid} };
        w.draw(ln, 2, sf::Lines);
    }
}

// ─── Объекты на поле ─────────────────────────────────────────────
void renderCell(sf::RenderWindow& w, int gx, int gy, Cell c) {
    float x  = gx * CELL, y = gy * CELL;
    float pad = 2.f, sz = CELL - pad * 2;
    float cx = x + CELL / 2.f, cy = y + CELL / 2.f;
    float r  = sz * 0.42f;

    switch (c) {
        case Cell::WALL:
            // Стена: серый прямоугольник с обводкой и бликом
            drawRect(w, x + pad, y + pad, sz, sz, {85, 90, 115}, {120, 125, 150}, 1.f);
            drawRect(w, x + pad, y + pad, sz * 0.35f, 2.f, {150, 155, 175});
            drawRect(w, x + pad, y + pad, 2.f, sz * 0.35f, {150, 155, 175});
            break;

        case Cell::FOOD:
            // Обычная еда: красное яблоко с блеском
            drawCircle(w, cx, cy, r + 5.f, {230, 65, 65, 40});
            drawCircle(w, cx, cy, r + 2.f, {230, 65, 65, 70});
            drawCircle(w, cx, cy, r,        {230, 65, 65});
            drawCircle(w, cx - r * 0.28f, cy - r * 0.28f, r * 0.32f, {255, 180, 180, 190});
            break;

        case Cell::POISON:
            // Отравленная еда: фиолетовый шар со знаком «–»
            drawCircle(w, cx, cy, r + 4.f, {168, 48, 230, 40});
            drawCircle(w, cx, cy, r,        {168, 48, 230});
            drawRect(w,  cx - r * 0.55f, cy - 1.5f, r * 1.1f, 3.f, {255, 255, 255, 200});
            break;

        case Cell::TELEPORT:
            // Телепорт: голубое кольцо с заливкой
            drawCircle(w, cx, cy, r + 5.f, {45, 205, 225, 50});
            {
                sf::CircleShape ring(r);
                ring.setOrigin(r, r); ring.setPosition(cx, cy);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor({45, 205, 225});
                ring.setOutlineThickness(3.f);
                w.draw(ring);
            }
            drawCircle(w, cx, cy, r * 0.4f, {45, 205, 225});
            break;

        case Cell::INVERT:
            // Инверсия: жёлтый шар со знаком «?»
            drawCircle(w, cx, cy, r + 4.f, {228, 182, 48, 40});
            drawCircle(w, cx, cy, r,        {228, 182, 48});
            drawRect(w, cx - 2.f, cy - r * 0.55f, 4.f, r * 0.7f, {40, 30, 10, 200});
            drawRect(w, cx - 2.f, cy + r * 0.25f, 4.f, r * 0.25f, {40, 30, 10, 200});
            break;

        default: break;
    }
}

// ─── Змейка ──────────────────────────────────────────────────────
void renderSnake(sf::RenderWindow& w, const Game& g) {
    float pad = 3.f, sz = CELL - pad * 2;

    // Тело: рисуем с хвоста, чтобы голова перекрывала
    for (size_t i = g.snake.size(); i-- > 0; ) {
        float t   = (float)i / (float)g.snake.size();
        sf::Color col = lerpColor({42, 175, 68}, {18, 80, 35}, t * 0.7f);
        float x = g.snake[i].x * CELL, y = g.snake[i].y * CELL;
        drawRect(w, x + pad, y + pad, sz, sz, col, {15, 65, 28}, 1.f);
        // Блик на сегменте
        drawRect(w, x + pad + 2, y + pad + 2, sz * 0.45f, 2.f, {140, 255, 150, 100});
    }

    // Голова (меняет цвет при инверсии управления)
    if (!g.snake.empty()) {
        float x = g.snake[0].x * CELL, y = g.snake[0].y * CELL;
        sf::Color headCol = g.invCtrl ? sf::Color{228, 182, 48} : sf::Color{72, 230, 95};
        drawRect(w, x + pad, y + pad, sz, sz, headCol, {18, 90, 35}, 1.5f);
        drawRect(w, x + pad + 2, y + pad + 2, sz * 0.55f, 3.f, {200, 255, 200, 160});
        // Глаза
        float ep = sz * 0.24f, er = 2.5f;
        float hx = x + pad, hy = y + pad;
        drawCircle(w, hx + ep,        hy + ep, er, {10, 12, 22});
        drawCircle(w, hx + sz - ep,   hy + ep, er, {10, 12, 22});
        drawCircle(w, hx + ep - 0.8f,      hy + ep - 0.8f, 1.2f, {255, 255, 255, 200});
        drawCircle(w, hx + sz - ep - 0.8f, hy + ep - 0.8f, 1.2f, {255, 255, 255, 200});
    }
}

// ─── Боковая панель ──────────────────────────────────────────────
void renderUI(sf::RenderWindow& w, const sf::Font& font, const Game& g) {
    float ox = (float)(COLS * CELL);
    float cx = ox + UI_W / 2.f;

    drawRect(w, ox, 0, (float)UI_W, (float)WIN_H, {14, 16, 28});
    sf::Vertex sep[] = { {{ox, 0}, {35, 38, 62}}, {{ox, (float)WIN_H}, {35, 38, 62}} };
    w.draw(sep, 2, sf::Lines);

    // Заголовок
    drawText(w, font, "ЗМЕЙКА", cx, 18.f, 44, {72, 230, 95}, true);

    // Сложность
    sf::Color diffCol = (g.diff == Diff::HARD   ? sf::Color{230, 80, 80} :
                         g.diff == Diff::MEDIUM  ? sf::Color{228, 182, 48} :
                         g.diff == Diff::EASY    ? sf::Color{120, 200, 90} :
                                                   sf::Color{72, 230, 95});
    drawText(w, font, diffName(g.diff), cx, 68.f, 15, diffCol, true);

    // Блок счёта
    drawRect(w, ox + 12.f, 98.f, UI_W - 24.f, 78.f, {20, 22, 40});
    drawText(w, font, "СЧЁТ",                  cx, 106.f, 16, {95, 100, 125}, true);
    drawText(w, font, std::to_string(g.score), cx, 126.f, 38, {255, 255, 255}, true);

    // Блок длины
    drawRect(w, ox + 12.f, 192.f, UI_W - 24.f, 68.f, {20, 22, 40});
    drawText(w, font, "ДЛИНА",                          cx, 200.f, 16, {95, 100, 125}, true);
    drawText(w, font, std::to_string(g.snake.size()),   cx, 220.f, 28, {200, 205, 225}, true);

    // Предупреждение об инверсии
    if (g.invCtrl) {
        drawRect(w, ox + 12.f, 276.f, UI_W - 24.f, 40.f, {60, 45, 10});
        drawText(w, font, "! ИНВЕРСИЯ !", cx, 288.f, 14, {228, 182, 48}, true);
    }

    // Легенда объектов (для сложностей выше безобидного)
    if (g.diff >= Diff::MEDIUM) {
        float ly = 340.f;
        drawText(w, font, "ОБЪЕКТЫ", cx, ly, 13, {55, 60, 85}, true);
        ly += 22.f;
        drawCircle(w, ox + 22.f, ly + 6.f, 6.f, {168, 48, 230});
        drawText(w, font, "яд  -15 очков", ox + 36.f, ly, 12, {160, 165, 185});
        if (g.diff == Diff::HARD) {
            ly += 24.f;
            sf::CircleShape ring(6.f); ring.setOrigin(6.f, 6.f);
            ring.setPosition(ox + 22.f, ly + 6.f);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor({45, 205, 225}); ring.setOutlineThickness(2.f);
            w.draw(ring);
            drawText(w, font, "телепорт", ox + 36.f, ly, 12, {160, 165, 185});
            ly += 24.f;
            drawCircle(w, ox + 22.f, ly + 6.f, 6.f, {228, 182, 48});
            drawText(w, font, "инверсия", ox + 36.f, ly, 12, {160, 165, 185});
        }
    }

    // Подсказка управления
    float hy = WIN_H - 105.f;
    drawRect(w, ox + 10.f, hy, UI_W - 20.f, 98.f, {18, 20, 36});
    drawText(w, font, "WASD / стрелки", cx, hy + 8.f,  13, {95, 100, 125}, true);
    drawText(w, font, "Enter — заново",  cx, hy + 32.f, 13, {95, 100, 125}, true);
    drawText(w, font, "Esc — меню",      cx, hy + 56.f, 13, {95, 100, 125}, true);
    drawText(w, font, "R — рекорды",     cx, hy + 80.f, 13, {95, 100, 125}, true);
}

// ─── Главное меню ────────────────────────────────────────────────
void renderMenu(sf::RenderWindow& w, const sf::Font& font, int sel) {
    w.clear({10, 12, 22});
    renderBgGrid(w, WIN_W, WIN_H);

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;

    drawText(w, font, "ЗМЕЙКА",          cx, cy - 140.f, 72, {72, 230, 95}, true);
    drawText(w, font, "классическая игра", cx, cy - 62.f,  18, {45, 120, 65}, true);

    const std::string items[] = { "Начать игру", "Рекорды", "Выход" };
    for (int i = 0; i < 3; i++) {
        bool active = (i == sel);
        float bx = cx - 155.f, by = cy - 10.f + i * 68.f;
        sf::Color bg  = active ? sf::Color{25, 55, 35}  : sf::Color{18, 20, 36};
        sf::Color brd = active ? sf::Color{72, 230, 95} : sf::Color{35, 38, 62};
        drawRect(w, bx, by, 310.f, 52.f, bg, brd, active ? 2.f : 1.f);
        sf::Color tc = active ? sf::Color{72, 230, 95} : sf::Color{155, 160, 180};
        drawText(w, font, items[i], cx, by + 26.f, 22, tc, true);
    }

    drawText(w, font, "стрелки / WASD — выбор       Enter — подтвердить",
             cx, WIN_H - 35.f, 13, {45, 50, 75}, true);
}

// ─── Экран выбора сложности ──────────────────────────────────────
void renderDifficulty(sf::RenderWindow& w, const sf::Font& font, int sel) {
    w.clear({10, 12, 22});
    renderBgGrid(w, WIN_W, WIN_H);

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    drawText(w, font, "СЛОЖНОСТЬ", cx, cy - 155.f, 48, {72, 230, 95}, true);

    const std::string names[] = { "Безобидный", "Лёгкий", "Средний", "Сложный" };
    const std::string descs[] = {
        "без препятствий",
        "стены на поле",
        "стены + ядовитая еда",
        "стены + яд + телепорт + инверсия"
    };
    const sf::Color cols[] = {
        {72,  230, 95},
        {120, 200, 90},
        {228, 182, 48},
        {230, 80,  80}
    };

    for (int i = 0; i < 4; i++) {
        bool active = (i == sel);
        float bx = cx - 200.f, by = cy - 80.f + i * 68.f;
        sf::Color bg  = active ? sf::Color{20, 40, 28}  : sf::Color{16, 18, 32};
        sf::Color brd = active ? cols[i]                 : sf::Color{30, 33, 55};
        drawRect(w, bx, by, 400.f, 56.f, bg, brd, active ? 2.f : 1.f);
        sf::Color nc = active ? cols[i] : sf::Color{140, 145, 165};
        drawText(w, font, names[i], cx, by + 12.f, 20, nc, true);
        sf::Color dc = active ? sf::Color{160, 165, 185} : sf::Color{75, 80, 105};
        drawText(w, font, descs[i], cx, by + 34.f, 13, dc, true);
    }

    drawText(w, font, "Esc — назад", cx, WIN_H - 35.f, 13, {45, 50, 75}, true);
}

// ─── Таблица рекордов ────────────────────────────────────────────
void renderRecords(sf::RenderWindow& w, const sf::Font& font,
                   const std::vector<Record>& recs)
{
    w.clear({10, 12, 22});
    renderBgGrid(w, WIN_W, WIN_H);

    float cx = WIN_W / 2.f;
    drawText(w, font, "ТАБЛИЦА РЕКОРДОВ", cx, 40.f, 36, {72, 230, 95}, true);
    drawText(w, font, "Esc / R — назад",  cx, 88.f, 14, {55, 60, 85},  true);

    if (recs.empty()) {
        drawText(w, font, "Рекордов пока нет", cx, WIN_H / 2.f, 22, {55, 60, 85}, true);
        return;
    }

    // Шапка таблицы
    float startY = 125.f;
    float lx = cx - 280.f;
    drawText(w, font, "#", lx,         startY, 14, {55, 60, 85});
    drawText(w, font, "Дата",   lx + 40.f,  startY, 14, {55, 60, 85});
    drawText(w, font, "Сложность", lx + 200.f, startY, 14, {55, 60, 85});
    drawText(w, font, "Счёт",  lx + 370.f, startY, 14, {55, 60, 85});
    startY += 22.f;
    // Разделитель
    sf::Vertex sep[] = { {{lx, startY}, {35,38,62}}, {{lx + 560.f, startY}, {35,38,62}} };
    w.draw(sep, 2, sf::Lines);
    startY += 10.f;

    int show = std::min((int)recs.size(), 13);
    for (int i = 0; i < show; i++) {
        // Первые 3 места выделяем цветом
        sf::Color col = (i == 0 ? sf::Color{228, 182, 48} :
                         i  < 3 ? sf::Color{200, 205, 225} :
                                  sf::Color{95, 100, 125});
        float ry = startY + i * 26.f;
        drawText(w, font, std::to_string(i + 1),  lx,          ry, 14, col);
        drawText(w, font, recs[i].date,            lx + 40.f,  ry, 14, col);
        drawText(w, font, recs[i].diff,            lx + 200.f, ry, 14, col);
        drawText(w, font, std::to_string(recs[i].score), lx + 370.f, ry, 14, col);
    }
}

// ─── Экран конца игры ────────────────────────────────────────────
void renderGameOver(sf::RenderWindow& w, const sf::Font& font, int score,
                    const std::vector<Record>& recs)
{
    // Затемнение
    drawRect(w, 0, 0, (float)WIN_W, (float)WIN_H, {0, 0, 0, 170});

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 80.f;
    drawRect(w, cx - 200.f, cy - 25.f, 400.f, 220.f, {14, 16, 28}, {50, 38, 62}, 2.f);

    drawText(w, font, "КОНЕЦ ИГРЫ",                     cx, cy,         42, {230, 80,  80},  true);
    drawText(w, font, "Счёт: " + std::to_string(score), cx, cy + 55.f,  26, {200, 205, 225}, true);

    // Показываем топ-3 прямо на экране конца игры
    drawText(w, font, "Лучшие результаты:", cx, cy + 95.f, 14, {55, 60, 85}, true);
    int show = std::min((int)recs.size(), 3);
    for (int i = 0; i < show; i++) {
        sf::Color col = (i == 0 ? sf::Color{228, 182, 48} : sf::Color{160, 165, 185});
        std::string line = std::to_string(i+1) + ".  " +
                           recs[i].diff + "   " + std::to_string(recs[i].score);
        drawText(w, font, line, cx, cy + 118.f + i * 22.f, 13, col, true);
    }

    drawText(w, font, "Enter — заново   Esc — меню", cx, cy + 188.f, 15, {95, 100, 125}, true);
}

// ─── Точка входа ─────────────────────────────────────────────────
int main() {
    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H), L"Змейка",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(120);

    // Загрузка шрифта с поддержкой кириллицы
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    State state   = State::MENU;
    int   menuSel = 0;
    int   diffSel = 0;
    auto  records = loadRecords();

    Game* game = nullptr;
    sf::Clock tickClock;

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (ev.type == sf::Event::KeyPressed) {
                auto k = ev.key.code;

                // ── Главное меню ──────────────────────────────────
                if (state == State::MENU) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W)
                        menuSel = (menuSel + 2) % 3;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S)
                        menuSel = (menuSel + 1) % 3;
                    if (k == sf::Keyboard::Enter) {
                        if      (menuSel == 0) { state = State::DIFFICULTY; diffSel = 0; }
                        else if (menuSel == 1) { records = loadRecords(); state = State::RECORDS; }
                        else                    window.close();
                    }
                    if (k == sf::Keyboard::Escape) window.close();
                }

                // ── Выбор сложности ───────────────────────────────
                else if (state == State::DIFFICULTY) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W)
                        diffSel = (diffSel + 3) % 4;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S)
                        diffSel = (diffSel + 1) % 4;
                    if (k == sf::Keyboard::Enter) {
                        delete game;
                        game = new Game(static_cast<Diff>(diffSel));
                        tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                }

                // ── Игровой процесс ───────────────────────────────
                else if (state == State::PLAYING) {
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                    if (k == sf::Keyboard::R)      { records = loadRecords(); state = State::RECORDS; }
                    if (game) game->handleKey(k);
                }

                // ── Конец игры ────────────────────────────────────
                else if (state == State::GAME_OVER) {
                    if (k == sf::Keyboard::Enter && game) {
                        // Перезапуск с той же сложностью
                        Diff d = game->diff;
                        delete game;
                        game = new Game(d);
                        tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                }

                // ── Таблица рекордов ──────────────────────────────
                else if (state == State::RECORDS) {
                    if (k == sf::Keyboard::Escape || k == sf::Keyboard::R) {
                        // Возврат туда, откуда пришли
                        state = (game && !game->alive) ? State::GAME_OVER : State::MENU;
                    }
                }
            }
        }

        // Обновление игры по таймеру
        if (state == State::PLAYING && game &&
            tickClock.getElapsedTime().asSeconds() >= game->tickInterval())
        {
            game->step();
            tickClock.restart();
            if (!game->alive) {
                // Сохраняем рекорд и обновляем таблицу
                saveRecord({ currentDate(), diffName(game->diff), game->score });
                records = loadRecords();
                state = State::GAME_OVER;
            }
        }

        // ── Отрисовка кадра ──────────────────────────────────────
        window.clear({10, 12, 22});

        if (state == State::MENU) {
            renderMenu(window, font, menuSel);
        }
        else if (state == State::DIFFICULTY) {
            renderDifficulty(window, font, diffSel);
        }
        else if (state == State::RECORDS) {
            renderRecords(window, font, records);
        }
        else if (game) {
            // Игровое поле
            renderGrid(window);
            for (int y = 0; y < ROWS; y++)
                for (int x = 0; x < COLS; x++)
                    if (game->grid[y][x] != Cell::EMPTY)
                        renderCell(window, x, y, game->grid[y][x]);
            renderSnake(window, *game);
            renderUI(window, font, *game);
            if (state == State::GAME_OVER)
                renderGameOver(window, font, game->score, records);
        }

        window.display();
    }

    delete game;
    return 0;
}
