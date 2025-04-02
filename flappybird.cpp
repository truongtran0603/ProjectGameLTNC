#include "flappybird.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>

FlappyBird::FlappyBird() {
    if (!initSDL() || !setupWindowAndRenderer() || !loadFontResources() ||
        !loadTextureResources() || !loadAudioResources()) {
        cleanup();
        return;
    }
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    running = true;
    highScore = loadHighScore();
    setupMenu();
    if (lobbyMusic) {
        Mix_VolumeMusic(MIX_MAX_VOLUME);
        Mix_PlayMusic(lobbyMusic, -1);
    }
}

FlappyBird::~FlappyBird() {
    saveHighScore();
    cleanup();
}

void FlappyBird::cleanup() {
    for (auto& [_, texture] : textCache) SDL_DestroyTexture(texture);
    for (auto& [_, texture] : scoreTextCache) SDL_DestroyTexture(texture);
    for (auto& [_, texture] : infoTextCache) SDL_DestroyTexture(texture);
    SDL_DestroyTexture(player1Texture);
    SDL_DestroyTexture(player2Texture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(logoTexture);
    SDL_DestroyTexture(pipeTexture);
    SDL_DestroyTexture(speakerOnTexture);
    SDL_DestroyTexture(speakerOffTexture);
    TTF_CloseFont(font);
    TTF_CloseFont(scoreFont);
    TTF_CloseFont(infoFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_FreeChunk(sfx_die);
    Mix_FreeChunk(sfx_hit);
    Mix_FreeChunk(sfx_point);
    Mix_FreeChunk(sfx_wing);
    Mix_FreeChunk(clickSound);
    Mix_FreeChunk(sfx_falling);
    Mix_FreeMusic(lobbyMusic);
    Mix_FreeMusic(playingMusic);
    delete soundButton;
    Mix_CloseAudio();
    Mix_Quit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

SDL_Texture* FlappyBird::createTextTexture(const std::string& text, TTF_Font* targetFont,
                                         std::unordered_map<std::string, SDL_Texture*>& cache,
                                         bool isRed) {
    const std::string key = text + (isRed ? "_red" : "");
    if (auto it = cache.find(key); it != cache.end()) return it->second;

    const SDL_Color color = isRed ? SDL_Color{255, 0, 0, 255} : SDL_Color{255, 255, 0, 255};
    const SDL_Color outlineColor = {0, 0, 0, 255};

    TTF_SetFontOutline(targetFont, 2);
    SDL_Surface* outlineSurface = TTF_RenderText_Blended(targetFont, text.c_str(), outlineColor);
    TTF_SetFontOutline(targetFont, 0);
    SDL_Surface* textSurface = TTF_RenderText_Blended(targetFont, text.c_str(), color);

    if (!outlineSurface || !textSurface) {
        SDL_FreeSurface(outlineSurface);
        SDL_FreeSurface(textSurface);
        return nullptr;
    }

    SDL_Surface* combinedSurface = SDL_CreateRGBSurface(0, outlineSurface->w, outlineSurface->h, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    SDL_BlitSurface(outlineSurface, nullptr, combinedSurface, nullptr);
    SDL_Rect offset = {(combinedSurface->w - textSurface->w) / 2,
                      (combinedSurface->h - textSurface->h) / 2, 0, 0};
    SDL_BlitSurface(textSurface, nullptr, combinedSurface, &offset);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, combinedSurface);
    if (texture) cache[key] = texture;

    SDL_FreeSurface(outlineSurface);
    SDL_FreeSurface(textSurface);
    SDL_FreeSurface(combinedSurface);
    return texture;
}

void FlappyBird::renderText(const std::string& text, int x, int y, bool center,
                           bool isScore, bool isInfo, bool isRed) {
    TTF_Font* targetFont = isScore ? scoreFont : (isInfo ? infoFont : font);
    auto& targetCache = isScore ? scoreTextCache : (isInfo ? infoTextCache : textCache);
    SDL_Texture* texture = createTextTexture(text, targetFont, targetCache, isRed);
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    SDL_Rect rect = {center ? x - w / 2 : x, y, w, h};
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
}

bool FlappyBird::isPointInRect(int x, int y, const SDL_Rect& rect) const {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
}

SDL_Texture* FlappyBird::loadTexture(const char* file) {
    SDL_Surface* surface = IMG_Load(file);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

bool FlappyBird::initSDL() {
    return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) >= 0 &&
           TTF_Init() >= 0 &&
           IMG_Init(IMG_INIT_PNG) != 0 &&
           Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) >= 0;
}

bool FlappyBird::setupWindowAndRenderer() {
    window = SDL_CreateWindow("Flappy Bird", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }
    return true;
}

bool FlappyBird::loadFontResources() {
    font = TTF_OpenFont("font/SVN-New Athletic M54.ttf", 48);
    scoreFont = TTF_OpenFont("font/SVN-New Athletic M54.ttf", 24);
    infoFont = TTF_OpenFont("font/SVN-New Athletic M54.ttf", 24);
    return font && scoreFont && infoFont;
}

bool FlappyBird::loadTextureResources() {
    player1Texture = loadTexture("picture/player1.png");
    player2Texture = loadTexture("picture/player2.png");
    backgroundTexture = loadTexture("picture/background_and_road.png");
    logoTexture = loadTexture("picture/logo.png");
    pipeTexture = loadTexture("picture/pipe.png");
    speakerOnTexture = loadTexture("picture/speaker_on.png");
    speakerOffTexture = loadTexture("picture/speaker_off.png");
    return player1Texture && player2Texture && backgroundTexture &&
           logoTexture && pipeTexture && speakerOnTexture && speakerOffTexture;
}

bool FlappyBird::loadAudioResources() {
    sfx_die = Mix_LoadWAV("sound/sfx_die.wav");
    sfx_hit = Mix_LoadWAV("sound/sfx_hit.wav");
    sfx_point = Mix_LoadWAV("sound/sfx_point.wav");
    sfx_wing = Mix_LoadWAV("sound/sfx_wing.wav");
    clickSound = Mix_LoadWAV("sound/clicking.wav");
    sfx_falling = Mix_LoadWAV("sound/falling.wav");
    lobbyMusic = Mix_LoadMUS("sound/sound at lobby.mp3");
    playingMusic = Mix_LoadMUS("sound/sound effect while playing.mp3");
    return sfx_die && sfx_hit && sfx_point && sfx_wing &&
           clickSound && sfx_falling && lobbyMusic && playingMusic;
}

void FlappyBird::setupMenu() {
    buttons.clear();
    const int centerX = Constants::WINDOW_WIDTH / 2 - 100;
    buttons.emplace_back(centerX, Constants::WINDOW_HEIGHT / 2 - 60, 200, 30, "1 Player");
    buttons.emplace_back(centerX, Constants::WINDOW_HEIGHT / 2, 200, 30, "2 Players");
    buttons.emplace_back(centerX, Constants::WINDOW_HEIGHT / 2 + 60, 200, 30, "Information");
    soundButton = new Button(Constants::WINDOW_WIDTH - 50, 10, 40, 40, "");
}

void FlappyBird::handleButtonHover(int x, int y, Button& button, float scaleFactor) {
    SDL_Texture* texture = createTextTexture(button.text, font, textCache, button.isRed);
    int w, h;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    SDL_Rect textRect = {button.rect.x + (button.rect.w - w) / 2,
                       button.rect.y + (button.rect.h - h) / 2, w, h};

    if (isPointInRect(x, y, textRect)) {
        if (!button.hovered) {
            button.hovered = true;
            button.rect.w = static_cast<int>(button.originalW * scaleFactor);
            button.rect.h = static_cast<int>(button.originalH * scaleFactor);
            button.rect.x -= (button.rect.w - button.originalW) / 2;
            button.rect.y -= (button.rect.h - button.originalH) / 2;
        }
    } else if (button.hovered) {
        button.hovered = false;
        button.rect.w = button.originalW;
        button.rect.h = button.originalH;
        button.rect.x += (static_cast<int>(button.originalW * scaleFactor) - button.originalW) / 2;
        button.rect.y += (static_cast<int>(button.originalH * scaleFactor) - button.originalH) / 2;
    }
}

void FlappyBird::updateBirdPhysics() {
    for (auto& bird : birds) {
        bird.velocity += 0.4f;
        bird.y += bird.velocity;
        //frameCounter++;
        //if (frameCounter >= 30) {
            //std::cout << "Bird position - Y: " << bird.y << ", Velocity: " << bird.velocity << std::endl;
            //frameCounter = 0; // Đặt lại đếm sau khi in
        //}

        if (bird.y + Constants::BIRD_SIZE >= Constants::WINDOW_HEIGHT) {
            if (bird.collided) bird.alive = false;
            else if (bird.alive) {
                Mix_PlayChannel(-1, sfx_falling, 0);
                bird.alive = false;
            }
        }
        if (bird.y < 0) {
            bird.y = 0;
            bird.velocity = 0;
        }
    }
}

void FlappyBird::updatePipes() {
    for (auto it = pipes.begin(); it != pipes.end();) {
        it->x -= 2;
        for (size_t i = 0; i < birds.size(); i++) {
            if (!birds[i].alive) continue;
            int birdX = Constants::WINDOW_WIDTH / 4 + static_cast<int>(i * Constants::BIRD_SPACING);
            SDL_Rect birdRect = {birdX, static_cast<int>(birds[i].y), Constants::BIRD_SIZE, Constants::BIRD_SIZE};
            SDL_Rect topPipe = {it->x, 0, Constants::PIPE_WIDTH, it->gapY - Constants::PIPE_GAP / 2};
            SDL_Rect bottomPipe = {it->x, it->gapY + Constants::PIPE_GAP / 2, Constants::PIPE_WIDTH,
                                 Constants::WINDOW_HEIGHT - (it->gapY + Constants::PIPE_GAP / 2)};

            if ((SDL_HasIntersection(&birdRect, &topPipe) || SDL_HasIntersection(&birdRect, &bottomPipe)) &&
                !birds[i].collided) {
                birds[i].collided = true;
                Mix_PlayChannel(-1, sfx_hit, 0);
            }
        }

        if (it->x + Constants::PIPE_WIDTH < Constants::WINDOW_WIDTH / 4 &&
            it->x + Constants::PIPE_WIDTH >= Constants::WINDOW_WIDTH / 4 - 2) {
            for (auto& bird : birds) {
                if (bird.alive) {
                    bird.score++;
                    Mix_PlayChannel(-1, sfx_point, 0);
                }
            }
        }
        it = (it->x + Constants::PIPE_WIDTH < 0) ? pipes.erase(it) : std::next(it);
    }
}

void FlappyBird::spawnPipe() {
    if (pipes.empty() || (Constants::WINDOW_WIDTH - pipes.back().x >= Constants::PIPE_SPACING + Constants::PIPE_WIDTH)) {
        int gapY = rand() % (Constants::WINDOW_HEIGHT - Constants::PIPE_GAP - 150) + 75;
        pipes.emplace_back(Constants::WINDOW_WIDTH, gapY);
    }
}

int FlappyBird::loadHighScore() {
    std::ifstream file(HIGH_SCORE_FILE);
    int score = 0;
    if (file.is_open()) {
        file >> score;
        file.close();
    }
    return score;
}

void FlappyBird::saveHighScore() {
    std::ofstream file(HIGH_SCORE_FILE);
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}

void FlappyBird::handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                handleKeyDown(event.key.keysym.sym);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                    handleMouseClick(event.button.x, event.button.y);
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotion(event.motion.x, event.motion.y);
                break;
        }
    }
}

void FlappyBird::handleKeyDown(SDL_Keycode key) {
    switch (gameState) {
        case GameState::TWO_PLAYER_WAITING:
        case GameState::ONE_PLAYER_WAITING:
            if (key == SDLK_SPACE) startGame();
            break;
        case GameState::ONE_PLAYER:
            if (key == SDLK_SPACE && !birds.empty() && birds[0].alive && !birds[0].collided) {
                birds[0].velocity = -8.0f;
                Mix_PlayChannel(-1, sfx_wing, 0);
            }
            break;
        case GameState::TWO_PLAYER:
            if (key == SDLK_SPACE && !birds.empty() && birds[0].alive && !birds[0].collided) {
                birds[0].velocity = -8.0f;
                Mix_PlayChannel(-1, sfx_wing, 0);
            }
            if (birds.size() > 1 && birds[1].alive && !birds[1].collided && key == SDLK_UP) {
                birds[1].velocity = -8.0f;
                Mix_PlayChannel(-1, sfx_wing, 0);
            }
            break;
        default:
            break;
    }
    if (showGameOver && key == SDLK_SPACE) reset(gameState == GameState::ONE_PLAYER ? 1 : 2);
}

void FlappyBird::handleMouseClick(int x, int y) {
    switch (gameState) {
        case GameState::MENU:
            handleMenuClick(x, y);
            break;
        case GameState::TWO_PLAYER_WAITING:
        case GameState::ONE_PLAYER_WAITING:
        case GameState::INFO:
        case GameState::ONE_PLAYER:
        case GameState::TWO_PLAYER:
            if (isPointInRect(x, y, buttons[0].rect)) {
                Mix_PlayChannel(-1, clickSound, 0);
                gameState = GameState::MENU;
                setupMenu();
                if (lobbyMusic) {
                    Mix_VolumeMusic(isMuted ? 0 : MIX_MAX_VOLUME);
                    Mix_PlayMusic(lobbyMusic, -1);
                }
            }
            break;
    }
}

void FlappyBird::handleMenuClick(int x, int y) {
    if (soundButton && isPointInRect(x, y, soundButton->rect)) {
        Mix_PlayChannel(-1, clickSound, 0);
        isMuted = !isMuted;
        Mix_VolumeMusic(isMuted ? 0 : MIX_MAX_VOLUME);
    } else if (isPointInRect(x, y, buttons[0].rect)) {
        Mix_PlayChannel(-1, clickSound, 0);
        Mix_HaltMusic();
        reset(1);
    } else if (isPointInRect(x, y, buttons[1].rect)) {
        Mix_PlayChannel(-1, clickSound, 0);
        Mix_HaltMusic();
        reset(2);
    } else if (isPointInRect(x, y, buttons[2].rect)) {
        Mix_PlayChannel(-1, clickSound, 0);
        gameState = GameState::INFO;
        buttons.clear();
        SDL_Texture* truongTexture = createTextTexture("-Truong-", infoFont, infoTextCache);
        int truongW, truongH;
        SDL_QueryTexture(truongTexture, nullptr, nullptr, &truongW, &truongH);
        buttons.emplace_back((Constants::WINDOW_WIDTH - truongW) / 2, Constants::WINDOW_HEIGHT - 60, truongW, 30, "Return", true);
    }
}

void FlappyBird::handleMouseMotion(int x, int y) {
    for (auto& button : buttons) handleButtonHover(x, y, button, 5.0f);
    if (soundButton) handleButtonHover(x, y, *soundButton, 1.2f);
}

void FlappyBird::startGame() {
    if (playingMusic) {
        Mix_VolumeMusic(isMuted ? 0 : 20);
        Mix_PlayMusic(playingMusic, -1);
    }
    for (auto& bird : birds) {
        bird.score = 0;
        bird.alive = true;
        bird.collided = false;
        bird.dieSoundPlayed = false;
    }
    winner = -1;
    pipes.emplace_back(Constants::WINDOW_WIDTH, rand() % (Constants::WINDOW_HEIGHT - Constants::PIPE_GAP - 100) + 50);
    gameState = (gameState == GameState::TWO_PLAYER_WAITING) ? GameState::TWO_PLAYER : GameState::ONE_PLAYER;
    buttons.clear();
}

void FlappyBird::update() {
    if (gameState == GameState::MENU || gameState == GameState::INFO ||
        gameState == GameState::TWO_PLAYER_WAITING || gameState == GameState::ONE_PLAYER_WAITING ||
        showGameOver) return;

    updateBirdPhysics();
    updatePipes();

    for (auto& bird : birds) {
        if (!bird.alive && bird.collided && !bird.dieSoundPlayed) {
            Mix_PlayChannel(-1, sfx_die, 0);
            bird.dieSoundPlayed = true;
        }
    }

    spawnPipe();

    if (birds.empty() || std::all_of(birds.begin(), birds.end(), [](const Bird& b) { return !b.alive; })) {
        if (!showGameOver) {
            updateGameOver();
            showGameOver = true;
            buttons.clear();
            buttons.emplace_back(Constants::WINDOW_WIDTH / 2 - 140, Constants::WINDOW_HEIGHT - 60, 280, 30, "Return to Menu", true);
        }
    }
}

void FlappyBird::updateGameOver() {
    if (gameState == GameState::TWO_PLAYER && birds.size() == 2) {
        int maxScore = std::max(birds[0].score, birds[1].score);
        highScore = std::max(highScore, maxScore);
        winner = (birds[0].score > birds[1].score) ? 0 : (birds[1].score > birds[0].score) ? 1 : -1;
    } else if (birds.size() == 1) {
        highScore = std::max(highScore, birds[0].score);
    }
}

void FlappyBird::render() {
    SDL_RenderClear(renderer);
    SDL_Rect bgRect = {0, 0, Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT};
    SDL_RenderCopy(renderer, backgroundTexture, nullptr, &bgRect);

    switch (gameState) {
        case GameState::MENU:
            renderMenu();
            break;
        case GameState::ONE_PLAYER_WAITING:
        case GameState::TWO_PLAYER_WAITING:
            renderWaitingScreen();
            break;
        case GameState::ONE_PLAYER:
        case GameState::TWO_PLAYER:
            renderGameplay();
            break;
        case GameState::INFO:
            renderInfo();
            break;
    }
    SDL_RenderPresent(renderer);
}

void FlappyBird::renderMenu() {
    SDL_Rect logoRect = {Constants::WINDOW_WIDTH / 2 - 200, 20, 400, 105};
    SDL_RenderCopy(renderer, logoTexture, nullptr, &logoRect);
    renderButtons();
    SDL_RenderCopy(renderer, isMuted ? speakerOffTexture : speakerOnTexture, nullptr, &soundButton->rect);
}

void FlappyBird::renderWaitingScreen() {
    const bool isTwoPlayer = gameState == GameState::TWO_PLAYER_WAITING;
    renderText(isTwoPlayer ? "2 Player Mode" : "1 Player Mode",
              Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 40, true);
    renderText("Press SPACE to Play", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 + 20, true);
    renderButtons();
}

void FlappyBird::renderGameplay() {
    for (size_t i = 0; i < birds.size(); i++) {
        if (!birds[i].alive) continue;
        SDL_Rect birdRect = {Constants::WINDOW_WIDTH / 4 + static_cast<int>(i * Constants::BIRD_SPACING),
                           static_cast<int>(birds[i].y), Constants::BIRD_SIZE, Constants::BIRD_SIZE};
        SDL_RenderCopy(renderer, i == 0 ? player1Texture : player2Texture, nullptr, &birdRect);
    }

    for (const auto& pipe : pipes) {
        SDL_Rect topPipe = {pipe.x, 0, Constants::PIPE_WIDTH, pipe.gapY - Constants::PIPE_GAP / 2};
        SDL_Rect bottomPipe = {pipe.x, pipe.gapY + Constants::PIPE_GAP / 2, Constants::PIPE_WIDTH,
                             Constants::WINDOW_HEIGHT - (pipe.gapY + Constants::PIPE_GAP / 2)};
        SDL_RenderCopyEx(renderer, pipeTexture, nullptr, &topPipe, 180, nullptr, SDL_FLIP_NONE);
        SDL_RenderCopy(renderer, pipeTexture, nullptr, &bottomPipe);
    }

    renderScores();
    if (showGameOver) renderGameOver();
}

void FlappyBird::renderScores() {
    if (gameState == GameState::ONE_PLAYER && birds.size() == 1) {
        renderText("Score: " + std::to_string(birds[0].score), 10, 10, false, true);
    } else if (birds.size() == 2) {
        renderText("Player 1 Score: " + std::to_string(birds[0].score), 10, 10, false, true);
        renderText("Player 2 Score: " + std::to_string(birds[1].score), 10, 40, false, true);
    }
}

void FlappyBird::renderGameOver() {
    renderText("Game Over!", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 120, true);
    if (gameState == GameState::ONE_PLAYER && birds.size() == 1) {
        renderText("Score: " + std::to_string(birds[0].score), Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 70, true);
        renderText("High Score: " + std::to_string(highScore), Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 20, true);
        if (birds[0].score == highScore && birds[0].score > 0)
            renderText("New Best High Score!", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 + 30, true);
        renderText("Press SPACE to Retry", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 + 80, true);
    } else if (gameState == GameState::TWO_PLAYER && birds.size() == 2) {
        renderText("High Score: " + std::to_string(highScore), Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 70, true);
        renderText(winner == -1 ? "Tie Game!" : "Player " + std::to_string(winner + 1) + " Wins!",
                  Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 20, true);
        if ((birds[0].score == highScore || birds[1].score == highScore) && std::max(birds[0].score, birds[1].score) > 0)
            renderText("New Best High Score!", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 + 30, true);
        renderText("Press SPACE to Retry", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 + 80, true);
    }
    renderButtons();
}

void FlappyBird::renderInfo() {
    const int startY = 30;
    const std::vector<std::string> infoLines = {
        "This game is based on the original",
        "Flappy Bird by Nguyen Quang Dong.",
        "Your duty is to make the bird fly",
        "through pipes as far as possible.",
        "New mode: 2 players! Player 1 is the",
        "yellow bird using the SPACE button,",
        "Player 2 is the green bird using",
        "UP or DOWN/LEFT/RIGHT buttons.",
        "Having fun!",
        "-Truong-"
    };
    for (size_t i = 0; i < infoLines.size(); ++i)
        renderText(infoLines[i], Constants::WINDOW_WIDTH / 2, startY + static_cast<int>(i * 40), true, false, true);
    renderButtons();
}

void FlappyBird::renderButtons() {
    for (const auto& button : buttons) {
        SDL_Texture* texture = createTextTexture(button.text, button.isRed ? infoFont : font,
                                               button.isRed ? infoTextCache : textCache, button.isRed);
        if (!texture) continue;
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        SDL_Rect textRect = {button.rect.x + (button.rect.w - w) / 2,
                           button.rect.y + (button.rect.h - h) / 2, w, h};
        SDL_RenderCopy(renderer, texture, nullptr, &textRect);
    }
}

void FlappyBird::run() {
    while (running) {
        handleInput();
        update();
        render();
        SDL_Delay(16);
    }
}

bool FlappyBird::isRunning() const {
    return running;
}

void FlappyBird::reset(int players) {
    Mix_HaltMusic();
    birds.clear();
    pipes.clear();
    birds.emplace_back(Constants::WINDOW_HEIGHT / 2.0f);
    if (players == 2) {
        birds.emplace_back(Constants::WINDOW_HEIGHT / 2.0f);
        gameState = GameState::TWO_PLAYER_WAITING;
    } else {
        gameState = GameState::ONE_PLAYER_WAITING;
        highScore = std::max(highScore, birds[0].score);
        birds[0].score = 0;
        birds[0].alive = true;
        birds[0].collided = false;
        birds[0].dieSoundPlayed = false;
        winner = -1;
    }
    buttons.clear();
    buttons.emplace_back(Constants::WINDOW_WIDTH / 2 - 140, Constants::WINDOW_HEIGHT - 60, 280, 30, "Return to Menu", true);
    showGameOver = false;
}
