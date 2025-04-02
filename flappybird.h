#ifndef FLAPPYBIRD_H
#define FLAPPYBIRD_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "constants.h"
#include "structs.h"

class FlappyBird {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    TTF_Font* scoreFont = nullptr;
    TTF_Font* infoFont = nullptr;
    int frameCounter = 0;

    SDL_Texture* backgroundTexture = nullptr;
    SDL_Texture* logoTexture = nullptr;
    SDL_Texture* pipeTexture = nullptr;
    SDL_Texture* player1Texture = nullptr;
    SDL_Texture* player2Texture = nullptr;
    SDL_Texture* speakerOnTexture = nullptr;
    SDL_Texture* speakerOffTexture = nullptr;

    std::vector<Bird> birds;
    std::vector<Pipe> pipes;
    std::vector<Button> buttons;

    std::unordered_map<std::string, SDL_Texture*> textCache;
    std::unordered_map<std::string, SDL_Texture*> scoreTextCache;
    std::unordered_map<std::string, SDL_Texture*> infoTextCache;

    Mix_Chunk* sfx_die = nullptr;
    Mix_Chunk* sfx_hit = nullptr;
    Mix_Chunk* sfx_point = nullptr;
    Mix_Chunk* sfx_wing = nullptr;
    Mix_Chunk* clickSound = nullptr;
    Mix_Chunk* sfx_falling = nullptr;

    Mix_Music* lobbyMusic = nullptr;
    Mix_Music* playingMusic = nullptr;

    Button* soundButton = nullptr;
    bool running = false;
    GameState gameState = GameState::MENU;
    int highScore = 0;
    int winner = -1;
    bool showGameOver = false;
    bool isMuted = false;
    const std::string HIGH_SCORE_FILE = "highscore.txt";

    void cleanup();
    SDL_Texture* createTextTexture(const std::string& text, TTF_Font* targetFont,
                                 std::unordered_map<std::string, SDL_Texture*>& cache,
                                 bool isRed = false);
    void renderText(const std::string& text, int x, int y, bool center = false,
                   bool isScore = false, bool isInfo = false, bool isRed = false);
    bool isPointInRect(int x, int y, const SDL_Rect& rect) const;
    SDL_Texture* loadTexture(const char* file);
    bool initSDL();
    bool setupWindowAndRenderer();
    bool loadFontResources();
    bool loadTextureResources();
    bool loadAudioResources();
    void setupMenu();
    void handleButtonHover(int x, int y, Button& button, float scaleFactor);
    void updateBirdPhysics();
    void updatePipes();
    void spawnPipe();
    int loadHighScore();
    void saveHighScore();

public:
    FlappyBird();
    ~FlappyBird();

    void handleInput();
    void handleKeyDown(SDL_Keycode key);
    void handleMouseClick(int x, int y);
    void handleMenuClick(int x, int y);
    void handleMouseMotion(int x, int y);
    void startGame();
    void update();
    void updateGameOver();
    void render();
    void renderMenu();
    void renderWaitingScreen();
    void renderGameplay();
    void renderScores();
    void renderGameOver();
    void renderInfo();
    void renderButtons();
    void run();
    bool isRunning() const;
    void reset(int players);
};

#endif // FLAPPYBIRD_H
