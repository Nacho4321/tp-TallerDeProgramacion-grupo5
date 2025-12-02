#ifndef RESULTS_SCREEN_H
#define RESULTS_SCREEN_H

#include <SDL2pp/SDL2pp.hh>
#include <chrono>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include "../common/messages.h"

using namespace SDL2pp;

// Configuration constants
constexpr int RESULTS_DISPLAY_DURATION_MS = 9000;
constexpr int OVERLAY_ALPHA = 180;

class ResultsScreen {
private:
    std::unique_ptr<Font> contentFont;
    std::unique_ptr<Font> headerFont;
    std::array<std::unique_ptr<Texture>, 8> positionImages;
    std::unique_ptr<Texture> tallerTexture;  // Upgrade icons sprite sheet

    bool visible;
    std::chrono::steady_clock::time_point startTime;
    std::vector<ServerMessage::PlayerRaceTime> raceResults;
    std::vector<ServerMessage::PlayerTotalTime> totalResults;
    int32_t mainPlayerId;

    // Player upgrade levels for displaying icons
    uint8_t upgradeSpeed;
    uint8_t upgradeAcceleration;
    uint8_t upgradeHandling;
    uint8_t upgradeDurability;

    int screenWidth;
    int screenHeight;

    void renderBackground(Renderer& renderer);
    void renderContent(Renderer& renderer);
    void renderUpgradeIcons(Renderer& renderer);

    std::string formatTime(uint32_t ms);

    bool countdownActive = false;
    int initialCountdown = 0;
    int currentCountdown = 0;
    Uint32 countdownStartTime = 0;
    std::unordered_map<int, SDL_Color> color_map = {
        {10, {255,   0,   0, 255}}, 
        {9,  {255,  40,  40, 255}}, 
        {8,  {255,  60,  60, 255}},
        {7,  {255,  80,  80, 255}},
        {6,  {255, 100, 100, 255}},  
        {5,  {255, 120, 120, 255}},  
        {4,  {255, 150,  80, 255}},  
        {3,  {255, 180,  40, 255}},  
        {2,  {255, 210,   0, 255}},  
        {1,  { 80, 255,  80, 255}},  
    };


public:
    ResultsScreen(Renderer& renderer, int width, int height);

    void show(const std::vector<ServerMessage::PlayerRaceTime>& raceTimes,
              const std::vector<ServerMessage::PlayerTotalTime>& totalTimes,
              int32_t mainPlayerId,
              uint8_t upgrade_speed,
              uint8_t upgrade_acceleration,
              uint8_t upgrade_handling,
              uint8_t upgrade_durability);
    void hide();
    bool isVisible() const { return visible; }
    bool shouldAutoDismiss() const;

    void updateUpgrades(uint8_t upgrade_speed,
                        uint8_t upgrade_acceleration,
                        uint8_t upgrade_handling,
                        uint8_t upgrade_durability);

    void startCountdown(int seconds);
    void updateCountdown();
    bool isCountdownActive() const { return countdownActive; }
    void renderCountDown(Renderer& renderer);


    void render(Renderer& renderer);
};

#endif // RESULTS_SCREEN_H
