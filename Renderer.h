#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Types.h"
#include "Game.h"
#include "Records.h"

// ════════════════════════════════════════════════════════════════
// Прямоугольники кнопок
// Используются как для отрисовки (hover), так и для обработки кликов
// ════════════════════════════════════════════════════════════════

// Кнопки главного меню: 0=Начать, 1=Рекорды, 2=Выход
sf::FloatRect menuButtonRect(int i);

// Кнопки выбора сложности: 0=Безобидный … 3=Сложный
sf::FloatRect diffButtonRect(int i);

// Кнопки экрана конца игры: 0=Заново, 1=В меню
sf::FloatRect gameOverButtonRect(int i);


// ════════════════════════════════════════════════════════════════
// Функции отрисовки экранов
// mousePos передаётся для подсветки кнопок при наведении мыши
// ════════════════════════════════════════════════════════════════

void renderMenu(sf::RenderWindow& w, const sf::Font& font,
                int selected, sf::Vector2i mousePos);

void renderDifficulty(sf::RenderWindow& w, const sf::Font& font,
                      int selected, sf::Vector2i mousePos);

void renderGame(sf::RenderWindow& w, const sf::Font& font,
                const Game& g);

void renderGameOver(sf::RenderWindow& w, const sf::Font& font,
                    const Game& g, const std::vector<Record>& recs,
                    sf::Vector2i mousePos);

void renderRecords(sf::RenderWindow& w, const sf::Font& font,
                   const std::vector<Record>& recs);


// ════════════════════════════════════════════════════════════════
// Низкоуровневые примитивы (используются внутри Renderer.cpp)
// ════════════════════════════════════════════════════════════════

sf::Color lerpColor(sf::Color a, sf::Color b, float t);

void drawRect(sf::RenderWindow& w, float x, float y, float wd, float ht,
              sf::Color fill,
              sf::Color outline = sf::Color::Transparent, float ot = 0.f);

void drawCircle(sf::RenderWindow& w, float cx, float cy, float r, sf::Color fill);

// fromUtf8 нужен для корректного отображения кириллицы
void drawText(sf::RenderWindow& w, const sf::Font& f, const std::string& txt,
              float x, float y, unsigned sz, sf::Color col, bool center = false);
