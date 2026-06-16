#include <SFML/Graphics.hpp>
#include <memory>
#include "Types.h"
#include "Game.h"
#include "Records.h"
#include "Renderer.h"

int main() {
    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H), L"Змейка",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(120);

    // Загрузка шрифта с поддержкой кириллицы
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    // ─── Состояние приложения ────────────────────────────────────
    State state   = State::MENU;
    int   menuSel = 0;   // выбранный пункт главного меню (0..2)
    int   diffSel = 0;   // выбранная сложность (0..3)

    std::unique_ptr<Game> game;
    std::vector<Record>   records = loadRecords();
    sf::Clock             tickClock;

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
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W) menuSel = (menuSel+2)%3;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S) menuSel = (menuSel+1)%3;
                    if (k == sf::Keyboard::Enter) {
                        if      (menuSel == 0) { state = State::DIFFICULTY; diffSel = 0; }
                        else if (menuSel == 1) { records = loadRecords(); state = State::RECORDS; }
                        else                    window.close();
                    }
                    if (k == sf::Keyboard::Escape) window.close();
                }

                else if (state == State::DIFFICULTY) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W) diffSel = (diffSel+3)%4;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S) diffSel = (diffSel+1)%4;
                    if (k == sf::Keyboard::Enter) {
                        game = std::make_unique<Game>(static_cast<Diff>(diffSel));
                        tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                }

                else if (state == State::PLAYING) {
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                    if (k == sf::Keyboard::R)      { records = loadRecords(); state = State::RECORDS; }
                    if (game) game->handleKey(k);
                }

                else if (state == State::GAME_OVER) {
                    if (k == sf::Keyboard::Enter && game) {
                        game = std::make_unique<Game>(game->diff);
                        tickClock.restart();
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
                    for (int i = 0; i < 3; i++) {
                        if (menuButtonRect(i).contains((float)mp.x, (float)mp.y)) {
                            menuSel = i;
                            // Имитируем нажатие Enter для выбранного пункта
                            if      (i == 0) { state = State::DIFFICULTY; diffSel = 0; }
                            else if (i == 1) { records = loadRecords(); state = State::RECORDS; }
                            else              window.close();
                        }
                    }
                }

                else if (state == State::DIFFICULTY) {
                    for (int i = 0; i < 4; i++) {
                        if (diffButtonRect(i).contains((float)mp.x, (float)mp.y)) {
                            game = std::make_unique<Game>(static_cast<Diff>(i));
                            diffSel = i;
                            tickClock.restart();
                            state = State::PLAYING;
                        }
                    }
                }

                else if (state == State::GAME_OVER && game) {
                    // Кнопка «Играть снова»
                    if (gameOverButtonRect(0).contains((float)mp.x, (float)mp.y)) {
                        game = std::make_unique<Game>(game->diff);
                        tickClock.restart();
                        state = State::PLAYING;
                    }
                    // Кнопка «В меню»
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
            game->step();
            tickClock.restart();
            if (!game->alive) {
                saveRecord({ currentDate(), diffName(game->diff), game->score });
                records = loadRecords();
                state = State::GAME_OVER;
            }
        }

        // ── Отрисовка кадра ──────────────────────────────────────
        window.clear({ 10, 12, 22 });

        switch (state) {
            case State::MENU:
                renderMenu(window, font, menuSel, mousePos);
                break;
            case State::DIFFICULTY:
                renderDifficulty(window, font, diffSel, mousePos);
                break;
            case State::PLAYING:
                if (game) renderGame(window, font, *game);
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
