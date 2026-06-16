#pragma once
#include <deque>
#include <vector>
#include <random>
#include "Types.h"

// ─── Игровая логика ──────────────────────────────────────────────
// Содержит состояние поля, змейки и все правила игры.
// Не знает ничего о графике — только данные и логика.
struct Game {
    std::deque<Pt>                 snake;        // сегменты змейки (голова = [0])
    Dir                            dir     = Dir::RIGHT;
    Dir                            nextDir = Dir::RIGHT;
    std::vector<std::vector<Cell>> grid;         // игровое поле ROWS x COLS
    int                            score   = 0;
    bool                           alive   = true;
    bool                           invCtrl = false; // инверсия управления активна
    int                            invTick = 0;     // оставшихся тиков инверсии
    Diff                           diff;
    std::mt19937                   rng;

    explicit Game(Diff d);

    // Обрабатывает нажатие клавиши (WASD / стрелки)
    void handleKey(sf::Keyboard::Key k);

    // Делает один игровой шаг (вызывается по таймеру)
    void step();

    // Интервал между шагами в секундах (уменьшается со счётом)
    float tickInterval() const;

private:
    void buildObstacles();          // расставляет стены на старте
    bool valid(Pt p) const;         // проверяет, что точка внутри поля
    bool onSnake(Pt p) const;       // проверяет, занята ли точка змейкой
    Pt   rndEmpty();                // случайная пустая клетка
    void spawnOne(Cell type);       // размещает объект, если его ещё нет на поле
};
