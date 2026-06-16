#include <SFML/Graphics.hpp>
#include <memory>
#include "Types.h"
#include "Game.h"
#include "AI.h"
#include "Records.h"
#include "Renderer.h"

static const std::string SAVE_FILE = "save.txt";

// Генерация иконки окна 32x32 — S-образная змейка на тёмном фоне
static sf::Image createSnakeIcon() {
    sf::Image img;
    img.create(32, 32, sf::Color(10, 12, 22, 255));
    sf::Color g{72, 230, 95, 255};  // зелёный
    sf::Color bg{10, 12, 22, 255};  // фон (для глаз)

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            bool snake =
                (x>=4 && x<=27 && y>=3  && y<=9)  ||  // верхний сегмент
                (x>=21&& x<=27 && y>=9  && y<=16) ||  // правый поворот вниз
                (x>=4 && x<=27 && y>=10 && y<=16) ||  // средний сегмент
                (x>=4 && x<=10 && y>=16 && y<=23) ||  // левый поворот вниз
                (x>=4 && x<=27 && y>=17 && y<=23);    // нижний сегмент
            if (!snake) continue;
            // Два глаза в правой части верхнего сегмента (голова)
            bool eye = (x>=20 && x<=22 && y>=4 && y<=6) ||
                       (x>=24 && x<=26 && y>=4 && y<=6);
            img.setPixel(x, y, eye ? bg : g);
        }
    }
    return img;
}

int main() {
    static const std::string TITLE = "Змейка";
    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H),
        sf::String::fromUtf8(TITLE.begin(), TITLE.end()),
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(120);

    // Установка иконки окна
    { auto icon = createSnakeIcon(); window.setIcon(32, 32, icon.getPixelsPtr()); }

    // Загрузка шрифта с поддержкой кириллицы
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    // ─── Состояние приложения ────────────────────────────────────
    State state     = State::MENU;
    int   menuSel   = 0;        // выбранный пункт меню (0..4)
    int   diffSel   = 0;        // выбранная сложность (0..3)
    bool  aiPending = false;    // true = следующая игра будет в ИИ-режиме

    std::unique_ptr<Game>   game;
    std::vector<Record>     records   = loadRecords();
    std::vector<Pt>         aiPath;           // путь ИИ для визуализации
    std::string             notif;            // текст уведомления в игре (сохранено/загружено)
    sf::Clock               notifClock;       // таймер уведомления в игре
    std::string             menuNotif;        // текст ошибки в главном меню
    sf::Clock               menuNotifClock;   // таймер ошибки меню
    std::string             pauseNotif;       // уведомление внутри паузы
    sf::Clock               pauseNotifClock;  // таймер уведомления паузы
    sf::Clock               tickClock;

    // ─── Главный цикл ────────────────────────────────────────────
    while (window.isOpen()) {

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        // ── Обработка событий ────────────────────────────────────
        sf::Event ev;
        while (window.pollEvent(ev)) {

            if (ev.type == sf::Event::Closed)
                window.close();

            // ── Нажатие клавиши ──────────────────────────────────
            if (ev.type == sf::Event::KeyPressed) {
                auto k = ev.key.code;

                if (state == State::MENU) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W) menuSel = (menuSel+4)%5;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S) menuSel = (menuSel+1)%5;
                    if (k == sf::Keyboard::Enter) {
                        if      (menuSel == 0) { aiPending = false; state = State::DIFFICULTY; diffSel = 0; }
                        else if (menuSel == 1) { aiPending = true;  state = State::DIFFICULTY; diffSel = 0; }
                        else if (menuSel == 2) {
                            // Загрузить сохранённую игру (показать ошибку если файла нет)
                            auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                            if (loaded->loadFromFile(SAVE_FILE)) {
                                game = std::move(loaded);
                                aiPath.clear();
                                tickClock.restart();
                                state = State::PLAYING;
                            } else {
                                menuNotif = "Нет файла сохранения!";
                                menuNotifClock.restart();
                            }
                        }
                        else if (menuSel == 3) { records = loadRecords(); state = State::RECORDS; }
                        else                    window.close();
                    }
                    if (k == sf::Keyboard::Escape) window.close();
                }

                else if (state == State::DIFFICULTY) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W) diffSel = (diffSel+3)%4;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S) diffSel = (diffSel+1)%4;
                    if (k == sf::Keyboard::Enter) {
                        game = std::make_unique<Game>(static_cast<Diff>(diffSel), aiPending);
                        aiPath.clear();
                        tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                }

                else if (state == State::PLAYING) {
                    if (k == sf::Keyboard::Escape || k == sf::Keyboard::P)
                        state = State::PAUSED;
                    if (k == sf::Keyboard::R) { records = loadRecords(); state = State::RECORDS; }
                    // Сохранение (F5)
                    if (k == sf::Keyboard::F5 && game) {
                        if (game->saveToFile(SAVE_FILE)) {
                            notif = "Игра сохранена!";
                            notifClock.restart();
                        }
                    }
                    // Загрузка (F6)
                    if (k == sf::Keyboard::F6 && game) {
                        auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                        if (loaded->loadFromFile(SAVE_FILE)) {
                            game = std::move(loaded);
                            aiPath.clear();
                            tickClock.restart();
                            notif = "Игра загружена!";
                            notifClock.restart();
                        } else {
                            notif = "Нет файла сохранения!";
                            notifClock.restart();
                        }
                    }
                    if (game) game->handleKey(k); // игнорируется если aiMode
                }

                else if (state == State::PAUSED) {
                    // Продолжить
                    if (k == sf::Keyboard::Escape || k == sf::Keyboard::P) {
                        state = State::PLAYING;
                        tickClock.restart();
                    }
                    // Сохранение (F5) из паузы
                    if (k == sf::Keyboard::F5 && game) {
                        if (game->saveToFile(SAVE_FILE)) {
                            pauseNotif = "Игра сохранена!";
                            pauseNotifClock.restart();
                        }
                    }
                    // Загрузка (F6) из паузы
                    if (k == sf::Keyboard::F6 && game) {
                        auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                        if (loaded->loadFromFile(SAVE_FILE)) {
                            game = std::move(loaded);
                            aiPath.clear();
                            state = State::PLAYING;
                            tickClock.restart();
                        } else {
                            pauseNotif = "Нет файла сохранения!";
                            pauseNotifClock.restart();
                        }
                    }
                }

                else if (state == State::GAME_OVER) {
                    if (k == sf::Keyboard::Enter && game) {
                        game = std::make_unique<Game>(game->diff, game->aiMode);
                        aiPath.clear(); tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                }

                else if (state == State::RECORDS) {
                    if (k == sf::Keyboard::Escape || k == sf::Keyboard::R)
                        state = (game && !game->alive) ? State::GAME_OVER : State::MENU;
                }
            }

            // ── Клик мышью ───────────────────────────────────────
            if (ev.type == sf::Event::MouseButtonPressed &&
                ev.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mp = { ev.mouseButton.x, ev.mouseButton.y };

                if (state == State::MENU) {
                    for (int i = 0; i < 5; i++) {
                        if (!menuButtonRect(i).contains((float)mp.x, (float)mp.y)) continue;
                        menuSel = i;
                        if      (i == 0) { aiPending = false; state = State::DIFFICULTY; diffSel = 0; }
                        else if (i == 1) { aiPending = true;  state = State::DIFFICULTY; diffSel = 0; }
                        else if (i == 2) {
                            auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                            if (loaded->loadFromFile(SAVE_FILE)) {
                                game = std::move(loaded);
                                aiPath.clear(); tickClock.restart();
                                state = State::PLAYING;
                            } else {
                                menuNotif = "Нет файла сохранения!";
                                menuNotifClock.restart();
                            }
                        }
                        else if (i == 3) { records = loadRecords(); state = State::RECORDS; }
                        else              window.close();
                    }
                }

                else if (state == State::DIFFICULTY) {
                    for (int i = 0; i < 4; i++) {
                        if (diffButtonRect(i).contains((float)mp.x, (float)mp.y)) {
                            game = std::make_unique<Game>(static_cast<Diff>(i), aiPending);
                            diffSel = i; aiPath.clear(); tickClock.restart();
                            state = State::PLAYING;
                        }
                    }
                }

                else if (state == State::PAUSED && game) {
                    // 0 — Продолжить
                    if (pauseButtonRect(0).contains((float)mp.x, (float)mp.y)) {
                        state = State::PLAYING;
                        tickClock.restart();
                    }
                    // 1 — Рестарт
                    if (pauseButtonRect(1).contains((float)mp.x, (float)mp.y)) {
                        game = std::make_unique<Game>(game->diff, game->aiMode);
                        aiPath.clear(); tickClock.restart();
                        state = State::PLAYING;
                    }
                    // 2 — Сохранить
                    if (pauseButtonRect(2).contains((float)mp.x, (float)mp.y)) {
                        if (game->saveToFile(SAVE_FILE)) {
                            pauseNotif = "Игра сохранена!";
                            pauseNotifClock.restart();
                        }
                    }
                    // 3 — Загрузить
                    if (pauseButtonRect(3).contains((float)mp.x, (float)mp.y)) {
                        auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                        if (loaded->loadFromFile(SAVE_FILE)) {
                            game = std::move(loaded);
                            aiPath.clear(); tickClock.restart();
                            state = State::PLAYING;
                        } else {
                            pauseNotif = "Нет файла сохранения!";
                            pauseNotifClock.restart();
                        }
                    }
                    // 4 — Главное меню
                    if (pauseButtonRect(4).contains((float)mp.x, (float)mp.y)) {
                        state = State::MENU; menuSel = 0;
                    }
                }

                else if (state == State::GAME_OVER && game) {
                    if (gameOverButtonRect(0).contains((float)mp.x, (float)mp.y)) {
                        game = std::make_unique<Game>(game->diff, game->aiMode);
                        aiPath.clear(); tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (gameOverButtonRect(1).contains((float)mp.x, (float)mp.y)) {
                        state = State::MENU; menuSel = 0;
                    }
                }
            }
        }

        // ── Обновление игры по таймеру ───────────────────────────
        if (state == State::PLAYING && game &&
            tickClock.getElapsedTime().asSeconds() >= game->tickInterval())
        {
            // В ИИ-режиме вычисляем следующий ход через BFS
            if (game->aiMode) {
                auto result    = computeAI(*game);
                game->nextDir  = result.direction;
                aiPath         = result.path;
            }

            game->step();
            tickClock.restart();

            if (!game->alive) {
                saveRecord({ currentDate(), diffName(game->diff), game->score, game->aiMode });
                records = loadRecords();
                aiPath.clear();
                state = State::GAME_OVER;
            }
        }

        // Скрываем игровое уведомление через 2 секунды
        if (!notif.empty() && notifClock.getElapsedTime().asSeconds() > 2.f)
            notif.clear();
        // Скрываем ошибку меню через 2.5 секунды
        if (!menuNotif.empty() && menuNotifClock.getElapsedTime().asSeconds() > 2.5f)
            menuNotif.clear();
        // Скрываем уведомление паузы через 2 секунды
        if (!pauseNotif.empty() && pauseNotifClock.getElapsedTime().asSeconds() > 2.f)
            pauseNotif.clear();

        // ── Отрисовка кадра ──────────────────────────────────────
        window.clear({ 10, 12, 22 });

        switch (state) {
            case State::MENU:
                renderMenu(window, font, menuSel, mousePos, menuNotif);
                break;
            case State::DIFFICULTY:
                renderDifficulty(window, font, diffSel, mousePos, aiPending);
                break;
            case State::PLAYING:
                if (game) renderGame(window, font, *game, aiPath, notif, mousePos);
                break;
            case State::PAUSED:
                if (game) {
                    renderGame(window, font, *game, aiPath, {}, mousePos);
                    renderPause(window, font, *game, pauseNotif, mousePos);
                }
                break;
            case State::GAME_OVER:
                if (game) renderGameOver(window, font, *game, records, mousePos);
                break;
            case State::RECORDS:
                renderRecords(window, font, records);
                break;
        }

        window.display();
    }
    return 0;
}
