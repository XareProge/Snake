#include <SFML/Graphics.hpp>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Types.h"
#include "Game.h"
#include "AI.h"
#include "Records.h"
#include "Renderer.h"

// ─── Пути к файлам ───────────────────────────────────────────────
static const std::string SAVES_DIR     = "saves";
static const std::string SAVE_FILES[3] = {
    "saves/save1.txt", "saves/save2.txt", "saves/save3.txt"
};

// ─── Иконка окна ─────────────────────────────────────────────────
static sf::Image createSnakeIcon() {
    sf::Image img;
    img.create(32, 32, sf::Color(10, 12, 22, 255));
    sf::Color g{72, 230, 95, 255};
    sf::Color bg{10, 12, 22, 255};
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            bool snake =
                (x>=4 && x<=27 && y>=3  && y<=9)  ||
                (x>=21&& x<=27 && y>=9  && y<=16) ||
                (x>=4 && x<=27 && y>=10 && y<=16) ||
                (x>=4 && x<=10 && y>=16 && y<=23) ||
                (x>=4 && x<=27 && y>=17 && y<=23);
            if (!snake) continue;
            bool eye = (x>=20 && x<=22 && y>=4 && y<=6) ||
                       (x>=24 && x<=26 && y>=4 && y<=6);
            img.setPixel(x, y, eye ? bg : g);
        }
    }
    return img;
}

// ─── Быстрое чтение информации о слоте (без загрузки всей игры) ──
static SaveSlotInfo readSlotInfo(const std::string& path) {
    SaveSlotInfo info;
    std::ifstream f(path);
    if (!f.is_open()) return info;
    info.exists = true;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("diff:", 0) == 0)
            info.diff = diffName(static_cast<Diff>(std::stoi(line.substr(5))));
        else if (line.rfind("score:", 0) == 0)
            info.score = std::stoi(line.substr(6));
    }
    return info;
}

int main() {
    std::filesystem::create_directories(SAVES_DIR);

    static const std::string TITLE = "Змейка";
    sf::RenderWindow window(
        sf::VideoMode(WIN_W, WIN_H),
        sf::String::fromUtf8(TITLE.begin(), TITLE.end()),
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(120);
    { auto icon = createSnakeIcon(); window.setIcon(32, 32, icon.getPixelsPtr()); }

    sf::Font font;
    if (!font.loadFromFile("assets/PressStart2P.ttf"))
        if (!font.loadFromFile("C:/Windows/Fonts/consola.ttf"))
            font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    // ─── Состояние приложения ────────────────────────────────────
    State      state      = State::MENU;
    int        menuSel    = 0;
    int        diffSel    = 0;
    bool       aiPending  = false;
    PausePanel pausePanel = PausePanel::NONE;

    std::unique_ptr<Game> game;
    std::vector<Record>   records = loadRecords();
    std::vector<Pt>       aiPath;

    std::string notif,     menuNotif,     pauseNotif,     loadNotif;
    sf::Clock   notifClk,  menuNotifClk,  pauseNotifClk,  loadNotifClk;

    bool      countdownActive = false;
    sf::Clock countdownClk;
    sf::Clock tickClock;

    // ─── Главный цикл ────────────────────────────────────────────
    while (window.isOpen()) {

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        // Актуальные данные слотов (3 файла, читается быстро)
        SaveSlotInfo slots[3];
        for (int i = 0; i < 3; i++) slots[i] = readSlotInfo(SAVE_FILES[i]);

        // ── Обработка событий ────────────────────────────────────
        sf::Event ev;
        while (window.pollEvent(ev)) {

            if (ev.type == sf::Event::Closed)
                window.close();

            // ── Клавиши ──────────────────────────────────────────
            if (ev.type == sf::Event::KeyPressed) {
                auto k = ev.key.code;

                if (state == State::MENU) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W) menuSel = (menuSel+4)%5;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S) menuSel = (menuSel+1)%5;
                    if (k == sf::Keyboard::Enter) {
                        if      (menuSel == 0) { aiPending = false; state = State::DIFFICULTY; diffSel = 0; }
                        else if (menuSel == 1) { aiPending = true;  state = State::DIFFICULTY; diffSel = 0; }
                        else if (menuSel == 2) { state = State::LOAD_SELECT; }
                        else if (menuSel == 3) { records = loadRecords(); state = State::RECORDS; }
                        else                    window.close();
                    }
                    if (k == sf::Keyboard::Escape) window.close();
                }

                else if (state == State::LOAD_SELECT) {
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                    // Удаление слота клавишами Delete/1-3 не добавляем — только мышь
                }

                else if (state == State::DIFFICULTY) {
                    if (k == sf::Keyboard::Up   || k == sf::Keyboard::W) diffSel = (diffSel+3)%4;
                    if (k == sf::Keyboard::Down || k == sf::Keyboard::S) diffSel = (diffSel+1)%4;
                    if (k == sf::Keyboard::Enter) {
                        game = std::make_unique<Game>(static_cast<Diff>(diffSel), aiPending);
                        aiPath.clear(); tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (k == sf::Keyboard::Escape) { state = State::MENU; menuSel = 0; }
                }

                else if (state == State::PLAYING) {
                    if (k == sf::Keyboard::Escape || k == sf::Keyboard::P) {
                        state = State::PAUSED; pausePanel = PausePanel::NONE;
                    }
                    if (k == sf::Keyboard::R) { records = loadRecords(); state = State::RECORDS; }
                    // F5 → пауза + панель сохранения
                    if (k == sf::Keyboard::F5) { state = State::PAUSED; pausePanel = PausePanel::SAVING; }
                    // F6 → пауза + панель загрузки
                    if (k == sf::Keyboard::F6) { state = State::PAUSED; pausePanel = PausePanel::LOADING; }
                    if (game) game->handleKey(k);
                }

                else if (state == State::PAUSED) {
                    if (k == sf::Keyboard::F5) { pausePanel = PausePanel::SAVING; }
                    if (k == sf::Keyboard::F6) { pausePanel = PausePanel::LOADING; }
                    if (k == sf::Keyboard::Escape || k == sf::Keyboard::P) {
                        if (pausePanel != PausePanel::NONE)
                            pausePanel = PausePanel::NONE;   // вернуться к кнопкам паузы
                        else {
                            // Продолжить с обратным отсчётом
                            countdownActive = true;
                            countdownClk.restart();
                            tickClock.restart();
                            state = State::PLAYING;
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

            // ── Клики мышью ──────────────────────────────────────
            if (ev.type == sf::Event::MouseButtonPressed &&
                ev.mouseButton.button == sf::Mouse::Left)
            {
                sf::Vector2i mp = { ev.mouseButton.x, ev.mouseButton.y };

                if (state == State::MENU) {
                    for (int i = 0; i < 5; i++) {
                        if (!menuButtonRect(i).contains((float)mp.x,(float)mp.y)) continue;
                        menuSel = i;
                        if      (i == 0) { aiPending = false; state = State::DIFFICULTY; diffSel = 0; }
                        else if (i == 1) { aiPending = true;  state = State::DIFFICULTY; diffSel = 0; }
                        else if (i == 2) { state = State::LOAD_SELECT; }
                        else if (i == 3) { records = loadRecords(); state = State::RECORDS; }
                        else              window.close();
                    }
                }

                else if (state == State::LOAD_SELECT) {
                    for (int i = 0; i < 3; i++) {
                        // Загрузить слот
                        if (slots[i].exists &&
                            pauseSlotActionRect(i).contains((float)mp.x,(float)mp.y))
                        {
                            auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                            if (loaded->loadFromFile(SAVE_FILES[i])) {
                                game = std::move(loaded);
                                aiPath.clear();
                                countdownActive = true;
                                countdownClk.restart();
                                tickClock.restart();
                                state = State::PLAYING;
                            } else {
                                loadNotif = "Ошибка загрузки слота!";
                                loadNotifClk.restart();
                            }
                        }
                        // Удалить слот
                        if (slots[i].exists &&
                            pauseSlotDeleteRect(i).contains((float)mp.x,(float)mp.y))
                        {
                            std::filesystem::remove(SAVE_FILES[i]);
                        }
                    }
                    if (pauseBackRect().contains((float)mp.x,(float)mp.y)) {
                        state = State::MENU; menuSel = 0;
                    }
                }

                else if (state == State::DIFFICULTY) {
                    for (int i = 0; i < 4; i++) {
                        if (diffButtonRect(i).contains((float)mp.x,(float)mp.y)) {
                            game = std::make_unique<Game>(static_cast<Diff>(i), aiPending);
                            diffSel = i; aiPath.clear(); tickClock.restart();
                            state = State::PLAYING;
                        }
                    }
                }

                else if (state == State::PAUSED && game) {
                    if (pausePanel == PausePanel::NONE) {
                        // Кнопки главного меню паузы
                        if (pauseButtonRect(0).contains((float)mp.x,(float)mp.y)) {
                            // Продолжить
                            countdownActive = true;
                            countdownClk.restart();
                            tickClock.restart();
                            state = State::PLAYING;
                        }
                        if (pauseButtonRect(1).contains((float)mp.x,(float)mp.y)) {
                            // Рестарт (без обратного отсчёта)
                            game = std::make_unique<Game>(game->diff, game->aiMode);
                            aiPath.clear(); tickClock.restart();
                            state = State::PLAYING;
                        }
                        if (pauseButtonRect(2).contains((float)mp.x,(float)mp.y))
                            pausePanel = PausePanel::SAVING;
                        if (pauseButtonRect(3).contains((float)mp.x,(float)mp.y))
                            pausePanel = PausePanel::LOADING;
                        if (pauseButtonRect(4).contains((float)mp.x,(float)mp.y)) {
                            state = State::MENU; menuSel = 0;
                        }
                    } else {
                        // Кнопки слот-панели
                        for (int i = 0; i < 3; i++) {
                            auto ar = pauseSlotActionRect(i);
                            if (ar.contains((float)mp.x,(float)mp.y)) {
                                if (pausePanel == PausePanel::SAVING) {
                                    // Сохранить в слот i
                                    if (game->saveToFile(SAVE_FILES[i])) {
                                        pauseNotif = "Слот " + std::to_string(i+1) + " сохранён!";
                                        pauseNotifClk.restart();
                                    }
                                } else {
                                    // Загрузить из слота i
                                    if (slots[i].exists) {
                                        auto loaded = std::make_unique<Game>(Diff::HARMLESS);
                                        if (loaded->loadFromFile(SAVE_FILES[i])) {
                                            game = std::move(loaded);
                                            aiPath.clear();
                                            pausePanel = PausePanel::NONE;
                                            countdownActive = true;
                                            countdownClk.restart();
                                            tickClock.restart();
                                            state = State::PLAYING;
                                        }
                                    }
                                }
                            }
                            // Удалить слот
                            if (slots[i].exists &&
                                pauseSlotDeleteRect(i).contains((float)mp.x,(float)mp.y))
                            {
                                std::filesystem::remove(SAVE_FILES[i]);
                                if (pauseNotif.find("сохранён") != std::string::npos)
                                    pauseNotif.clear();
                            }
                        }
                        if (pauseBackRect().contains((float)mp.x,(float)mp.y))
                            pausePanel = PausePanel::NONE;
                    }
                }

                else if (state == State::GAME_OVER && game) {
                    if (gameOverButtonRect(0).contains((float)mp.x,(float)mp.y)) {
                        game = std::make_unique<Game>(game->diff, game->aiMode);
                        aiPath.clear(); tickClock.restart();
                        state = State::PLAYING;
                    }
                    if (gameOverButtonRect(1).contains((float)mp.x,(float)mp.y)) {
                        state = State::MENU; menuSel = 0;
                    }
                }
            }
        }

        // ── Обратный отсчёт ──────────────────────────────────────
        if (countdownActive && countdownClk.getElapsedTime().asSeconds() >= 3.f) {
            countdownActive = false;
            tickClock.restart();
        }

        // ── Тик игры (только когда нет отсчёта) ──────────────────
        if (state == State::PLAYING && game && !countdownActive &&
            tickClock.getElapsedTime().asSeconds() >= game->tickInterval())
        {
            if (game->aiMode) {
                auto result   = computeAI(*game);
                game->nextDir = result.direction;
                aiPath        = result.path;
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

        // ── Таймеры уведомлений ───────────────────────────────────
        if (!notif.empty()      && notifClk.getElapsedTime().asSeconds()      > 2.f)  notif.clear();
        if (!menuNotif.empty()  && menuNotifClk.getElapsedTime().asSeconds()  > 2.5f) menuNotif.clear();
        if (!pauseNotif.empty() && pauseNotifClk.getElapsedTime().asSeconds() > 2.f)  pauseNotif.clear();
        if (!loadNotif.empty()  && loadNotifClk.getElapsedTime().asSeconds()  > 2.f)  loadNotif.clear();

        // ── Отрисовка ─────────────────────────────────────────────
        window.clear({10, 12, 22});

        switch (state) {
            case State::MENU:
                renderMenu(window, font, menuSel, mousePos, menuNotif);
                break;
            case State::LOAD_SELECT:
                renderLoadSelect(window, font, slots, loadNotif, mousePos);
                break;
            case State::DIFFICULTY:
                renderDifficulty(window, font, diffSel, mousePos, aiPending);
                break;
            case State::PLAYING:
                if (game) {
                    renderGame(window, font, *game, aiPath, notif, mousePos);
                    if (countdownActive)
                        renderCountdown(window, font, countdownClk.getElapsedTime().asSeconds());
                }
                break;
            case State::PAUSED:
                if (game) {
                    renderGame(window, font, *game, aiPath, {}, mousePos);
                    renderPause(window, font, *game, pauseNotif, pausePanel, slots, mousePos);
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
