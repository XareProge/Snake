#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <random>

// ─── Размеры поля и окна ─────────────────────────────────────────
constexpr int CELL  = 26;   // размер клетки в пикселях
constexpr int COLS  = 28;   // столбцов
constexpr int ROWS  = 22;   // строк
constexpr int UI_W  = 200;  // ширина панели справа
constexpr int WIN_W = COLS * CELL + UI_W;
constexpr int WIN_H = ROWS * CELL;

// ─── Состояния игры ──────────────────────────────────────────────
enum class State { MENU, PLAYING, GAME_OVER };

enum class Dir { UP, DOWN, LEFT, RIGHT };

struct Pt {
    int x, y;
    bool operator==(const Pt& o) const { return x == o.x && y == o.y; }
};

// ─── Вспомогательные функции рисования ──────────────────────────
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

// ─── Игровая логика ──────────────────────────────────────────────
struct Game {
    std::deque<Pt> snake;
    Dir dir     = Dir::RIGHT;
    Dir nextDir = Dir::RIGHT;
    Pt  food{};
    int score   = 0;
    bool alive  = true;
    std::mt19937 rng{ std::random_device{}() };

    Game() {
        // Начальное положение змейки в центре поля
        int mx = COLS / 2, my = ROWS / 2;
        snake = { {mx, my}, {mx - 1, my}, {mx - 2, my} };
        spawnFood();
    }

    bool onSnake(Pt p) const {
        for (auto& s : snake) if (s == p) return true;
        return false;
    }

    void spawnFood() {
        do { food = { (int)(rng() % COLS), (int)(rng() % ROWS) }; }
        while (onSnake(food));
    }

    void handleKey(sf::Keyboard::Key k) {
        Dir d = nextDir;
        if      (k == sf::Keyboard::Up    || k == sf::Keyboard::W) d = Dir::UP;
        else if (k == sf::Keyboard::Down  || k == sf::Keyboard::S) d = Dir::DOWN;
        else if (k == sf::Keyboard::Left  || k == sf::Keyboard::A) d = Dir::LEFT;
        else if (k == sf::Keyboard::Right || k == sf::Keyboard::D) d = Dir::RIGHT;
        else return;

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

        Pt nh = snake.front();
        switch (dir) {
            case Dir::UP:    nh.y--; break;
            case Dir::DOWN:  nh.y++; break;
            case Dir::LEFT:  nh.x--; break;
            case Dir::RIGHT: nh.x++; break;
        }

        // Столкновение с границей поля
        if (nh.x < 0 || nh.x >= COLS || nh.y < 0 || nh.y >= ROWS) {
            alive = false; return;
        }
        // Столкновение с собой (хвост не считается — он сдвинется)
        for (size_t i = 0; i + 1 < snake.size(); i++)
            if (snake[i] == nh) { alive = false; return; }

        bool grow = (nh == food);
        if (grow) { score += 10; spawnFood(); }
        snake.push_front(nh);
        if (!grow) snake.pop_back();
    }

    float tickInterval() const {
        // Скорость постепенно растёт со счётом
        return std::max(0.07f, 0.18f - score * 0.0003f);
    }
};

// ─── Сетка поля ──────────────────────────────────────────────────
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

// ─── Еда ─────────────────────────────────────────────────────────
void renderFood(sf::RenderWindow& w, Pt food) {
    float cx = food.x * CELL + CELL / 2.f;
    float cy = food.y * CELL + CELL / 2.f;
    float r  = CELL * 0.38f;
    // Свечение
    drawCircle(w, cx, cy, r + 6.f, {230, 65, 65, 40});
    drawCircle(w, cx, cy, r + 3.f, {230, 65, 65, 70});
    // Тело яблока
    drawCircle(w, cx, cy, r, {230, 65, 65});
    // Блик
    drawCircle(w, cx - r * 0.28f, cy - r * 0.28f, r * 0.32f, {255, 180, 180, 190});
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

    // Голова
    if (!g.snake.empty()) {
        float x = g.snake[0].x * CELL, y = g.snake[0].y * CELL;
        drawRect(w, x + pad, y + pad, sz, sz, {72, 230, 95}, {18, 90, 35}, 1.5f);
        // Блик на голове
        drawRect(w, x + pad + 2, y + pad + 2, sz * 0.55f, 3.f, {200, 255, 200, 160});
        // Глаза
        float ep = sz * 0.24f, er = 2.5f;
        float hx = x + pad, hy = y + pad;
        drawCircle(w, hx + ep,        hy + ep, er, {10, 12, 22});
        drawCircle(w, hx + sz - ep,   hy + ep, er, {10, 12, 22});
        // Блики на глазах
        drawCircle(w, hx + ep - 0.8f,      hy + ep - 0.8f, 1.2f, {255, 255, 255, 200});
        drawCircle(w, hx + sz - ep - 0.8f, hy + ep - 0.8f, 1.2f, {255, 255, 255, 200});
    }
}

// ─── Боковая панель ──────────────────────────────────────────────
void renderUI(sf::RenderWindow& w, const sf::Font& font, const Game& g) {
    float ox = (float)(COLS * CELL);
    float cx = ox + UI_W / 2.f;

    drawRect(w, ox, 0, (float)UI_W, (float)WIN_H, {14, 16, 28});
    // Линия-разделитель
    sf::Vertex sep[] = { {{ox, 0}, {35, 38, 62}}, {{ox, (float)WIN_H}, {35, 38, 62}} };
    w.draw(sep, 2, sf::Lines);

    // Заголовок
    drawText(w, font, "ЗМЕЙКА", cx, 22.f, 48, {72, 230, 95}, true);

    // Блок счёта
    drawRect(w, ox + 12.f, 92.f, UI_W - 24.f, 80.f, {20, 22, 40});
    drawText(w, font, "СЧЁТ",                  cx, 100.f, 18, {95, 100, 125}, true);
    drawText(w, font, std::to_string(g.score), cx, 122.f, 40, {255, 255, 255}, true);

    // Блок длины
    drawRect(w, ox + 12.f, 188.f, UI_W - 24.f, 72.f, {20, 22, 40});
    drawText(w, font, "ДЛИНА",                          cx, 196.f, 18, {95, 100, 125}, true);
    drawText(w, font, std::to_string(g.snake.size()),   cx, 218.f, 30, {200, 205, 225}, true);

    // Подсказка управления
    float hy = WIN_H - 100.f;
    drawRect(w, ox + 10.f, hy, UI_W - 20.f, 94.f, {18, 20, 36});
    drawText(w, font, "WASD / стрелки", cx, hy + 8.f,  15, {95, 100, 125}, true);
    drawText(w, font, "Enter — заново",  cx, hy + 36.f, 15, {95, 100, 125}, true);
    drawText(w, font, "Esc — меню",      cx, hy + 64.f, 15, {95, 100, 125}, true);
}

// ─── Главное меню ────────────────────────────────────────────────
void renderMenu(sf::RenderWindow& w, const sf::Font& font, int selected) {
    w.clear({10, 12, 22});

    // Декоративная сетка на фоне
    sf::Color grid{18, 20, 36};
    for (int x = 0; x <= WIN_W; x += CELL) {
        sf::Vertex ln[] = { {{(float)x, 0}, grid}, {{(float)x, (float)WIN_H}, grid} };
        w.draw(ln, 2, sf::Lines);
    }
    for (int y = 0; y <= WIN_H; y += CELL) {
        sf::Vertex ln[] = { {{0, (float)y}, grid}, {{(float)WIN_W, (float)y}, grid} };
        w.draw(ln, 2, sf::Lines);
    }

    float cx = WIN_W / 2.f;
    float cy = WIN_H / 2.f;

    // Большой заголовок
    drawText(w, font, "ЗМЕЙКА", cx, cy - 130.f, 72, {72, 230, 95}, true);
    // Подзаголовок
    drawText(w, font, "классическая игра", cx, cy - 55.f, 18, {55, 130, 75}, true);

    // Пункты меню
    const std::string items[] = { "Начать игру", "Выход" };
    const int COUNT = 2;
    for (int i = 0; i < COUNT; i++) {
        bool active = (i == selected);
        float bx = cx - 150.f, by = cy + 10.f + i * 70.f;
        float bw = 300.f,      bh = 52.f;

        // Фон кнопки
        sf::Color bgCol  = active ? sf::Color{25, 55, 35}  : sf::Color{18, 20, 36};
        sf::Color brdCol = active ? sf::Color{72, 230, 95} : sf::Color{35, 38, 62};
        drawRect(w, bx, by, bw, bh, bgCol, brdCol, active ? 2.f : 1.f);

        // Текст кнопки
        sf::Color txtCol = active ? sf::Color{72, 230, 95} : sf::Color{160, 165, 185};
        drawText(w, font, items[i], cx, by + bh / 2.f, 22, txtCol, true);
    }

    // Подсказка навигации
    drawText(w, font, "стрелки / WASD — выбор       Enter — подтвердить",
             cx, WIN_H - 35.f, 13, {55, 60, 85}, true);
}

// ─── Экран конца игры ────────────────────────────────────────────
void renderGameOver(sf::RenderWindow& w, const sf::Font& font, int score) {
    // Затемнение
    drawRect(w, 0, 0, (float)WIN_W, (float)WIN_H, {0, 0, 0, 165});

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 45.f;
    drawRect(w, cx - 185.f, cy - 28.f, 370.f, 160.f, {14, 16, 28}, {50, 38, 62}, 2.f);
    drawText(w, font, "КОНЕЦ ИГРЫ",                     cx, cy,         44, {230, 80, 80},  true);
    drawText(w, font, "Счёт: " + std::to_string(score), cx, cy + 58.f,  26, {200, 205, 225}, true);
    drawText(w, font, "Enter — заново   Esc — меню",    cx, cy + 105.f, 16, {95, 100, 125}, true);
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

    State state    = State::MENU;
    int   menuSel  = 0;      // выбранный пункт меню (0 = начать, 1 = выход)
    Game  game;
    sf::Clock tickClock;

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (ev.type == sf::Event::KeyPressed) {
                auto k = ev.key.code;

                // ── Главное меню ──
                if (state == State::MENU) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W)
                        menuSel = (menuSel + 1) % 2;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S)
                        menuSel = (menuSel + 1) % 2;
                    if (k == sf::Keyboard::Enter) {
                        if (menuSel == 0) {
                            // Начать игру
                            game = Game();
                            tickClock.restart();
                            state = State::PLAYING;
                        } else {
                            // Выход
                            window.close();
                        }
                    }
                    if (k == sf::Keyboard::Escape) window.close();
                }

                // ── Игровой процесс ──
                else if (state == State::PLAYING) {
                    if (k == sf::Keyboard::Escape) {
                        state = State::MENU;
                        menuSel = 0;
                    }
                    game.handleKey(k);
                }

                // ── Конец игры ──
                else if (state == State::GAME_OVER) {
                    if (k == sf::Keyboard::Enter) {
                        // Заново
                        game = Game();
                        tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) {
                        state = State::MENU;
                        menuSel = 0;
                    }
                }
            }
        }

        // Обновление по таймеру только во время игры
        if (state == State::PLAYING &&
            tickClock.getElapsedTime().asSeconds() >= game.tickInterval())
        {
            game.step();
            tickClock.restart();
            if (!game.alive) state = State::GAME_OVER;
        }

        // ── Отрисовка кадра ──
        window.clear({10, 12, 22});

        if (state == State::MENU) {
            renderMenu(window, font, menuSel);
        } else {
            renderGrid(window);
            renderFood(window, game.food);
            renderSnake(window, game);
            renderUI(window, font, game);
            if (state == State::GAME_OVER)
                renderGameOver(window, font, game.score);
        }

        window.display();
    }
    return 0;
}
