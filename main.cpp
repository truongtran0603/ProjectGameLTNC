#include "flappybird.h"

int main(int argc, char* argv[]) {
    FlappyBird game;
    if (game.isRunning()) game.run();
    return 0;
}
