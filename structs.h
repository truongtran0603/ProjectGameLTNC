#ifndef STRUCTS_H
#define STRUCTS_H

#include <SDL.h>
#include <string>

struct Pipe {
    int x;      // Tọa độ x của ống
    int gapY;   // Tọa độ y của khoảng trống giữa hai ống
    Pipe(int x_, int gapY_) : x(x_), gapY(gapY_) {}
};

struct Bird {
    float y;            // Tọa độ y của chim
    float velocity;     // Vận tốc rơi/lên của chim
    bool alive = true;  // Trạng thái sống/chết của chim
    bool collided = false;  // Trạng thái va chạm với ống
    bool dieSoundPlayed = false;  // Đánh dấu âm thanh chết đã phát chưa
    int score = 0;      // Điểm số của chim
    explicit Bird(float y_) : y(y_), velocity(0) {}
};

struct Button {
    SDL_Rect rect;      // Hình chữ nhật xác định vị trí và kích thước nút
    std::string text;   // Văn bản hiển thị trên nút
    int originalW, originalH;  // Kích thước gốc của nút
    bool hovered = false;      // Trạng thái chuột đang hover trên nút
    bool isRed = false;        // Màu sắc của nút (đỏ hoặc không)
    Button(int x, int y, int w, int h, const std::string& t, bool red = false)
        : rect{x, y, w, h}, text(t), originalW(w), originalH(h), isRed(red) {}
};

enum class GameState {
    MENU,               // Màn hình menu chính
    ONE_PLAYER,         // Chế độ 1 người chơi
    TWO_PLAYER,         // Chế độ 2 người chơi
    INFO,               // Màn hình thông tin
    TWO_PLAYER_WAITING, // Đợi bắt đầu chế độ 2 người
    ONE_PLAYER_WAITING  // Đợi bắt đầu chế độ 1 người
};

#endif // STRUCTS_H
