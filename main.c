#include "raylib.h"
#include "raymath.h"

// Game state constants
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_WIDTH 100
#define PLAYER_HEIGHT 20
#define BALL_RADIUS 10
#define BLOCK_WIDTH 70
#define BLOCK_HEIGHT 30
#define BLOCKS_X 10
#define BLOCKS_Y 5
#define BLOCK_SPACING_X 10
#define BLOCK_SPACING_Y 10
#define INITIAL_LIVES 3
#define POWERUP_SIZE 15
#define POWERUP_SPEED 150

// Game states
typedef enum GameState {
    TITLE,
    GAMEPLAY,
    GAME_OVER,
    GAME_WIN
} GameState;

// Block types
typedef enum BlockType {
    NORMAL,
    POWERUP
} BlockType;

// Powerup types
typedef enum PowerupType {
    EXTRA_LIFE
} PowerupType;

// Block structure
typedef struct Block {
    Rectangle rect;
    bool active;
    Color color;
    BlockType type;
} Block;

// Powerup structure
typedef struct Powerup {
    Vector2 position;
    bool active;
    PowerupType type;
    Rectangle rect;
} Powerup;

// Game structure
typedef struct Game {
    Vector2 playerPosition;
    Vector2 ballPosition;
    Vector2 ballVelocity;
    Block blocks[BLOCKS_X * BLOCKS_Y];
    Powerup powerups[BLOCKS_X * BLOCKS_Y]; // Maximum possible powerups
    int lives;
    int score;
    int activeBlocks;
    GameState state;
} Game;

// Function prototypes
void InitGame(Game *game);
void UpdateGame(Game *game, float deltaTime);
void DrawGame(Game *game);
void ResetBall(Game *game);
void CheckCollisions(Game *game);
bool CheckBlockCollision(Game *game, Vector2 *ballPos, Vector2 *ballVel, Rectangle blockRect);
void ActivatePowerup(Game *game, PowerupType type);

int main(void) {
    // Initialize window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Block Kuzushi");
    SetTargetFPS(60);
    
    // Initialize game
    Game game;
    InitGame(&game);
    
    // Main game loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Update game
        UpdateGame(&game, deltaTime);
        
        // Draw game
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawGame(&game);
        EndDrawing();
    }
    
    // Close window
    CloseWindow();
    
    return 0;
}

// Initialize game state
void InitGame(Game *game) {
    // Initialize player
    game->playerPosition = (Vector2){ SCREEN_WIDTH / 2.0f - PLAYER_WIDTH / 2.0f, SCREEN_HEIGHT - 50 };
    
    // Initialize ball
    ResetBall(game);
    
    // Initialize blocks
    int blockIndex = 0;
    game->activeBlocks = 0;
    
    for (int y = 0; y < BLOCKS_Y; y++) {
        for (int x = 0; x < BLOCKS_X; x++) {
            float blockX = x * (BLOCK_WIDTH + BLOCK_SPACING_X) + BLOCK_SPACING_X;
            float blockY = y * (BLOCK_HEIGHT + BLOCK_SPACING_Y) + 50;
            
            game->blocks[blockIndex].rect = (Rectangle){ blockX, blockY, BLOCK_WIDTH, BLOCK_HEIGHT };
            game->blocks[blockIndex].active = true;
            game->blocks[blockIndex].color = (Color){ 230, 41, 55, 255 }; // Red
            
            // Every 5th block has powerup (simple rule)
            if (blockIndex % 5 == 0) {
                game->blocks[blockIndex].type = POWERUP;
                game->blocks[blockIndex].color = (Color){ 0, 228, 48, 255 }; // Green
            } else {
                game->blocks[blockIndex].type = NORMAL;
            }
            
            game->powerups[blockIndex].active = false;
            
            blockIndex++;
            game->activeBlocks++;
        }
    }
    
    // Initialize game state
    game->lives = INITIAL_LIVES;
    game->score = 0;
    game->state = GAMEPLAY;
}

// Reset ball position
void ResetBall(Game *game) {
    game->ballPosition = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    
    // Random angle between -45 and 45 degrees (in radians)
    float angle = GetRandomValue(-45, 45) * DEG2RAD;
    game->ballVelocity = (Vector2){ cosf(angle) * 300, -sinf(angle) * 300 };
}

// Update game state
void UpdateGame(Game *game, float deltaTime) {
    if (game->state == GAMEPLAY) {
        // Move player
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            game->playerPosition.x -= 500.0f * deltaTime;
        }
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            game->playerPosition.x += 500.0f * deltaTime;
        }
        
        // Keep player within screen bounds
        if (game->playerPosition.x < 0) {
            game->playerPosition.x = 0;
        }
        if (game->playerPosition.x > SCREEN_WIDTH - PLAYER_WIDTH) {
            game->playerPosition.x = SCREEN_WIDTH - PLAYER_WIDTH;
        }
        
        // Update ball position
        game->ballPosition = Vector2Add(game->ballPosition, Vector2Scale(game->ballVelocity, deltaTime));
        
        // Ball-wall collisions
        if (game->ballPosition.x < BALL_RADIUS || game->ballPosition.x > SCREEN_WIDTH - BALL_RADIUS) {
            game->ballVelocity.x *= -1.0f;
        }
        if (game->ballPosition.y < BALL_RADIUS) {
            game->ballVelocity.y *= -1.0f;
        }
        
        // Ball-player collision
        Rectangle playerRect = { game->playerPosition.x, game->playerPosition.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        if (CheckCollisionCircleRec(game->ballPosition, BALL_RADIUS, playerRect) && game->ballVelocity.y > 0) {
            // Calculate reflection angle based on where the ball hit the paddle
            float hitPos = (game->ballPosition.x - game->playerPosition.x) / PLAYER_WIDTH;
            float angle = (hitPos - 0.5f) * PI; // -PI/2 to PI/2
            
            float speed = Vector2Length(game->ballVelocity);
            game->ballVelocity.x = cosf(angle) * speed;
            game->ballVelocity.y = -sinf(angle) * speed;
        }
        
        // Ball out of bounds
        if (game->ballPosition.y > SCREEN_HEIGHT + BALL_RADIUS) {
            game->lives--;
            if (game->lives <= 0) {
                game->state = GAME_OVER;
            } else {
                ResetBall(game);
            }
        }
        
        // Update powerups
        for (int i = 0; i < BLOCKS_X * BLOCKS_Y; i++) {
            if (game->powerups[i].active) {
                game->powerups[i].position.y += POWERUP_SPEED * deltaTime;
                game->powerups[i].rect.y = game->powerups[i].position.y;
                
                // Check if powerup is out of bounds
                if (game->powerups[i].position.y > SCREEN_HEIGHT) {
                    game->powerups[i].active = false;
                }
                
                // Check if powerup collides with player
                if (CheckCollisionRecs(game->powerups[i].rect, playerRect)) {
                    ActivatePowerup(game, game->powerups[i].type);
                    game->powerups[i].active = false;
                }
            }
        }
        
        // Check collisions with blocks
        CheckCollisions(game);
        
        // Check if all blocks are destroyed
        if (game->activeBlocks <= 0) {
            game->state = GAME_WIN;
        }
    } else {
        // Title, Game Over, or Win state
        if (IsKeyPressed(KEY_SPACE)) {
            InitGame(game);
        }
    }
}

// Check and handle collisions between ball and blocks
void CheckCollisions(Game *game) {
    for (int i = 0; i < BLOCKS_X * BLOCKS_Y; i++) {
        if (game->blocks[i].active) {
            if (CheckBlockCollision(game, &game->ballPosition, &game->ballVelocity, game->blocks[i].rect)) {
                game->blocks[i].active = false;
                game->activeBlocks--;
                game->score += 10;
                
                // Spawn powerup if block had one
                if (game->blocks[i].type == POWERUP) {
                    game->powerups[i].position = (Vector2){ 
                        game->blocks[i].rect.x + game->blocks[i].rect.width / 2.0f - POWERUP_SIZE / 2.0f,
                        game->blocks[i].rect.y + game->blocks[i].rect.height
                    };
                    game->powerups[i].active = true;
                    game->powerups[i].type = EXTRA_LIFE;
                    game->powerups[i].rect = (Rectangle){ 
                        game->powerups[i].position.x, 
                        game->powerups[i].position.y, 
                        POWERUP_SIZE, 
                        POWERUP_SIZE 
                    };
                }
            }
        }
    }
}

// Handle block collision and return whether it occurred
bool CheckBlockCollision(Game *game, Vector2 *ballPos, Vector2 *ballVel, Rectangle blockRect) {
    if (CheckCollisionCircleRec(*ballPos, BALL_RADIUS, blockRect)) {
        // Calculate collision response
        float ballLeft = ballPos->x - BALL_RADIUS;
        float ballRight = ballPos->x + BALL_RADIUS;
        float ballTop = ballPos->y - BALL_RADIUS;
        float ballBottom = ballPos->y + BALL_RADIUS;
        
        float blockLeft = blockRect.x;
        float blockRight = blockRect.x + blockRect.width;
        float blockTop = blockRect.y;
        float blockBottom = blockRect.y + blockRect.height;
        
        // Determine collision side (simple approximation)
        float overlapLeft = ballRight - blockLeft;
        float overlapRight = blockRight - ballLeft;
        float overlapTop = ballBottom - blockTop;
        float overlapBottom = blockBottom - ballTop;
        
        // Find smallest overlap
        float minOverlapX = overlapLeft < overlapRight ? overlapLeft : overlapRight;
        float minOverlapY = overlapTop < overlapBottom ? overlapTop : overlapBottom;
        
        // Resolve collision on the axis with smallest overlap
        if (minOverlapX < minOverlapY) {
            ballVel->x *= -1.0f;
        } else {
            ballVel->y *= -1.0f;
        }
        
        return true;
    }
    
    return false;
}

// Activate powerup effect
void ActivatePowerup(Game *game, PowerupType type) {
    switch (type) {
        case EXTRA_LIFE:
            game->lives++;
            break;
    }
}

// Draw game elements
void DrawGame(Game *game) {
    if (game->state == GAMEPLAY) {
        // Draw player
        DrawRectangleV(game->playerPosition, (Vector2){ PLAYER_WIDTH, PLAYER_HEIGHT }, BLUE);
        
        // Draw ball
        DrawCircleV(game->ballPosition, BALL_RADIUS, RED);
        
        // Draw blocks
        for (int i = 0; i < BLOCKS_X * BLOCKS_Y; i++) {
            if (game->blocks[i].active) {
                DrawRectangleRec(game->blocks[i].rect, game->blocks[i].color);
                DrawRectangleLinesEx(game->blocks[i].rect, 1, BLACK);
            }
        }
        
        // Draw powerups
        for (int i = 0; i < BLOCKS_X * BLOCKS_Y; i++) {
            if (game->powerups[i].active) {
                DrawRectangleRec(game->powerups[i].rect, GREEN);
                DrawText("+1", (int)game->powerups[i].position.x + 2, (int)game->powerups[i].position.y + 2, 10, WHITE);
            }
        }
        
        // Draw UI
        DrawText(TextFormat("LIVES: %d", game->lives), 10, SCREEN_HEIGHT - 30, 20, DARKGRAY);
        DrawText(TextFormat("SCORE: %d", game->score), SCREEN_WIDTH - 140, SCREEN_HEIGHT - 30, 20, DARKGRAY);
    } else if (game->state == GAME_OVER) {
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 40, 40, RED);
        DrawText("PRESS SPACE TO RESTART", SCREEN_WIDTH / 2 - MeasureText("PRESS SPACE TO RESTART", 20) / 2, SCREEN_HEIGHT / 2 + 10, 20, DARKGRAY);
        DrawText(TextFormat("FINAL SCORE: %d", game->score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("FINAL SCORE: %d", game->score), 20) / 2, SCREEN_HEIGHT / 2 + 40, 20, DARKGRAY);
    } else if (game->state == GAME_WIN) {
        DrawText("YOU WIN!", SCREEN_WIDTH / 2 - MeasureText("YOU WIN!", 40) / 2, SCREEN_HEIGHT / 2 - 40, 40, GREEN);
        DrawText("PRESS SPACE TO RESTART", SCREEN_WIDTH / 2 - MeasureText("PRESS SPACE TO RESTART", 20) / 2, SCREEN_HEIGHT / 2 + 10, 20, DARKGRAY);
        DrawText(TextFormat("FINAL SCORE: %d", game->score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("FINAL SCORE: %d", game->score), 20) / 2, SCREEN_HEIGHT / 2 + 40, 20, DARKGRAY);
    }
}
