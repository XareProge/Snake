#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <deque>
#include <vector>
#include <random>
#include <string>
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
    bool                           aiMode  = false; // true = ИИ управляет змейкой
    bool                           invCtrl = false; // инверсия управления активна
    int                            invTick = 0;     // оставшихся тиков инверсии
    Diff                           diff;
    std::mt19937                   rng;

    explicit Game(Diff d, bool ai = false);

    // Обрабатывает нажатие клавиши (WASD / стрелки) — только в режиме игрока
    void handleKey(sf::Keyboard::Key k);

    // Делает один игровой шаг (вызывается по таймеру)
    void step();

    // Интервал между шагами в секундах (уменьшается со счётом)
    float tickInterval() const;

    // Проверяет, что точка внутри поля (публичный — нужен в AI.cpp)
    bool validPt(Pt p) const;

    // Сохраняет состояние игры в текстовый файл
    bool saveToFile(const std::string& path) const;

    // Загружает состояние игры из текстового файла
    // Возвращает false если файл не найден или повреждён
    bool loadFromFile(const std::string& path);

private:
    void buildObstacles();
    bool onSnake(Pt p) const;
    Pt   rndEmpty();
    void spawnOne(Cell type);
};
