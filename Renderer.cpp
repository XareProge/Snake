#include "Renderer.h"
#include <string>
#include <algorithm>

// ════════════════════════════════════════════════════════════════
// Прямоугольники кнопок
// ════════════════════════════════════════════════════════════════

sf::FloatRect menuButtonRect(int i) {
    // 5 кнопок, расположены по центру окна
    float cx = WIN_W / 2.f;
    return { cx - 155.f, 158.f + i * 62.f, 310.f, 50.f };
}

sf::FloatRect diffButtonRect(int i) {
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f;
    return { cx - 200.f, cy - 80.f + i * 68.f, 400.f, 56.f };
}

sf::FloatRect gameOverButtonRect(int i) {
    // 0 = «Играть снова» (слева), 1 = «В меню» (справа)
    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 80.f;
    float bw = 160.f, bx = (i == 0) ? cx - bw - 10.f : cx + 10.f;
    return { bx, cy + 168.f, bw, 46.f };
}

sf::FloatRect saveButtonRect() {
    float ox = (float)(COLS * CELL);
    return { ox + 12.f, WIN_H - 148.f, UI_W - 24.f, 34.f };
}

sf::FloatRect loadButtonRect() {
    float ox = (float)(COLS * CELL);
    return { ox + 12.f, WIN_H - 106.f, UI_W - 24.f, 34.f };
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

// fromUtf8 для корректного отображения кириллицы
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

// Универсальная кнопка с поддержкой hover и selected
static void drawButton(sf::RenderWindow& w, const sf::Font& font,
                        sf::FloatRect rect, const std::string& label,
                        sf::Color activeColor, bool sel, bool hov,
                        int fontSize = 18)
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

// Декоративная сетка для экранов меню
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
// Главное меню (5 пунктов)
// ════════════════════════════════════════════════════════════════

void renderMenu(sf::RenderWindow& w, const sf::Font& font,
                int selected, sf::Vector2i mousePos)
{
    w.clear({ 10, 12, 22 });
    drawBgGrid(w);

    float cx = WIN_W / 2.f;
    // Заголовок над кнопками
    drawText(w, font, "ЗМЕЙКА",           cx, 45.f,  60, { 72, 230, 95 }, true);
    drawText(w, font, "классическая игра", cx, 115.f, 16, { 45, 120, 65 }, true);

    const std::string labels[] = {
        "Начать игру", "ИИ-режим", "Загрузить игру", "Рекорды", "Выход"
    };
    // Цвет ИИ-режима — синеватый, остальные зелёные
    const sf::Color colors[] = {
        {72,230,95}, {80,180,230}, {72,230,95}, {72,230,95}, {72,230,95}
    };
    for (int i = 0; i < 5; i++) {
        auto rect = menuButtonRect(i);
        bool hov  = rect.contains((float)mousePos.x, (float)mousePos.y);
        drawButton(w, font, rect, labels[i], colors[i], selected == i, hov, 20);
    }

    drawText(w, font, "стрелки / WASD — выбор       Enter — подтвердить",
             cx, WIN_H - 30.f, 13, { 45, 50, 75 }, true);
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
    // Заголовок меняется в зависимости от режима
    std::string title = aiPending ? "ИИ-РЕЖИМ: СЛОЖНОСТЬ" : "СЛОЖНОСТЬ";
    sf::Color titleCol = aiPending ? sf::Color{80,180,230} : sf::Color{72,230,95};
    drawText(w, font, title, cx, WIN_H / 2.f - 155.f, 44, titleCol, true);

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
        sf::Color nc = sel || hov ? cols[i]                    : sf::Color{140,145,165};
        sf::Color dc = sel || hov ? sf::Color{160,165,185}     : sf::Color{75,80,105};
        drawText(w, font, names[i], cx, rect.top + 12.f, 20, nc, true);
        drawText(w, font, descs[i], cx, rect.top + 34.f, 13, dc, true);
    }

    drawText(w, font, "Esc — назад", cx, WIN_H - 30.f, 13, {45,50,75}, true);
}

// ════════════════════════════════════════════════════════════════
// Игровой экран
// ════════════════════════════════════════════════════════════════

// Сетка поля
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

// Объект на клетке поля
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

// Путь ИИ — полупрозрачные точки
static void drawAIPath(sf::RenderWindow& w, const std::vector<Pt>& path) {
    int total = (int)path.size();
    for (int i = 0; i < total; i++) {
        // Интенсивность уменьшается к концу пути
        float t   = 1.f - (float)i / (float)(total + 1);
        uint8_t a = (uint8_t)(180 * t);
        float cx  = path[i].x * CELL + CELL / 2.f;
        float cy  = path[i].y * CELL + CELL / 2.f;
        drawCircle(w, cx, cy, 3.5f, {80, 180, 230, a});
    }
}

// Змейка
static void drawSnake(sf::RenderWindow& w, const Game& g) {
    float pad = 3.f, sz = CELL - pad*2;

    // Тело с хвоста к голове
    for (size_t i = g.snake.size(); i-- > 0; ) {
        float t = (float)i / (float)g.snake.size();
        sf::Color col = lerpColor({42,175,68}, {18,80,35}, t*0.7f);
        float x = g.snake[i].x*CELL, y = g.snake[i].y*CELL;
        drawRect(w, x+pad, y+pad, sz, sz, col, {15,65,28}, 1.f);
        drawRect(w, x+pad+2, y+pad+2, sz*0.45f, 2.f, {140,255,150,100});
    }

    // Голова: жёлтая при инверсии, голубая в ИИ-режиме, зелёная в норме
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

// Правая панель с кнопками Сохранить/Загрузить
static void drawUI(sf::RenderWindow& w, const sf::Font& font,
                   const Game& g, sf::Vector2i mousePos)
{
    float ox = (float)(COLS * CELL);
    float cx = ox + UI_W / 2.f;

    drawRect(w, ox, 0, (float)UI_W, (float)WIN_H, {14,16,28});
    sf::Vertex sep[] = { {{ox,0},{35,38,62}}, {{ox,(float)WIN_H},{35,38,62}} };
    w.draw(sep, 2, sf::Lines);

    // Заголовок + бейджи режима
    drawText(w, font, "ЗМЕЙКА", cx, 18.f, 44, {72,230,95}, true);
    if (g.aiMode) {
        drawRect(w, ox+12.f, 68.f, UI_W-24.f, 22.f, {15,40,65});
        drawText(w, font, "ИИ-РЕЖИМ", cx, 71.f, 14, {80,180,230}, true);
    } else {
        sf::Color dc = (g.diff==Diff::HARD   ? sf::Color{230,80,80}  :
                        g.diff==Diff::MEDIUM  ? sf::Color{228,182,48} :
                        g.diff==Diff::EASY    ? sf::Color{120,200,90} :
                                               sf::Color{72,230,95});
        drawText(w, font, diffName(g.diff), cx, 70.f, 14, dc, true);
    }

    // Счёт
    drawRect(w, ox+12.f, 98.f, UI_W-24.f, 78.f, {20,22,40});
    drawText(w, font, "СЧЁТ",                  cx, 106.f, 16, {95,100,125}, true);
    drawText(w, font, std::to_string(g.score), cx, 126.f, 38, {255,255,255}, true);

    // Длина
    drawRect(w, ox+12.f, 190.f, UI_W-24.f, 68.f, {20,22,40});
    drawText(w, font, "ДЛИНА",                         cx, 198.f, 16, {95,100,125}, true);
    drawText(w, font, std::to_string(g.snake.size()),  cx, 218.f, 28, {200,205,225}, true);

    // Инверсия
    if (g.invCtrl) {
        drawRect(w, ox+12.f, 270.f, UI_W-24.f, 36.f, {60,45,10});
        drawText(w, font, "! ИНВЕРСИЯ !", cx, 280.f, 13, {228,182,48}, true);
    }

    // Легенда (для сложностей с особыми объектами)
    if (g.diff >= Diff::MEDIUM) {
        float ly = 322.f;
        drawText(w, font, "ОБЪЕКТЫ", cx, ly, 12, {55,60,85}, true);
        ly += 20.f;
        drawCircle(w, ox+22.f, ly+6.f, 5.f, {168,48,230});
        drawText(w, font, "яд  -15 очков", ox+34.f, ly, 11, {160,165,185});
        if (g.diff == Diff::HARD) {
            ly += 22.f;
            sf::CircleShape ring(5.f); ring.setOrigin(5.f,5.f);
            ring.setPosition(ox+22.f, ly+6.f);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor({45,205,225}); ring.setOutlineThickness(2.f);
            w.draw(ring);
            drawText(w, font, "телепорт", ox+34.f, ly, 11, {160,165,185});
            ly += 22.f;
            drawCircle(w, ox+22.f, ly+6.f, 5.f, {228,182,48});
            drawText(w, font, "инверсия", ox+34.f, ly, 11, {160,165,185});
        }
    }

    // ── Кнопки Сохранить / Загрузить ──────────────────────────
    {
        auto sr = saveButtonRect();
        bool hov = sr.contains((float)mousePos.x, (float)mousePos.y);
        drawButton(w, font, sr, "Сохранить  [S]", {72,230,95}, false, hov, 14);
    }
    {
        auto lr = loadButtonRect();
        bool hov = lr.contains((float)mousePos.x, (float)mousePos.y);
        drawButton(w, font, lr, "Загрузить  [L]", {72,230,95}, false, hov, 14);
    }

    // Подсказки
    float hy = WIN_H - 60.f;
    drawText(w, font, "WASD/стрелки  Esc-меню  R-рекорды",
             cx, hy, 11, {55,60,85}, true);
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

    // Путь ИИ рисуем под змейкой
    if (!path.empty()) drawAIPath(w, path);

    drawSnake(w, g);
    drawUI(w, font, g, mousePos);

    // Уведомление (сохранено/загружено)
    if (!notif.empty()) {
        float cx = (float)(COLS * CELL) / 2.f;
        float cy = WIN_H / 2.f;
        drawRect(w, cx-130.f, cy-22.f, 260.f, 44.f, {14,16,28,220}, {72,230,95}, 2.f);
        drawText(w, font, notif, cx, cy, 17, {72,230,95}, true);
    }
}

// ════════════════════════════════════════════════════════════════
// Конец игры
// ════════════════════════════════════════════════════════════════

void renderGameOver(sf::RenderWindow& w, const sf::Font& font,
                    const Game& g, const std::vector<Record>& recs,
                    sf::Vector2i mousePos)
{
    // Поле под оверлеем (без пути — игра закончена)
    renderGame(w, font, g, {}, {}, mousePos);

    drawRect(w, 0, 0, (float)WIN_W, (float)WIN_H, {0,0,0,170});

    float cx = WIN_W / 2.f, cy = WIN_H / 2.f - 80.f;
    drawRect(w, cx-205.f, cy-28.f, 410.f, 240.f, {14,16,28}, {50,38,62}, 2.f);

    drawText(w, font, "КОНЕЦ ИГРЫ",                     cx, cy,       42, {230,80,80},  true);
    drawText(w, font, "Счёт: "+std::to_string(g.score), cx, cy+52.f,  24, {200,205,225},true);

    // Топ-3
    drawText(w, font, "Лучшие результаты:", cx, cy+92.f, 13, {55,60,85}, true);
    int show = std::min((int)recs.size(), 3);
    for (int i = 0; i < show; i++) {
        sf::Color col = (i==0 ? sf::Color{228,182,48} : sf::Color{160,165,185});
        std::string line = std::to_string(i+1)+".  "+recs[i].diff
                          +"   "+std::to_string(recs[i].score);
        drawText(w, font, line, cx, cy+112.f+i*20.f, 12, col, true);
    }

    // Кнопки с мышью
    const std::string btnLabels[] = { "Играть снова", "В меню" };
    for (int i = 0; i < 2; i++) {
        auto rect = gameOverButtonRect(i);
        bool hov  = rect.contains((float)mousePos.x, (float)mousePos.y);
        sf::Color bg  = hov ? sf::Color{25,55,35}  : sf::Color{18,20,36};
        sf::Color brd = hov ? sf::Color{72,230,95} : sf::Color{35,38,62};
        sf::Color tc  = hov ? sf::Color{72,230,95} : sf::Color{155,160,180};
        drawRect(w, rect.left, rect.top, rect.width, rect.height, bg, brd, hov?2.f:1.f);
        drawText(w, font, btnLabels[i],
                 rect.left+rect.width/2.f, rect.top+rect.height/2.f, 16, tc, true);
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
    drawText(w, font, "ТАБЛИЦА РЕКОРДОВ", cx, 40.f, 36, {72,230,95}, true);
    drawText(w, font, "Esc / R — назад",  cx, 88.f, 14, {55,60,85},  true);

    if (recs.empty()) {
        drawText(w, font, "Рекордов пока нет", cx, WIN_H/2.f, 22, {55,60,85}, true);
        return;
    }

    float startY = 125.f, lx = cx - 280.f;
    drawText(w, font, "#",          lx,        startY, 14, {55,60,85});
    drawText(w, font, "Дата",       lx+40.f,   startY, 14, {55,60,85});
    drawText(w, font, "Сложность",  lx+200.f,  startY, 14, {55,60,85});
    drawText(w, font, "Счёт",       lx+370.f,  startY, 14, {55,60,85});
    startY += 22.f;
    sf::Vertex sep[] = { {{lx,startY},{35,38,62}}, {{lx+560.f,startY},{35,38,62}} };
    w.draw(sep, 2, sf::Lines);
    startY += 10.f;

    int show = std::min((int)recs.size(), 13);
    for (int i = 0; i < show; i++) {
        sf::Color col = (i==0 ? sf::Color{228,182,48} :
                         i <3 ? sf::Color{200,205,225} :
                                sf::Color{95,100,125});
        float ry = startY + i*26.f;
        drawText(w, font, std::to_string(i+1),          lx,        ry, 14, col);
        drawText(w, font, recs[i].date,                 lx+40.f,   ry, 14, col);
        drawText(w, font, recs[i].diff,                 lx+200.f,  ry, 14, col);
        drawText(w, font, std::to_string(recs[i].score),lx+370.f,  ry, 14, col);
    }
}
