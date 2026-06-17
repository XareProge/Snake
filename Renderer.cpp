#include "Renderer.h"
#include <string>
#include <algorithm>

// ════════════════════════════════════════════════════════════════
// Прямоугольники кнопок
// ════════════════════════════════════════════════════════════════

sf::FloatRect menuButtonRect(int i) {
    float cx = WIN_W / 2.f;
    return { cx - 155.f, 158.f + i * 62.f, 310.f, 50.f };
}

sf::FloatRect diffButtonRect(int i) {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    return { cx - 200.f, cy - 80.f + i * 68.f, 400.f, 56.f };
}

sf::FloatRect gameOverButtonRect(int i) {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 80.f;
    float bw = 180.f, bx = (i == 0) ? cx - bw - 10.f : cx + 10.f;
    return { bx, cy + 168.f, bw, 46.f };
}

sf::FloatRect pauseButtonRect(int i) {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    return { cx - 145.f, cy - 90.f + i * 56.f, 290.f, 46.f };
}

sf::FloatRect pauseSlotActionRect(int slot) {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    return { cx + 74.f, cy - 96.f + slot * 56.f, 100.f, 34.f };
}

sf::FloatRect pauseSlotDeleteRect(int slot) {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    return { cx + 180.f, cy - 96.f + slot * 56.f, 34.f, 34.f };
}

sf::FloatRect pauseBackRect() {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    return { cx - 82.f, cy + 76.f, 164.f, 38.f };
}

// ════════════════════════════════════════════════════════════════
// Низкоуровневые примитивы
// ════════════════════════════════════════════════════════════════

sf::Color lerpColor(sf::Color a, sf::Color b, float t) {
    return {
        (uint8_t)(a.r + t * (b.r - a.r)),
        (uint8_t)(a.g + t * (b.g - a.g)),
        (uint8_t)(a.b + t * (b.b - a.b))
    };
}

void drawRect(sf::RenderWindow& w, float x, float y, float wd, float ht,
              sf::Color fill, sf::Color outline, float ot)
{
    sf::RectangleShape r({wd, ht});
    r.setPosition(x, y); r.setFillColor(fill);
    if (ot > 0.f) { r.setOutlineColor(outline); r.setOutlineThickness(ot); }
    w.draw(r);
}

void drawCircle(sf::RenderWindow& w, float cx, float cy, float r, sf::Color fill) {
    sf::CircleShape s(r);
    s.setOrigin(r, r); s.setPosition(cx, cy); s.setFillColor(fill);
    w.draw(s);
}

void drawText(sf::RenderWindow& w, const sf::Font& f, const std::string& txt,
              float x, float y, unsigned sz, sf::Color col, bool center)
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

// ════════════════════════════════════════════════════════════════
// Внутренние вспомогательные функции
// ════════════════════════════════════════════════════════════════

static void drawButton(sf::RenderWindow& w, const sf::Font& font,
                        sf::FloatRect rect, const std::string& label,
                        sf::Color activeColor, bool sel, bool hov,
                        int fontSize = 16)
{
    bool hi = sel || hov;
    sf::Color bg  = hi    ? sf::Color{ 25, 55, 35 }  : sf::Color{ 18, 20, 36 };
    sf::Color brd = sel   ? activeColor               :
                    hov   ? sf::Color{ (uint8_t)(activeColor.r*0.7f),
                                       (uint8_t)(activeColor.g*0.7f),
                                       (uint8_t)(activeColor.b*0.7f) }
                          : sf::Color{ 35, 38, 62 };
    sf::Color tc  = hi    ? activeColor               : sf::Color{ 155, 160, 180 };
    drawRect(w, rect.left, rect.top, rect.width, rect.height, bg, brd, hi ? 2.f : 1.f);
    drawText(w, font, label,
             rect.left + rect.width  / 2.f,
             rect.top  + rect.height / 2.f,
             fontSize, tc, true);
}

static void drawBgGrid(sf::RenderWindow& w) {
    sf::Color col{ 18, 20, 36 };
    for (int x = 0; x <= WIN_W; x += CELL) {
        sf::Vertex ln[] = { {{(float)x,0},col}, {{(float)x,(float)WIN_H},col} };
        w.draw(ln, 2, sf::Lines);
    }
    for (int y = 0; y <= WIN_H; y += CELL) {
        sf::Vertex ln[] = { {{0,(float)y},col}, {{(float)WIN_W,(float)y},col} };
        w.draw(ln, 2, sf::Lines);
    }
}

// ════════════════════════════════════════════════════════════════
// Главное меню
// ════════════════════════════════════════════════════════════════

void renderMenu(sf::RenderWindow& w, const sf::Font& font,
                int selected, sf::Vector2i mousePos,
                const std::string& notif)
{
    w.clear({ 10, 12, 22 });
    drawBgGrid(w);

    float cx = WIN_W / 2.f;
    drawText(w, font, "ЗМЕЙКА",           cx, 42.f,  32, { 72, 230, 95 }, true);
    drawText(w, font, "классическая игра", cx, 104.f,  8, { 45, 120, 65 }, true);

    const std::string labels[] = {
        "Начать игру", "ИИ-режим", "Загрузить игру", "Рекорды", "Выход"
    };
    const sf::Color colors[] = {
        {72,230,95}, {80,180,230}, {72,230,95}, {72,230,95}, {72,230,95}
    };
    for (int i = 0; i < 5; i++) {
        auto rect = menuButtonRect(i);
        bool hov  = rect.contains((float)mousePos.x, (float)mousePos.y);
        drawButton(w, font, rect, labels[i], colors[i], selected == i, hov, 16);
    }

    if (!notif.empty()) {
        float ny = WIN_H - 62.f;
        drawRect(w, cx - 200.f, ny - 10.f, 400.f, 36.f, {40,10,10,220}, {230,80,80}, 1.5f);
        drawText(w, font, notif, cx, ny, 8, {230,80,80}, true);
    }

    drawText(w, font, "стрелки/WASD — выбор         Enter — подтвердить",
             cx, WIN_H - 24.f, 8, {130,135,165}, true);
}

// ════════════════════════════════════════════════════════════════
// Выбор сложности
// ════════════════════════════════════════════════════════════════

void renderDifficulty(sf::RenderWindow& w, const sf::Font& font,
                      int selected, sf::Vector2i mousePos, bool aiPending)
{
    w.clear({ 10, 12, 22 });
    drawBgGrid(w);

    float cx = WIN_W / 2.f;
    std::string title = aiPending ? "ИИ-РЕЖИМ: СЛОЖНОСТЬ" : "СЛОЖНОСТЬ";
    sf::Color titleCol = aiPending ? sf::Color{80,180,230} : sf::Color{72,230,95};
    drawText(w, font, title, cx, WIN_H / 2.f - 160.f, 16, titleCol, true);

    const std::string names[] = { "Безобидный", "Лёгкий", "Средний", "Сложный" };
    const std::string descs[] = {
        "без препятствий",
        "стены на поле",
        "стены + ядовитая еда",
        "стены + яд + телепорт + инверсия"
    };
    const sf::Color cols[] = {
        {72,230,95}, {120,200,90}, {228,182,48}, {230,80,80}
    };

    for (int i = 0; i < 4; i++) {
        auto rect = diffButtonRect(i);
        bool hov  = rect.contains((float)mousePos.x, (float)mousePos.y);
        bool sel  = (i == selected);
        sf::Color bg  = sel || hov ? sf::Color{20,40,28} : sf::Color{16,18,32};
        sf::Color brd = sel        ? cols[i]
                      : hov        ? sf::Color{(uint8_t)(cols[i].r*0.6f),
                                               (uint8_t)(cols[i].g*0.6f),
                                               (uint8_t)(cols[i].b*0.6f)}
                                   : sf::Color{30,33,55};
        drawRect(w, rect.left, rect.top, rect.width, rect.height,
                 bg, brd, sel || hov ? 2.f : 1.f);
        sf::Color nc = sel || hov ? cols[i]                : sf::Color{140,145,165};
        sf::Color dc = sel || hov ? sf::Color{160,165,185} : sf::Color{75,80,105};
        drawText(w, font, names[i], cx, rect.top + 12.f, 16, nc, true);
        drawText(w, font, descs[i], cx, rect.top + 38.f,  8, dc, true);
    }

    drawText(w, font, "Esc — назад", cx, WIN_H - 24.f, 8, {130,135,165}, true);
}

// ════════════════════════════════════════════════════════════════
// Игровой экран
// ════════════════════════════════════════════════════════════════

static void drawGrid(sf::RenderWindow& w) {
    int gw = COLS * CELL;
    drawRect(w, 0, 0, (float)gw, (float)WIN_H, {10,12,22});
    sf::Color grid{20,22,40};
    for (int x = 0; x <= COLS; x++) {
        sf::Vertex ln[] = { {{(float)(x*CELL),0},grid}, {{(float)(x*CELL),(float)WIN_H},grid} };
        w.draw(ln, 2, sf::Lines);
    }
    for (int y = 0; y <= ROWS; y++) {
        sf::Vertex ln[] = { {{0,(float)(y*CELL)},grid}, {{(float)gw,(float)(y*CELL)},grid} };
        w.draw(ln, 2, sf::Lines);
    }
}

static void drawCell(sf::RenderWindow& w, int gx, int gy, Cell c) {
    float x = gx*CELL, y = gy*CELL;
    float pad = 2.f, sz = CELL - pad*2;
    float cx = x + CELL/2.f, cy = y + CELL/2.f, r = sz*0.42f;

    switch (c) {
        case Cell::WALL:
            drawRect(w, x+pad, y+pad, sz, sz, {85,90,115}, {120,125,150}, 1.f);
            drawRect(w, x+pad, y+pad, sz*0.35f, 2.f, {150,155,175});
            drawRect(w, x+pad, y+pad, 2.f, sz*0.35f, {150,155,175});
            break;
        case Cell::FOOD:
            drawCircle(w, cx, cy, r+5.f, {230,65,65,40});
            drawCircle(w, cx, cy, r+2.f, {230,65,65,70});
            drawCircle(w, cx, cy, r,     {230,65,65});
            drawCircle(w, cx-r*0.28f, cy-r*0.28f, r*0.32f, {255,180,180,190});
            break;
        case Cell::POISON:
            drawCircle(w, cx, cy, r+4.f, {168,48,230,40});
            drawCircle(w, cx, cy, r,     {168,48,230});
            drawRect(w, cx-r*0.55f, cy-1.5f, r*1.1f, 3.f, {255,255,255,200});
            break;
        case Cell::TELEPORT:
            drawCircle(w, cx, cy, r+5.f, {45,205,225,50});
            {
                sf::CircleShape ring(r);
                ring.setOrigin(r,r); ring.setPosition(cx,cy);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor({45,205,225});
                ring.setOutlineThickness(3.f);
                w.draw(ring);
            }
            drawCircle(w, cx, cy, r*0.4f, {45,205,225});
            break;
        case Cell::INVERT:
            drawCircle(w, cx, cy, r+4.f, {228,182,48,40});
            drawCircle(w, cx, cy, r,     {228,182,48});
            drawRect(w, cx-2.f, cy-r*0.55f, 4.f, r*0.7f,  {40,30,10,200});
            drawRect(w, cx-2.f, cy+r*0.25f, 4.f, r*0.25f, {40,30,10,200});
            break;
        default: break;
    }
}

static void drawAIPath(sf::RenderWindow& w, const std::vector<Pt>& path) {
    int total = (int)path.size();
    for (int i = 0; i < total; i++) {
        float t   = 1.f - (float)i / (float)(total + 1);
        uint8_t a = (uint8_t)(180 * t);
        float cx  = path[i].x * CELL + CELL / 2.f;
        float cy  = path[i].y * CELL + CELL / 2.f;
        drawCircle(w, cx, cy, 3.5f, {80, 180, 230, a});
    }
}

static void drawSnake(sf::RenderWindow& w, const Game& g) {
    float pad = 3.f, sz = CELL - pad*2;

    for (size_t i = g.snake.size(); i-- > 0; ) {
        float t = (float)i / (float)g.snake.size();
        sf::Color col = lerpColor({42,175,68}, {18,80,35}, t*0.7f);
        float x = g.snake[i].x*CELL, y = g.snake[i].y*CELL;
        drawRect(w, x+pad, y+pad, sz, sz, col, {15,65,28}, 1.f);
        drawRect(w, x+pad+2, y+pad+2, sz*0.45f, 2.f, {140,255,150,100});
    }

    if (!g.snake.empty()) {
        float x = g.snake[0].x*CELL, y = g.snake[0].y*CELL;
        sf::Color head = g.invCtrl  ? sf::Color{228,182,48}  :
                         g.aiMode   ? sf::Color{80,180,230}   :
                                      sf::Color{72,230,95};
        drawRect(w, x+pad, y+pad, sz, sz, head, {18,90,35}, 1.5f);
        drawRect(w, x+pad+2, y+pad+2, sz*0.55f, 3.f, {200,255,200,160});
        float ep = sz*0.24f, er = 2.5f, hx = x+pad, hy = y+pad;
        drawCircle(w, hx+ep,      hy+ep, er, {10,12,22});
        drawCircle(w, hx+sz-ep,   hy+ep, er, {10,12,22});
        drawCircle(w, hx+ep-0.8f,      hy+ep-0.8f, 1.2f, {255,255,255,200});
        drawCircle(w, hx+sz-ep-0.8f,   hy+ep-0.8f, 1.2f, {255,255,255,200});
    }
}

static void drawUI(sf::RenderWindow& w, const sf::Font& font,
                   const Game& g, sf::Vector2i /*mousePos*/)
{
    float ox = (float)(COLS * CELL);
    float cx = ox + UI_W / 2.f;

    drawRect(w, ox, 0, (float)UI_W, (float)WIN_H, {14,16,28});
    sf::Vertex sep[] = { {{ox,0},{35,38,62}}, {{ox,(float)WIN_H},{35,38,62}} };
    w.draw(sep, 2, sf::Lines);

    // Заголовок панели
    drawText(w, font, "ЗМЕЙКА", cx, 18.f, 16, {72,230,95}, true);

    // Режим / сложность
    if (g.aiMode) {
        drawRect(w, ox+12.f, 46.f, UI_W-24.f, 22.f, {15,40,65});
        drawText(w, font, "ИИ-РЕЖИМ", cx, 50.f, 8, {80,180,230}, true);
    } else {
        sf::Color dc = (g.diff==Diff::HARD   ? sf::Color{230,80,80}  :
                        g.diff==Diff::MEDIUM  ? sf::Color{228,182,48} :
                        g.diff==Diff::EASY    ? sf::Color{120,200,90} :
                                               sf::Color{72,230,95});
        drawText(w, font, diffName(g.diff), cx, 50.f, 8, dc, true);
    }

    // Блок счёта (y=80, h=80, центр=120; группа 16+8+24=48px → старт 96)
    drawRect(w, ox+12.f, 80.f, UI_W-24.f, 80.f, {20,22,40});
    drawText(w, font, "СЧЁТ",                  cx, 104.f, 16, {120,125,155}, true);
    drawText(w, font, std::to_string(g.score), cx, 132.f, 24, {255,255,255}, true);

    // Блок длины (y=172, h=64, центр=204; группа 16+8+16=40px → старт 184)
    drawRect(w, ox+12.f, 172.f, UI_W-24.f, 64.f, {20,22,40});
    drawText(w, font, "ДЛИНА",                        cx, 192.f, 16, {120,125,155}, true);
    drawText(w, font, std::to_string(g.snake.size()), cx, 216.f, 16, {200,205,225}, true);

    // Инверсия
    if (g.invCtrl) {
        drawRect(w, ox+12.f, 248.f, UI_W-24.f, 28.f, {60,45,10});
        drawText(w, font, "! ИНВЕРСИЯ !", cx, 255.f, 8, {228,182,48}, true);
    }

    // Легенда объектов
    if (g.diff >= Diff::MEDIUM) {
        float ly = 292.f;
        drawText(w, font, "ОБЪЕКТЫ", cx, ly, 8, {100,105,135}, true);
        ly += 18.f;
        drawCircle(w, ox+20.f, ly+4.f, 5.f, {168,48,230});
        drawText(w, font, "яд  -15 очков", ox+32.f, ly, 8, {160,165,185});
        if (g.diff == Diff::HARD) {
            ly += 18.f;
            sf::CircleShape ring(5.f); ring.setOrigin(5.f,5.f);
            ring.setPosition(ox+20.f, ly+4.f);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor({45,205,225}); ring.setOutlineThickness(2.f);
            w.draw(ring);
            drawText(w, font, "телепорт", ox+32.f, ly, 8, {160,165,185});
            ly += 18.f;
            drawCircle(w, ox+20.f, ly+4.f, 5.f, {228,182,48});
            drawText(w, font, "инверсия", ox+32.f, ly, 8, {160,165,185});
        }
    }

    // Подсказки (столбиком внизу панели)
    const char* hints[] = {
        "WASD/стрелки", "Esc — пауза",
        "R — рекорды",  "F5 — сохранить", "F6 — загрузить"
    };
    float hy0 = WIN_H - 98.f;
    for (int i = 0; i < 5; i++)
        drawText(w, font, hints[i], cx, hy0 + i * 18.f, 8, {120,125,155}, true);
}

void renderGame(sf::RenderWindow& w, const sf::Font& font,
                const Game& g, const std::vector<Pt>& path,
                const std::string& notif, sf::Vector2i mousePos)
{
    drawGrid(w);
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            if (g.grid[y][x] != Cell::EMPTY)
                drawCell(w, x, y, g.grid[y][x]);

    if (!path.empty()) drawAIPath(w, path);
    drawSnake(w, g);
    drawUI(w, font, g, mousePos);

    if (!notif.empty()) {
        float cx = (float)(COLS * CELL) / 2.f;
        float cy = WIN_H / 2.f;
        drawRect(w, cx-140.f, cy-20.f, 280.f, 40.f, {14,16,28,220}, {72,230,95}, 2.f);
        drawText(w, font, notif, cx, cy, 8, {72,230,95}, true);
    }
}

// ════════════════════════════════════════════════════════════════
// Пауза
// ════════════════════════════════════════════════════════════════

void renderPause(sf::RenderWindow& w, const sf::Font& font,
                 const Game& g, const std::string& notif,
                 PausePanel panel, const SaveSlotInfo slotInfos[3],
                 sf::Vector2i mousePos)
{
    drawRect(w, 0, 0, (float)WIN_W, (float)WIN_H, {0,0,0,160});
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;

    if (panel == PausePanel::NONE) {
        drawRect(w, cx-170.f, cy-118.f, 340.f, 310.f, {14,16,28}, {50,55,80}, 2.f);
        drawText(w, font, "ПАУЗА",                          cx, cy-108.f, 24, {72,230,95},   true);
        drawText(w, font, "Счёт: "+std::to_string(g.score), cx, cy-74.f,   8, {160,165,185}, true);
        sf::Vertex sep[] = { {{cx-140.f,cy-58.f},{35,38,62}}, {{cx+140.f,cy-58.f},{35,38,62}} };
        w.draw(sep, 2, sf::Lines);
        const std::string labels[] = {
            "Продолжить", "Рестарт", "Сохранить [F5]", "Загрузить [F6]", "Главное меню"
        };
        const sf::Color colors[] = {
            {72,230,95}, {228,182,48}, {72,230,95}, {72,230,95}, {160,165,185}
        };
        for (int i = 0; i < 5; i++) {
            auto r = pauseButtonRect(i);
            drawButton(w, font, r, labels[i], colors[i], false,
                       r.contains((float)mousePos.x,(float)mousePos.y), 16);
        }
    } else {
        drawRect(w, cx-220.f, cy-180.f, 440.f, 380.f, {14,16,28}, {50,55,80}, 2.f);
        drawText(w, font, "ПАУЗА", cx, cy-168.f, 16, {72,230,95}, true);

        bool saving = (panel == PausePanel::SAVING);
        drawText(w, font,
                 saving ? "Выберите слот для сохранения" : "Выберите слот для загрузки",
                 cx, cy-144.f, 8, {120,125,155}, true);

        sf::Vertex s1[] = { {{cx-200.f,cy-128.f},{35,38,62}}, {{cx+200.f,cy-128.f},{35,38,62}} };
        w.draw(s1, 2, sf::Lines);

        for (int i = 0; i < 3; i++) {
            float ry = cy - 102.f + i * 56.f;
            const auto& si = slotInfos[i];

            sf::Color rowBg  = si.corrupted ? sf::Color{40,10,10}  : sf::Color{20,22,40};
            sf::Color rowBrd = si.corrupted ? sf::Color{140,35,35} : sf::Color{35,38,62};
            drawRect(w, cx-210.f, ry, 380.f, 44.f, rowBg, rowBrd, si.corrupted ? 1.5f : 1.f);
            drawText(w, font, "Слот "+std::to_string(i+1), cx-202.f, ry+14.f, 8, {100,105,135});

            if (si.corrupted) {
                drawText(w, font, "Файл повреждён", cx-148.f, ry+14.f, 8, {230,80,80});
            } else {
                std::string info = si.exists
                    ? si.diff + "  /  " + std::to_string(si.score) + " оч."
                    : "Пусто";
                drawText(w, font, info, cx-148.f, ry+14.f, 8,
                         si.exists ? sf::Color{160,165,185} : sf::Color{105,110,140});
            }

            auto ar = pauseSlotActionRect(i);
            bool ahov = ar.contains((float)mousePos.x, (float)mousePos.y);
            if (si.corrupted) {
                // повреждённый слот — кнопку загрузки не показываем
                drawRect(w, ar.left, ar.top, ar.width, ar.height, {18,20,36}, {50,20,20}, 1.f);
                drawText(w, font, "Ошибка", ar.left+ar.width/2.f, ar.top+ar.height/2.f,
                         8, {120,40,40}, true);
            } else if (saving || si.exists) {
                sf::Color ac = saving ? sf::Color{72,230,95} : sf::Color{80,180,230};
                drawButton(w, font, ar, saving ? "Сохранить" : "Загрузить", ac, false, ahov, 8);
            } else {
                drawRect(w, ar.left, ar.top, ar.width, ar.height, {18,20,36}, {30,33,55}, 1.f);
                drawText(w, font, "Пусто", ar.left+ar.width/2.f, ar.top+ar.height/2.f,
                         8, {105,110,140}, true);
            }

            if (si.exists) {
                auto dr = pauseSlotDeleteRect(i);
                bool dhov = dr.contains((float)mousePos.x, (float)mousePos.y);
                sf::Color dc = dhov ? sf::Color{230,80,80} : sf::Color{140,60,60};
                drawRect(w, dr.left, dr.top, dr.width, dr.height, {30,12,12}, dc, dhov?2.f:1.f);
                drawText(w, font, "X", dr.left+dr.width/2.f, dr.top+dr.height/2.f, 16, dc, true);
            }
        }

        sf::Vertex s2[] = { {{cx-200.f,cy+68.f},{35,38,62}}, {{cx+200.f,cy+68.f},{35,38,62}} };
        w.draw(s2, 2, sf::Lines);

        auto br = pauseBackRect();
        drawButton(w, font, br, "<  Назад", {160,165,185}, false,
                   br.contains((float)mousePos.x,(float)mousePos.y), 8);
    }

    if (!notif.empty()) {
        bool err = notif.find("Нет") != std::string::npos;
        sf::Color c = err ? sf::Color{230,80,80} : sf::Color{72,230,95};
        float ny = cy + (panel == PausePanel::NONE ? 160.f : 175.f);
        drawRect(w, cx-145.f, ny-8.f, 290.f, 28.f, {14,16,28,210}, c, 1.f);
        drawText(w, font, notif, cx, ny, 8, c, true);
    }
}

// ════════════════════════════════════════════════════════════════
// Обратный отсчёт (3-2-1)
// ════════════════════════════════════════════════════════════════

void renderCountdown(sf::RenderWindow& w, const sf::Font& font, float elapsed) {
    int num = 3 - (int)elapsed;
    if (num <= 0) return;

    float frac  = elapsed - (int)elapsed;
    float alpha = frac < 0.55f ? 1.f : 1.f - (frac - 0.55f) / 0.45f;
    uint8_t a   = (uint8_t)(alpha * 255.f);

    float cx = (float)(COLS * CELL) / 2.f;
    float cy = WIN_H / 2.f;

    drawCircle(w, cx, cy, 54.f, {10,12,22, (uint8_t)(a * 0.85f)});
    drawText(w, font, std::to_string(num), cx, cy,      32, {72,230,95,a}, true);
    drawText(w, font, "Приготовься!",      cx, cy+52.f,  8, {72,230,95, (uint8_t)(a*0.75f)}, true);
}

// ════════════════════════════════════════════════════════════════
// Конец игры
// ════════════════════════════════════════════════════════════════

void renderGameOver(sf::RenderWindow& w, const sf::Font& font,
                    const Game& g, const std::vector<Record>& recs,
                    sf::Vector2i mousePos)
{
    renderGame(w, font, g, {}, {}, mousePos);
    drawRect(w, 0, 0, (float)WIN_W, (float)WIN_H, {0,0,0,170});

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 80.f;
    drawRect(w, cx-205.f, cy-28.f, 410.f, 250.f, {14,16,28}, {50,38,62}, 2.f);

    drawText(w, font, "КОНЕЦ ИГРЫ",                     cx, cy,      24, {230,80,80},   true);
    drawText(w, font, "Счёт: "+std::to_string(g.score), cx, cy+42.f, 16, {200,205,225}, true);

    drawText(w, font, "Лучшие результаты:", cx, cy+82.f, 8, {100,105,135}, true);
    int show = std::min((int)recs.size(), 3);
    for (int i = 0; i < show; i++) {
        sf::Color col = (i==0 ? sf::Color{228,182,48} : sf::Color{160,165,185});
        std::string line = std::to_string(i+1)+".  "+recs[i].diff
                          +"   "+std::to_string(recs[i].score);
        drawText(w, font, line, cx, cy+102.f+i*22.f, 8, col, true);
    }

    const std::string btnLabels[] = { "Играть снова", "В меню" };
    for (int i = 0; i < 2; i++) {
        auto rect = gameOverButtonRect(i);
        bool hov  = rect.contains((float)mousePos.x, (float)mousePos.y);
        sf::Color bg  = hov ? sf::Color{25,55,35}  : sf::Color{18,20,36};
        sf::Color brd = hov ? sf::Color{72,230,95} : sf::Color{35,38,62};
        sf::Color tc  = hov ? sf::Color{72,230,95} : sf::Color{155,160,180};
        drawRect(w, rect.left, rect.top, rect.width, rect.height, bg, brd, hov?2.f:1.f);
        drawText(w, font, btnLabels[i],
                 rect.left+rect.width/2.f, rect.top+rect.height/2.f, 8, tc, true);
    }
}

// ════════════════════════════════════════════════════════════════
// Выбор слота загрузки из главного меню
// ════════════════════════════════════════════════════════════════

void renderLoadSelect(sf::RenderWindow& w, const sf::Font& font,
                      const SaveSlotInfo slotInfos[3],
                      const std::string& notif,
                      sf::Vector2i mousePos)
{
    w.clear({10,12,22});
    drawBgGrid(w);

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;

    drawText(w, font, "ЗАГРУЗИТЬ ИГРУ", cx, cy-170.f, 24, {72,230,95}, true);
    drawText(w, font, "Выберите слот",  cx, cy-138.f,  8, {120,125,155}, true);

    sf::Vertex s1[] = { {{cx-200.f,cy-118.f},{35,38,62}}, {{cx+200.f,cy-118.f},{35,38,62}} };
    w.draw(s1, 2, sf::Lines);

    for (int i = 0; i < 3; i++) {
        float ry = cy - 92.f + i * 56.f;
        const auto& si = slotInfos[i];

        sf::Color rowBg  = si.corrupted ? sf::Color{40,10,10}  : sf::Color{18,20,36};
        sf::Color rowBrd = si.corrupted ? sf::Color{140,35,35} : sf::Color{35,38,62};
        drawRect(w, cx-210.f, ry, 380.f, 44.f, rowBg, rowBrd, si.corrupted ? 1.5f : 1.f);
        drawText(w, font, "Слот "+std::to_string(i+1), cx-202.f, ry+14.f, 8, {100,105,135});

        if (si.corrupted) {
            drawText(w, font, "Файл повреждён", cx-148.f, ry+14.f, 8, {230,80,80});
        } else {
            std::string info = si.exists
                ? si.diff + "  /  " + std::to_string(si.score) + " оч."
                : "Пусто";
            drawText(w, font, info, cx-148.f, ry+14.f, 8,
                     si.exists ? sf::Color{160,165,185} : sf::Color{105,110,140});
        }

        auto ar = pauseSlotActionRect(i);
        bool ahov = ar.contains((float)mousePos.x, (float)mousePos.y);
        if (si.corrupted) {
            drawRect(w, ar.left, ar.top, ar.width, ar.height, {18,20,36}, {50,20,20}, 1.f);
            drawText(w, font, "Ошибка", ar.left+ar.width/2.f, ar.top+ar.height/2.f, 8, {120,40,40}, true);
        } else if (si.exists) {
            drawButton(w, font, ar, "Загрузить", {80,180,230}, false, ahov, 8);
        } else {
            drawRect(w, ar.left, ar.top, ar.width, ar.height, {18,20,36}, {30,33,55}, 1.f);
            drawText(w, font, "Пусто", ar.left+ar.width/2.f, ar.top+ar.height/2.f, 8, {105,110,140}, true);
        }

        if (si.exists) {
            auto dr = pauseSlotDeleteRect(i);
            bool dhov = dr.contains((float)mousePos.x, (float)mousePos.y);
            sf::Color dc = dhov ? sf::Color{230,80,80} : sf::Color{140,60,60};
            drawRect(w, dr.left, dr.top, dr.width, dr.height, {30,12,12}, dc, dhov?2.f:1.f);
            drawText(w, font, "X", dr.left+dr.width/2.f, dr.top+dr.height/2.f, 16, dc, true);
        }
    }

    sf::Vertex s2[] = { {{cx-200.f,cy+68.f},{35,38,62}}, {{cx+200.f,cy+68.f},{35,38,62}} };
    w.draw(s2, 2, sf::Lines);

    auto br = pauseBackRect();
    drawButton(w, font, br, "<  Назад", {160,165,185}, false,
               br.contains((float)mousePos.x,(float)mousePos.y), 8);

    if (!notif.empty()) {
        bool err = notif.find("Нет") != std::string::npos;
        sf::Color c = err ? sf::Color{230,80,80} : sf::Color{72,230,95};
        drawRect(w, cx-145.f, cy+122.f, 290.f, 28.f, {14,16,28,210}, c, 1.f);
        drawText(w, font, notif, cx, cy+126.f, 8, c, true);
    }
}

// ════════════════════════════════════════════════════════════════
// Таблица рекордов
// ════════════════════════════════════════════════════════════════

void renderRecords(sf::RenderWindow& w, const sf::Font& font,
                   const std::vector<Record>& recs)
{
    w.clear({10,12,22});
    drawBgGrid(w);

    float cx = WIN_W / 2.f;
    drawText(w, font, "ТАБЛИЦА РЕКОРДОВ", cx, 38.f, 24, {72,230,95}, true);
    drawText(w, font, "Esc / R — назад",  cx, 80.f,  8, {130,135,165}, true);

    if (recs.empty()) {
        drawText(w, font, "Рекордов пока нет", cx, WIN_H/2.f, 16, {120,125,155}, true);
        return;
    }

    float startY = 114.f, lx = cx - 280.f;
    drawText(w, font, "#",          lx,        startY, 8, {110,115,145});
    drawText(w, font, "Дата",       lx+40.f,   startY, 8, {110,115,145});
    drawText(w, font, "Сложность",  lx+210.f,  startY, 8, {110,115,145});
    drawText(w, font, "Счёт",       lx+410.f,  startY, 8, {110,115,145});
    startY += 18.f;
    sf::Vertex sep[] = { {{lx,startY},{35,38,62}}, {{lx+560.f,startY},{35,38,62}} };
    w.draw(sep, 2, sf::Lines);
    startY += 8.f;

    int show = std::min((int)recs.size(), 10);
    for (int i = 0; i < show; i++) {
        sf::Color col = (i==0 ? sf::Color{228,182,48} :
                         i <3 ? sf::Color{200,205,225} :
                                sf::Color{120,125,155});
        float ry = startY + i*24.f;
        drawText(w, font, std::to_string(i+1),           lx,        ry, 8, col);
        drawText(w, font, recs[i].date,                  lx+40.f,   ry, 8, col);
        drawText(w, font, recs[i].diff,                  lx+210.f,  ry, 8, col);
        if (recs[i].isAI)
            drawText(w, font, "[ИИ]", lx+330.f, ry, 8, {80,180,230});
        drawText(w, font, std::to_string(recs[i].score), lx+410.f,  ry, 8, col);
    }
}
