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

sf::FloatRect menuButtonRect(int i);        // 0=Начать, 1=ИИ, 2=Загрузить, 3=Рекорды, 4=Выход
sf::FloatRect diffButtonRect(int i);        // 0=Безобидный … 3=Сложный
sf::FloatRect gameOverButtonRect(int i);    // 0=Заново, 1=В меню
sf::FloatRect saveButtonRect();             // кнопка «Сохранить» в панели
sf::FloatRect loadButtonRect();             // кнопка «Загрузить» в панели


// ════════════════════════════════════════════════════════════════
// Функции отрисовки экранов
// mousePos передаётся для подсветки кнопок при наведении мыши
// ════════════════════════════════════════════════════════════════

// notif — строка ошибки (например «Нет файла сохранения!»), пустая если нет ошибки
void renderMenu(sf::RenderWindow& w, const sf::Font& font,
                int selected, sf::Vector2i mousePos,
                const std::string& notif = "");

void renderDifficulty(sf::RenderWindow& w, const sf::Font& font,
                      int selected, sf::Vector2i mousePos, bool aiPending);

// path — маршрут ИИ для визуализации (пустой если режим игрока)
// notif — текст уведомления (пустой если нет)
void renderGame(sf::RenderWindow& w, const sf::Font& font,
                const Game& g,
                const std::vector<Pt>& path,
                const std::string& notif,
                sf::Vector2i mousePos);

void renderGameOver(sf::RenderWindow& w, const sf::Font& font,
                    const Game& g, const std::vector<Record>& recs,
                    sf::Vector2i mousePos);

void renderRecords(sf::RenderWindow& w, const sf::Font& font,
                   const std::vector<Record>& recs);


// ════════════════════════════════════════════════════════════════
// Низкоуровневые примитивы
// ════════════════════════════════════════════════════════════════

sf::Color lerpColor(sf::Color a, sf::Color b, float t);

void drawRect(sf::RenderWindow& w, float x, float y, float wd, float ht,
              sf::Color fill,
              sf::Color outline = sf::Color::Transparent, float ot = 0.f);

void drawCircle(sf::RenderWindow& w, float cx, float cy, float r, sf::Color fill);

void drawText(sf::RenderWindow& w, const sf::Font& f, const std::string& txt,
              float x, float y, unsigned sz, sf::Color col, bool center = false);
