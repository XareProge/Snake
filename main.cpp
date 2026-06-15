#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <random>

constexpr int CELL  = 26;
constexpr int COLS  = 28;
constexpr int ROWS  = 22;
constexpr int UI_W  = 200;
constexpr int WIN_W = COLS * CELL + UI_W;
constexpr int WIN_H = ROWS * CELL;

enum class Dir { UP, DOWN, LEFT, RIGHT };

struct Pt {
    int x, y;
    bool operator==(const Pt& o) const { return x == o.x && y == o.y; }
};

// ─── Helpers ─────────────────────────────────────────────────────
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

void drawText(sf::RenderWindow& w, const sf::Font& f, const std::string& txt,
              float x, float y, unsigned sz, sf::Color col, bool center = false)
{
    sf::Text t(txt, f, sz);
    t.setFillColor(col);
    if (center) {
        auto b = t.getLocalBounds();
        t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
    }
    t.setPosition(x, y);
    w.draw(t);
}

// ─── Game ────────────────────────────────────────────────────────
struct Game {
    std::deque<Pt> snake;
    Dir dir     = Dir::RIGHT;
    Dir nextDir = Dir::RIGHT;
    Pt  food{};
    int score   = 0;
    bool alive  = true;
    std::mt19937 rng{ std::random_device{}() };

    Game() {
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

        // Border collision
        if (nh.x < 0 || nh.x >= COLS || nh.y < 0 || nh.y >= ROWS) {
            alive = false; return;
        }
        // Self collision (exclude tail that will move away)
        for (size_t i = 0; i + 1 < snake.size(); i++)
            if (snake[i] == nh) { alive = false; return; }

        bool grow = (nh == food);
        if (grow) { score += 10; spawnFood(); }
        snake.push_front(nh);
        if (!grow) snake.pop_back();
    }

    float tickInterval() const {
        // Speed up gradually with score
        return std::max(0.07f, 0.18f - score * 0.0003f);
    }
};

// ─── Render ──────────────────────────────────────────────────────
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

void renderFood(sf::RenderWindow& w, Pt food) {
    float cx = food.x * CELL + CELL / 2.f;
    float cy = food.y * CELL + CELL / 2.f;
    float r  = CELL * 0.38f;
    // Soft glow
    drawCircle(w, cx, cy, r + 6.f, {230, 65, 65, 40});
    drawCircle(w, cx, cy, r + 3.f, {230, 65, 65, 70});
    // Body
    drawCircle(w, cx, cy, r, {230, 65, 65});
    // Shine
    drawCircle(w, cx - r * 0.28f, cy - r * 0.28f, r * 0.32f, {255, 180, 180, 190});
}

void renderSnake(sf::RenderWindow& w, const Game& g) {
    float pad = 3.f, sz = CELL - pad * 2;

    // Body (back to front so head renders on top)
    for (size_t i = g.snake.size(); i-- > 0; ) {
        float t   = (float)i / (float)g.snake.size();
        sf::Color col = lerpColor({42, 175, 68}, {18, 80, 35}, t * 0.7f);
        float x = g.snake[i].x * CELL, y = g.snake[i].y * CELL;
        drawRect(w, x + pad, y + pad, sz, sz, col, {15, 65, 28}, 1.f);
        // Top-left shine streak
        drawRect(w, x + pad + 2, y + pad + 2, sz * 0.45f, 2.f, {140, 255, 150, 100});
    }

    // Head
    if (!g.snake.empty()) {
        float x = g.snake[0].x * CELL, y = g.snake[0].y * CELL;
        drawRect(w, x + pad, y + pad, sz, sz, {72, 230, 95}, {18, 90, 35}, 1.5f);
        // Shine
        drawRect(w, x + pad + 2, y + pad + 2, sz * 0.55f, 3.f, {200, 255, 200, 160});
        // Eyes (positioned relative to move direction)
        float ep = sz * 0.24f, er = 2.5f;
        float hx = x + pad, hy = y + pad;
        drawCircle(w, hx + ep,        hy + ep,        er, {10, 12, 22});
        drawCircle(w, hx + sz - ep,   hy + ep,        er, {10, 12, 22});
        // Eye highlight
        drawCircle(w, hx + ep - 0.8f,      hy + ep - 0.8f,      1.2f, {255, 255, 255, 200});
        drawCircle(w, hx + sz - ep - 0.8f, hy + ep - 0.8f,      1.2f, {255, 255, 255, 200});
    }
}

void renderUI(sf::RenderWindow& w, const sf::Font& font, const Game& g) {
    float ox = (float)(COLS * CELL);
    float cx = ox + UI_W / 2.f;

    drawRect(w, ox, 0, (float)UI_W, (float)WIN_H, {14, 16, 28});
    // Separator line
    sf::Vertex sep[] = { {{ox, 0}, {35, 38, 62}}, {{ox, (float)WIN_H}, {35, 38, 62}} };
    w.draw(sep, 2, sf::Lines);

    // Title
    drawText(w, font, "SNAKE", cx, 35.f, 30, {72, 230, 95}, true);

    // Score block
    drawRect(w, ox + 15.f, 90.f, UI_W - 30.f, 58.f, {20, 22, 40});
    drawText(w, font, "СЧЁТ",              cx, 97.f,  12, {95, 100, 125}, true);
    drawText(w, font, std::to_string(g.score), cx, 115.f, 26, {255, 255, 255}, true);

    // Length
    drawRect(w, ox + 15.f, 168.f, UI_W - 30.f, 48.f, {20, 22, 40});
    drawText(w, font, "ДЛИНА",                          cx, 175.f, 12, {95, 100, 125}, true);
    drawText(w, font, std::to_string(g.snake.size()),   cx, 192.f, 18, {200, 205, 225}, true);

    // Controls hint
    float hy = WIN_H - 75.f;
    drawRect(w, ox + 10.f, hy, UI_W - 20.f, 65.f, {18, 20, 36});
    drawText(w, font, "WASD / стрелки", cx, hy + 8.f,  10, {95, 100, 125}, true);
    drawText(w, font, "Enter — заново",  cx, hy + 24.f, 10, {95, 100, 125}, true);
    drawText(w, font, "Esc — выход",     cx, hy + 40.f, 10, {95, 100, 125}, true);
}

void renderGameOver(sf::RenderWindow& w, const sf::Font& font, int score) {
    // Dim overlay
    drawRect(w, 0, 0, (float)WIN_W, (float)WIN_H, {0, 0, 0, 165});

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 45.f;
    drawRect(w, cx - 145.f, cy - 22.f, 290.f, 115.f, {14, 16, 28}, {50, 38, 62}, 2.f);
    drawText(w, font, "GAME OVER",                    cx, cy,        30, {230, 80, 80},  true);
    drawText(w, font, "Счёт: " + std::to_string(score), cx, cy + 42.f, 18, {200, 205, 225}, true);
    drawText(w, font, "Enter — сыграть снова",         cx, cy + 72.f, 12, {95, 100, 125}, true);
}

// ─── Main ────────────────────────────────────────────────────────
int main() {
    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H), L"Snake",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(120);

    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    Game game;
    sf::Clock tickClock;
    bool gameOver = false;

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type == sf::Event::KeyPressed) {
                auto k = ev.key.code;
                if (k == sf::Keyboard::Escape) window.close();
                if (gameOver && k == sf::Keyboard::Enter) {
                    game = Game(); gameOver = false; tickClock.restart();
                }
                game.handleKey(k);
            }
        }

        if (!gameOver && tickClock.getElapsedTime().asSeconds() >= game.tickInterval()) {
            game.step();
            tickClock.restart();
            if (!game.alive) gameOver = true;
        }

        window.clear({10, 12, 22});
        renderGrid(window);
        renderFood(window, game.food);
        renderSnake(window, game);
        renderUI(window, font, game);
        if (gameOver) renderGameOver(window, font, game.score);
        window.display();
    }
    return 0;
}
