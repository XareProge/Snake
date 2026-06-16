#include "AI.h"
#include <queue>
#include <vector>
#include <optional>

// ─── Смещения для 4 направлений ──────────────────────────────────
static const int DX[] = {  0,  0, -1,  1 };
static const int DY[] = { -1,  1,  0,  0 };
static const Dir DS[] = { Dir::UP, Dir::DOWN, Dir::LEFT, Dir::RIGHT };

// Проверяет, можно ли войти в клетку
// Хвост (последний сегмент) не считается препятствием — он сдвинется
static bool isSafe(const Game& g, Pt p) {
    if (!g.validPt(p)) return false;
    if (g.grid[p.y][p.x] == Cell::WALL) return false;
    for (size_t i = 0; i + 1 < g.snake.size(); i++)
        if (g.snake[i] == p) return false;
    return true;
}

// BFS от головы змейки до цели
// Возвращает: направление первого шага + полный путь (или nullopt если пути нет)
static std::optional<AIResult> bfs(const Game& g, Pt target) {
    Pt head = g.snake.front();
    if (head == target) return std::nullopt;

    // parent[y][x] = предыдущая клетка на пути (для восстановления пути)
    std::vector<std::vector<Pt>> parent(ROWS, std::vector<Pt>(COLS, { -1, -1 }));
    std::vector<std::vector<Dir>> firstDir(ROWS, std::vector<Dir>(COLS, Dir::RIGHT));
    std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));

    std::queue<Pt> q;
    visited[head.y][head.x] = true;

    // Добавляем начальные ходы (с проверкой 180° разворота)
    for (int i = 0; i < 4; i++) {
        if (DS[i] == Dir::UP    && g.dir == Dir::DOWN)  continue;
        if (DS[i] == Dir::DOWN  && g.dir == Dir::UP)    continue;
        if (DS[i] == Dir::LEFT  && g.dir == Dir::RIGHT) continue;
        if (DS[i] == Dir::RIGHT && g.dir == Dir::LEFT)  continue;

        Pt next = { head.x + DX[i], head.y + DY[i] };
        if (isSafe(g, next) && !visited[next.y][next.x]) {
            visited[next.y][next.x] = true;
            parent[next.y][next.x]  = head;
            firstDir[next.y][next.x] = DS[i];
            q.push(next);
        }
    }

    // BFS
    while (!q.empty()) {
        Pt cur = q.front(); q.pop();

        if (cur == target) {
            // Восстанавливаем путь: идём от цели обратно к голове
            std::vector<Pt> path;
            Pt step = cur;
            while (!(step == head)) {
                path.push_back(step);
                step = parent[step.y][step.x];
            }
            std::reverse(path.begin(), path.end());
            return AIResult{ firstDir[cur.y][cur.x], path };
        }

        for (int i = 0; i < 4; i++) {
            Pt next = { cur.x + DX[i], cur.y + DY[i] };
            if (isSafe(g, next) && !visited[next.y][next.x]) {
                visited[next.y][next.x] = true;
                parent[next.y][next.x]  = cur;
                firstDir[next.y][next.x] = firstDir[cur.y][cur.x];
                q.push(next);
            }
        }
    }
    return std::nullopt; // путь не найден
}

// ─── Основная функция ИИ ─────────────────────────────────────────
AIResult computeAI(const Game& g) {
    // Находим еду на поле
    Pt food = { -1, -1 };
    for (int y = 0; y < ROWS && food.x == -1; y++)
        for (int x = 0; x < COLS && food.x == -1; x++)
            if (g.grid[y][x] == Cell::FOOD) food = { x, y };

    // 1. Ищем путь к еде
    if (food.x != -1)
        if (auto res = bfs(g, food)) return *res;

    // 2. Режим выживания: ищем путь к хвосту (чтобы не заблокироваться)
    if (auto res = bfs(g, g.snake.back())) return *res;

    // 3. Полная блокировка: идём в любую безопасную сторону
    Pt head = g.snake.front();
    for (int i = 0; i < 4; i++) {
        if (DS[i] == Dir::UP    && g.dir == Dir::DOWN)  continue;
        if (DS[i] == Dir::DOWN  && g.dir == Dir::UP)    continue;
        if (DS[i] == Dir::LEFT  && g.dir == Dir::RIGHT) continue;
        if (DS[i] == Dir::RIGHT && g.dir == Dir::LEFT)  continue;
        Pt next = { head.x + DX[i], head.y + DY[i] };
        if (isSafe(g, next)) return { DS[i], {} };
    }

    return { g.dir, {} }; // крайний случай — держим курс
}
