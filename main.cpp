#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <unordered_map>

// Namespace chứa các hằng số cố định của game
namespace Constants {
    constexpr int WINDOW_WIDTH = 640;   // Chiều rộng cửa sổ game
    constexpr int WINDOW_HEIGHT = 480;  // Chiều cao cửa sổ game
    constexpr int BIRD_SIZE = 40;       // Kích thước của chim
    constexpr int PIPE_WIDTH = 50;      // Chiều rộng ống
    constexpr int PIPE_GAP = 150;       // Khoảng cách giữa hai ống trên dưới
    constexpr int PIPE_SPACING = 120;   // Khoảng cách giữa các cặp ống
    constexpr int BIRD_SPACING = 80;    // Khoảng cách giữa hai chim trong chế độ 2 người
}

// Cấu trúc đại diện cho một cặp ống (pipe)
struct Pipe {
    int x;      // Tọa độ x của ống
    int gapY;   // Tọa độ y của khoảng trống giữa hai ống
    Pipe(int x_, int gapY_) : x(x_), gapY(gapY_) {}
};

// Cấu trúc đại diện cho một con chim
struct Bird {
    float y;            // Tọa độ y của chim
    float velocity;     // Vận tốc rơi/lên của chim
    bool alive = true;  // Trạng thái sống/chết của chim
    bool collided = false;  // Trạng thái va chạm với ống
    bool dieSoundPlayed = false;  // Đánh dấu âm thanh chết đã phát chưa
    int score = 0;      // Điểm số của chim
    explicit Bird(float y_) : y(y_), velocity(0) {}
};

// Cấu trúc đại diện cho một nút bấm trên giao diện
struct Button {
    SDL_Rect rect;      // Hình chữ nhật xác định vị trí và kích thước nút
    std::string text;   // Văn bản hiển thị trên nút
    int originalW, originalH;  // Kích thước gốc của nút
    bool hovered = false;      // Trạng thái chuột đang hover trên nút
    bool isRed = false;        // Màu sắc của nút (đỏ hoặc không)
    Button(int x, int y, int w, int h, const std::string& t, bool red = false)
        : rect{x, y, w, h}, text(t), originalW(w), originalH(h), isRed(red) {}
};

// Các trạng thái của game (dùng enum class để an toàn hơn)
enum class GameState {
    MENU,               // Màn hình menu chính
    ONE_PLAYER,         // Chế độ 1 người chơi
    TWO_PLAYER,         // Chế độ 2 người chơi
    INFO,               // Màn hình thông tin
    TWO_PLAYER_WAITING, // Đợi bắt đầu chế độ 2 người
    ONE_PLAYER_WAITING  // Đợi bắt đầu chế độ 1 người
};

class FlappyBird {
private:
    // Các tài nguyên SDL chính
    SDL_Window* window = nullptr;      // Cửa sổ game
    SDL_Renderer* renderer = nullptr;  // Bộ vẽ đồ họa
    TTF_Font* font = nullptr;          // Font chữ chính (48pt)
    TTF_Font* scoreFont = nullptr;     // Font cho điểm số (24pt)
    TTF_Font* infoFont = nullptr;      // Font cho thông tin (24pt)

    // Các texture hình ảnh
    SDL_Texture* backgroundTexture = nullptr;  // Hình nền
    SDL_Texture* logoTexture = nullptr;        // Logo game
    SDL_Texture* pipeTexture = nullptr;        // Hình ống
    SDL_Texture* player1Texture = nullptr;     // Hình chim người chơi 1
    SDL_Texture* player2Texture = nullptr;     // Hình chim người chơi 2
    SDL_Texture* speakerOnTexture = nullptr;   // Icon loa bật
    SDL_Texture* speakerOffTexture = nullptr;  // Icon loa tắt

    // Danh sách các đối tượng trong game
    std::vector<Bird> birds;       // Danh sách chim
    std::vector<Pipe> pipes;       // Danh sách ống
    std::vector<Button> buttons;   // Danh sách nút bấm

    // Bộ nhớ đệm cho các texture văn bản để tăng hiệu suất
    std::unordered_map<std::string, SDL_Texture*> textCache;      // Cache văn bản chính
    std::unordered_map<std::string, SDL_Texture*> scoreTextCache; // Cache điểm số
    std::unordered_map<std::string, SDL_Texture*> infoTextCache;  // Cache thông tin

    // Các hiệu ứng âm thanh
    Mix_Chunk* sfx_die = nullptr;     // Âm thanh khi chim chết
    Mix_Chunk* sfx_hit = nullptr;     // Âm thanh khi va chạm
    Mix_Chunk* sfx_point = nullptr;   // Âm thanh khi ghi điểm
    Mix_Chunk* sfx_wing = nullptr;    // Âm thanh khi vỗ cánh
    Mix_Chunk* clickSound = nullptr;  // Âm thanh khi nhấn nút
    Mix_Chunk* sfx_falling = nullptr; // Âm thanh khi rơi xuống

    // Nhạc nền
    Mix_Music* lobbyMusic = nullptr;   // Nhạc ở menu
    Mix_Music* playingMusic = nullptr; // Nhạc khi chơi

    Button* soundButton = nullptr;     // Nút điều khiển âm thanh
    bool running = false;              // Trạng thái chạy của game
    GameState gameState = GameState::MENU;  // Trạng thái hiện tại của game
    int highScore = 0;                 // Điểm cao nhất
    int winner = -1;                   // Người thắng (0: Player 1, 1: Player 2, -1: Hòa)
    bool showGameOver = false;         // Hiển thị màn hình game over
    bool isMuted = false;              // Trạng thái tắt tiếng

    // Dọn dẹp tài nguyên khi game kết thúc
    void cleanup() {
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

    // Tạo texture cho văn bản với viền đen
    SDL_Texture* createTextTexture(const std::string& text, TTF_Font* targetFont,
                                 std::unordered_map<std::string, SDL_Texture*>& cache,
                                 bool isRed = false) {
        const std::string key = text + (isRed ? "_red" : "");  // Key để lưu vào cache
        if (auto it = cache.find(key); it != cache.end()) return it->second;  // Nếu đã có trong cache thì trả về

        const SDL_Color color = isRed ? SDL_Color{255, 0, 0, 255} : SDL_Color{255, 255, 0, 255};  // Màu chữ
        const SDL_Color outlineColor = {0, 0, 0, 255};  // Màu viền

        // Tạo viền đen
        TTF_SetFontOutline(targetFont, 2);
        SDL_Surface* outlineSurface = TTF_RenderText_Blended(targetFont, text.c_str(), outlineColor);
        TTF_SetFontOutline(targetFont, 0);
        SDL_Surface* textSurface = TTF_RenderText_Blended(targetFont, text.c_str(), color);

        if (!outlineSurface || !textSurface) {
            SDL_FreeSurface(outlineSurface);
            SDL_FreeSurface(textSurface);
            return nullptr;
        }

        // Kết hợp viền và chữ
        SDL_Surface* combinedSurface = SDL_CreateRGBSurface(0, outlineSurface->w, outlineSurface->h, 32,
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

        SDL_BlitSurface(outlineSurface, nullptr, combinedSurface, nullptr);
        SDL_Rect offset = {(combinedSurface->w - textSurface->w) / 2,
                          (combinedSurface->h - textSurface->h) / 2, 0, 0};
        SDL_BlitSurface(textSurface, nullptr, combinedSurface, &offset);

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, combinedSurface);
        if (texture) cache[key] = texture;  // Lưu vào cache nếu tạo thành công

        SDL_FreeSurface(outlineSurface);
        SDL_FreeSurface(textSurface);
        SDL_FreeSurface(combinedSurface);
        return texture;
    }

    // Hiển thị văn bản lên màn hình
    void renderText(const std::string& text, int x, int y, bool center = false,
                   bool isScore = false, bool isInfo = false, bool isRed = false) {
        TTF_Font* targetFont = isScore ? scoreFont : (isInfo ? infoFont : font);  // Chọn font phù hợp
        auto& targetCache = isScore ? scoreTextCache : (isInfo ? infoTextCache : textCache);  // Chọn cache phù hợp
        SDL_Texture* texture = createTextTexture(text, targetFont, targetCache, isRed);
        if (!texture) return;

        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        SDL_Rect rect = {center ? x - w / 2 : x, y, w, h};  // Căn giữa nếu cần
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
    }

    // Kiểm tra điểm có nằm trong hình chữ nhật không
    bool isPointInRect(int x, int y, const SDL_Rect& rect) const {
        return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
    }

    // Tải texture từ file hình ảnh
    SDL_Texture* loadTexture(const char* file) {
        SDL_Surface* surface = IMG_Load(file);
        if (!surface) return nullptr;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        return texture;
    }

    // Khởi tạo các thư viện SDL
    bool initSDL() {
        return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) >= 0 &&  // Khởi tạo video và audio
               TTF_Init() >= 0 &&                         // Khởi tạo thư viện font
               IMG_Init(IMG_INIT_PNG) != 0 &&             // Khởi tạo thư viện hình ảnh
               Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) >= 0;  // Khởi tạo âm thanh
    }

    // Thiết lập cửa sổ và renderer
    bool setupWindowAndRenderer() {
        window = SDL_CreateWindow("Flappy Bird", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) return false;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);  // Sử dụng tăng tốc phần cứng
        if (!renderer) {
            SDL_DestroyWindow(window);
            window = nullptr;
            return false;
        }
        return true;
    }

    // Tải các font chữ
    bool loadFontResources() {
        font = TTF_OpenFont("font/SVN-New Athletic M54.ttf", 48);
        scoreFont = TTF_OpenFont("font/SVN-New Athletic M54.ttf", 24);
        infoFont = TTF_OpenFont("font/SVN-New Athletic M54.ttf", 24);
        return font && scoreFont && infoFont;
    }

    // Tải các texture hình ảnh
    bool loadTextureResources() {
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

    // Tải các tài nguyên âm thanh
    bool loadAudioResources() {
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

    // Thiết lập menu chính
    void setupMenu() {
        buttons.clear();
        const int centerX = Constants::WINDOW_WIDTH / 2 - 100;  // Vị trí trung tâm theo chiều ngang
        buttons.emplace_back(centerX, Constants::WINDOW_HEIGHT / 2 - 60, 200, 30, "1 Player");  // Nút 1 người chơi
        buttons.emplace_back(centerX, Constants::WINDOW_HEIGHT / 2, 200, 30, "2 Players");      // Nút 2 người chơi
        buttons.emplace_back(centerX, Constants::WINDOW_HEIGHT / 2 + 60, 200, 30, "Information");  // Nút thông tin
        soundButton = new Button(Constants::WINDOW_WIDTH - 50, 10, 40, 40, "");  // Nút loa ở góc trên phải
    }

    // Xử lý hiệu ứng hover cho nút
    void handleButtonHover(int x, int y, Button& button, float scaleFactor) {
        SDL_Texture* texture = createTextTexture(button.text, font, textCache, button.isRed);
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        SDL_Rect textRect = {button.rect.x + (button.rect.w - w) / 2,
                           button.rect.y + (button.rect.h - h) / 2, w, h};

        if (isPointInRect(x, y, textRect)) {  // Khi chuột hover
            if (!button.hovered) {
                button.hovered = true;
                button.rect.w = static_cast<int>(button.originalW * scaleFactor);  // Phóng to nút
                button.rect.h = static_cast<int>(button.originalH * scaleFactor);
                button.rect.x -= (button.rect.w - button.originalW) / 2;  // Căn lại vị trí
                button.rect.y -= (button.rect.h - button.originalH) / 2;
            }
        } else if (button.hovered) {  // Khi chuột rời khỏi
            button.hovered = false;
            button.rect.w = button.originalW;  // Trở về kích thước gốc
            button.rect.h = button.originalH;
            button.rect.x += (static_cast<int>(button.originalW * scaleFactor) - button.originalW) / 2;
            button.rect.y += (static_cast<int>(button.originalH * scaleFactor) - button.originalH) / 2;
        }
    }

    // Cập nhật vật lý cho chim (rơi, giới hạn màn hình)
    void updateBirdPhysics() {
        for (auto& bird : birds) {
            bird.velocity += 0.4f;  // Tăng vận tốc rơi (trọng lực)
            bird.y += bird.velocity;  // Cập nhật vị trí y

            // Kiểm tra chim chạm đất
            if (bird.y + Constants::BIRD_SIZE >= Constants::WINDOW_HEIGHT) {
                if (bird.collided) bird.alive = false;
                else if (bird.alive) {
                    Mix_PlayChannel(-1, sfx_falling, 0);  // Phát âm thanh rơi
                    bird.alive = false;
                }
            }
            // Kiểm tra chim chạm trần
            if (bird.y < 0) {
                bird.y = 0;
                bird.velocity = 0;
            }
        }
    }

    // Cập nhật vị trí và va chạm của ống
    void updatePipes() {
        for (auto it = pipes.begin(); it != pipes.end();) {
            it->x -= 2;  // Di chuyển ống sang trái
            for (size_t i = 0; i < birds.size(); i++) {
                if (!birds[i].alive) continue;
                int birdX = Constants::WINDOW_WIDTH / 4 + static_cast<int>(i * Constants::BIRD_SPACING);  // Vị trí x của chim
                SDL_Rect birdRect = {birdX, static_cast<int>(birds[i].y), Constants::BIRD_SIZE, Constants::BIRD_SIZE};
                SDL_Rect topPipe = {it->x, 0, Constants::PIPE_WIDTH, it->gapY - Constants::PIPE_GAP / 2};  // Ống trên
                SDL_Rect bottomPipe = {it->x, it->gapY + Constants::PIPE_GAP / 2, Constants::PIPE_WIDTH,
                                     Constants::WINDOW_HEIGHT - (it->gapY + Constants::PIPE_GAP / 2)};  // Ống dưới

                // Kiểm tra va chạm giữa chim và ống
                if ((SDL_HasIntersection(&birdRect, &topPipe) || SDL_HasIntersection(&birdRect, &bottomPipe)) &&
                    !birds[i].collided) {
                    birds[i].collided = true;
                    Mix_PlayChannel(-1, sfx_hit, 0);  // Phát âm thanh va chạm
                }
            }

            // Ghi điểm khi chim vượt qua ống
            if (it->x + Constants::PIPE_WIDTH < Constants::WINDOW_WIDTH / 4 &&
                it->x + Constants::PIPE_WIDTH >= Constants::WINDOW_WIDTH / 4 - 2) {
                for (auto& bird : birds) {
                    if (bird.alive) {
                        bird.score++;
                        Mix_PlayChannel(-1, sfx_point, 0);  // Phát âm thanh ghi điểm
                    }
                }
            }
            // Xóa ống khi ra khỏi màn hình
            it = (it->x + Constants::PIPE_WIDTH < 0) ? pipes.erase(it) : std::next(it);
        }
    }

    // Tạo ống mới
    void spawnPipe() {
        if (pipes.empty() || (Constants::WINDOW_WIDTH - pipes.back().x >= Constants::PIPE_SPACING + Constants::PIPE_WIDTH)) {
            int gapY = rand() % (Constants::WINDOW_HEIGHT - Constants::PIPE_GAP - 150) + 75;  // Vị trí ngẫu nhiên cho khoảng trống
            pipes.emplace_back(Constants::WINDOW_WIDTH, gapY);
        }
    }

public:
    // Khởi tạo game
    FlappyBird() {
        if (!initSDL() || !setupWindowAndRenderer() || !loadFontResources() ||
            !loadTextureResources() || !loadAudioResources()) {
            cleanup();
            return;
        }
        std::srand(static_cast<unsigned>(std::time(nullptr)));  // Khởi tạo seed ngẫu nhiên
        running = true;
        setupMenu();
        if (lobbyMusic) {
            Mix_VolumeMusic(MIX_MAX_VOLUME);  // Đặt âm lượng tối đa
            Mix_PlayMusic(lobbyMusic, -1);    // Phát nhạc lobby lặp vô hạn
        }
    }

    ~FlappyBird() { cleanup(); }

    // Xử lý đầu vào từ người chơi
    void handleInput() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:  // Thoát game khi nhấn nút đóng cửa sổ
                    running = false;
                    break;
                case SDL_KEYDOWN:  // Xử lý phím nhấn
                    handleKeyDown(event.key.keysym.sym);
                    break;
                case SDL_MOUSEBUTTONDOWN:  // Xử lý click chuột
                    if (event.button.button == SDL_BUTTON_LEFT)
                        handleMouseClick(event.button.x, event.button.y);
                    break;
                case SDL_MOUSEMOTION:  // Xử lý di chuyển chuột
                    handleMouseMotion(event.motion.x, event.motion.y);
                    break;
            }
        }
    }

    // Xử lý sự kiện phím nhấn
    void handleKeyDown(SDL_Keycode key) {
        switch (gameState) {
            case GameState::TWO_PLAYER_WAITING:
            case GameState::ONE_PLAYER_WAITING:
                if (key == SDLK_SPACE) startGame();  // Bắt đầu game khi nhấn SPACE
                break;
            case GameState::ONE_PLAYER:
                if (key == SDLK_SPACE && !birds.empty() && birds[0].alive && !birds[0].collided) {
                    birds[0].velocity = -8.0f;  // Chim bay lên
                    Mix_PlayChannel(-1, sfx_wing, 0);  // Phát âm thanh vỗ cánh
                }
                break;
            case GameState::TWO_PLAYER:
                if (key == SDLK_SPACE && !birds.empty() && birds[0].alive && !birds[0].collided) {
                    birds[0].velocity = -8.0f;  // Player 1 bay lên
                    Mix_PlayChannel(-1, sfx_wing, 0);
                }
                if (birds.size() > 1 && birds[1].alive && !birds[1].collided && key == SDLK_UP) {
                    birds[1].velocity = -8.0f;  // Player 2 bay lên
                    Mix_PlayChannel(-1, sfx_wing, 0);
                }
                break;
            default:
                break;
        }
        if (showGameOver && key == SDLK_SPACE) reset(gameState == GameState::ONE_PLAYER ? 1 : 2);  // Reset game khi game over
    }

    // Xử lý sự kiện click chuột
    void handleMouseClick(int x, int y) {
        switch (gameState) {
            case GameState::MENU:
                handleMenuClick(x, y);
                break;
            case GameState::TWO_PLAYER_WAITING:
            case GameState::ONE_PLAYER_WAITING:
            case GameState::INFO:
            case GameState::ONE_PLAYER:
            case GameState::TWO_PLAYER:
                if (isPointInRect(x, y, buttons[0].rect)) {  // Nhấn nút "Return to Menu"
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

    // Xử lý click trong menu chính
    void handleMenuClick(int x, int y) {
        if (soundButton && isPointInRect(x, y, soundButton->rect)) {  // Nhấn nút loa
            Mix_PlayChannel(-1, clickSound, 0);
            isMuted = !isMuted;
            Mix_VolumeMusic(isMuted ? 0 : MIX_MAX_VOLUME);  // Bật/tắt âm thanh
        } else if (isPointInRect(x, y, buttons[0].rect)) {  // Chọn 1 người chơi
            Mix_PlayChannel(-1, clickSound, 0);
            Mix_HaltMusic();
            reset(1);
        } else if (isPointInRect(x, y, buttons[1].rect)) {  // Chọn 2 người chơi
            Mix_PlayChannel(-1, clickSound, 0);
            Mix_HaltMusic();
            reset(2);
        } else if (isPointInRect(x, y, buttons[2].rect)) {  // Chọn màn hình thông tin
            Mix_PlayChannel(-1, clickSound, 0);
            gameState = GameState::INFO;
            buttons.clear();
            SDL_Texture* truongTexture = createTextTexture("-Truong-", infoFont, infoTextCache);
            int truongW, truongH;
            SDL_QueryTexture(truongTexture, nullptr, nullptr, &truongW, &truongH);
            buttons.emplace_back((Constants::WINDOW_WIDTH - truongW) / 2, Constants::WINDOW_HEIGHT - 60, truongW, 30, "Return", true);
        }
    }

    // Xử lý di chuyển chuột (hover)
    void handleMouseMotion(int x, int y) {
        for (auto& button : buttons) handleButtonHover(x, y, button, 5.0f);  // Hover cho các nút chính
        if (soundButton) handleButtonHover(x, y, *soundButton, 1.2f);       // Hover cho nút loa
    }

    // Bắt đầu game từ trạng thái chờ
    void startGame() {
        if (playingMusic) {
            Mix_VolumeMusic(isMuted ? 0 : 20);  // Giảm âm lượng khi chơi
            Mix_PlayMusic(playingMusic, -1);
        }
        for (auto& bird : birds) {  // Reset trạng thái chim
            bird.score = 0;
            bird.alive = true;
            bird.collided = false;
            bird.dieSoundPlayed = false;
        }
        winner = -1;
        pipes.emplace_back(Constants::WINDOW_WIDTH, rand() % (Constants::WINDOW_HEIGHT - Constants::PIPE_GAP - 100) + 50);  // Tạo ống đầu tiên
        gameState = (gameState == GameState::TWO_PLAYER_WAITING) ? GameState::TWO_PLAYER : GameState::ONE_PLAYER;
        buttons.clear();
    }

    // Cập nhật trạng thái game
    void update() {
        // Không cập nhật nếu đang ở trạng thái tĩnh hoặc game over
        if (gameState == GameState::MENU || gameState == GameState::INFO ||
            gameState == GameState::TWO_PLAYER_WAITING || gameState == GameState::ONE_PLAYER_WAITING ||
            showGameOver) return;

        updateBirdPhysics();  // Cập nhật vị trí chim
        updatePipes();        // Cập nhật ống và va chạm

        // Phát âm thanh chết khi cần
        for (auto& bird : birds) {
            if (!bird.alive && bird.collided && !bird.dieSoundPlayed) {
                Mix_PlayChannel(-1, sfx_die, 0);
                bird.dieSoundPlayed = true;
            }
        }

        spawnPipe();  // Tạo ống mới nếu cần

        // Kiểm tra game over
        if (birds.empty() || std::all_of(birds.begin(), birds.end(), [](const Bird& b) { return !b.alive; })) {
            if (!showGameOver) {
                updateGameOver();  // Cập nhật điểm cao và người thắng
                showGameOver = true;
                buttons.clear();
                buttons.emplace_back(Constants::WINDOW_WIDTH / 2 - 140, Constants::WINDOW_HEIGHT - 60, 280, 30, "Return to Menu", true);
            }
        }
    }

    // Cập nhật thông tin game over
    void updateGameOver() {
        if (gameState == GameState::TWO_PLAYER && birds.size() == 2) {
            int maxScore = std::max(birds[0].score, birds[1].score);
            highScore = std::max(highScore, maxScore);  // Cập nhật điểm cao nhất
            winner = (birds[0].score > birds[1].score) ? 0 : (birds[1].score > birds[0].score) ? 1 : -1;  // Xác định người thắng
        } else if (birds.size() == 1) {
            highScore = std::max(highScore, birds[0].score);
        }
    }

    // Vẽ toàn bộ giao diện game
    void render() {
        SDL_RenderClear(renderer);  // Xóa màn hình
        SDL_Rect bgRect = {0, 0, Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT};
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, &bgRect);  // Vẽ nền

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
        SDL_RenderPresent(renderer);  // Hiển thị frame mới
    }

    // Vẽ màn hình menu
    void renderMenu() {
        SDL_Rect logoRect = {Constants::WINDOW_WIDTH / 2 - 200, 20, 400, 105};
        SDL_RenderCopy(renderer, logoTexture, nullptr, &logoRect);  // Vẽ logo
        renderButtons();  // Vẽ các nút
        SDL_RenderCopy(renderer, isMuted ? speakerOffTexture : speakerOnTexture, nullptr, &soundButton->rect);  // Vẽ nút loa
    }

    // Vẽ màn hình chờ
    void renderWaitingScreen() {
        const bool isTwoPlayer = gameState == GameState::TWO_PLAYER_WAITING;
        renderText(isTwoPlayer ? "2 Player Mode" : "1 Player Mode",
                  Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 - 40, true);  // Hiển thị chế độ chơi
        renderText("Press SPACE to Play", Constants::WINDOW_WIDTH / 2, Constants::WINDOW_HEIGHT / 2 + 20, true);  // Hướng dẫn
        renderButtons();
    }

    // Vẽ màn hình chơi game
    void renderGameplay() {
        // Vẽ chim
        for (size_t i = 0; i < birds.size(); i++) {
            if (!birds[i].alive) continue;
            SDL_Rect birdRect = {Constants::WINDOW_WIDTH / 4 + static_cast<int>(i * Constants::BIRD_SPACING),
                               static_cast<int>(birds[i].y), Constants::BIRD_SIZE, Constants::BIRD_SIZE};
            SDL_RenderCopy(renderer, i == 0 ? player1Texture : player2Texture, nullptr, &birdRect);
        }

        // Vẽ ống
        for (const auto& pipe : pipes) {
            SDL_Rect topPipe = {pipe.x, 0, Constants::PIPE_WIDTH, pipe.gapY - Constants::PIPE_GAP / 2};
            SDL_Rect bottomPipe = {pipe.x, pipe.gapY + Constants::PIPE_GAP / 2, Constants::PIPE_WIDTH,
                                 Constants::WINDOW_HEIGHT - (pipe.gapY + Constants::PIPE_GAP / 2)};
            SDL_RenderCopyEx(renderer, pipeTexture, nullptr, &topPipe, 180, nullptr, SDL_FLIP_NONE);  // Ống trên (lật ngược)
            SDL_RenderCopy(renderer, pipeTexture, nullptr, &bottomPipe);  // Ống dưới
        }

        renderScores();  // Vẽ điểm số
        if (showGameOver) renderGameOver();  // Vẽ màn hình game over nếu cần
    }

    // Vẽ điểm số
    void renderScores() {
        if (gameState == GameState::ONE_PLAYER && birds.size() == 1) {
            renderText("Score: " + std::to_string(birds[0].score), 10, 10, false, true);
        } else if (birds.size() == 2) {
            renderText("Player 1 Score: " + std::to_string(birds[0].score), 10, 10, false, true);
            renderText("Player 2 Score: " + std::to_string(birds[1].score), 10, 40, false, true);
        }
    }

    // Vẽ màn hình game over
    void renderGameOver() {
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

    // Vẽ màn hình thông tin
    void renderInfo() {
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

    // Vẽ tất cả các nút
    void renderButtons() {
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

    // Vòng lặp chính của game
    void run() {
        while (running) {
            handleInput();  // Xử lý đầu vào
            update();       // Cập nhật trạng thái
            render();       // Vẽ giao diện
            SDL_Delay(16);  // Giới hạn FPS (~60 FPS)
        }
    }

    bool isRunning() const { return running; }

    // Reset game về trạng thái ban đầu
    void reset(int players) {
        Mix_HaltMusic();  // Dừng nhạc
        birds.clear();
        pipes.clear();
        birds.emplace_back(Constants::WINDOW_HEIGHT / 2.0f);  // Tạo chim 1
        if (players == 2) {
            birds.emplace_back(Constants::WINDOW_HEIGHT / 2.0f);  // Tạo chim 2 nếu 2 người chơi
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
};

int main(int argc, char* argv[]) {
    FlappyBird game;
    if (game.isRunning()) game.run();  // Chạy game nếu khởi tạo thành công
    return 0;
}
