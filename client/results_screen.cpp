#include "results_screen.h"
#include "carsprites.h"
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <SDL_ttf.h>

ResultsScreen::ResultsScreen(Renderer& renderer, int width, int height)
    : visible(false),
      screenWidth(width),
      screenHeight(height)
{
    if (TTF_Init() == -1) {
        std::cerr << "[ResultsScreen] Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
    }

    try {
        contentFont = std::make_unique<Font>("data/fonts/race-font.otf", 20);
        headerFont = std::make_unique<Font>("data/fonts/race-font.otf", 32);
    } catch (const std::exception& e) {
        std::cerr << "[ResultsScreen] Warning: Could not load fonts: " << e.what() << std::endl;
    }

    const std::array<std::string, 8> positionFileNames = {
        "1st-place.png", "2nd-place.png", "3th-place.png", "4th-place.png",
        "5th-place.png", "6th-place.png", "7th-place.png", "8th-place.png"
    };

    for (size_t i = 0; i < positionFileNames.size(); ++i) {
        try {
            std::string path = "data/positions/" + positionFileNames[i];
            positionImages[i] = std::make_unique<Texture>(renderer, Surface(path));
        } catch (const std::exception& e) {
            std::cerr << "[ResultsScreen] Warning: Could not load position image " << positionFileNames[i]
                      << ": " << e.what() << std::endl;
        }
    }

    try {
        Surface tallerSurface("data/cars/taller.png");
        tallerSurface.SetColorKey(true, SDL_MapRGB(tallerSurface.Get()->format, 0x55, 0x55, 0x55));
        tallerTexture = std::make_unique<Texture>(renderer, std::move(tallerSurface));
    } catch (const std::exception& e) {
        std::cerr << "[ResultsScreen] Warning: Could not load taller sprite sheet: " << e.what() << std::endl;
    }
}

void ResultsScreen::show(const std::vector<ServerMessage::PlayerRaceTime>& raceTimes,
                         const std::vector<ServerMessage::PlayerTotalTime>& totalTimes,
                         int32_t mainPlayerId,
                         uint8_t upgrade_speed,
                         uint8_t upgrade_acceleration,
                         uint8_t upgrade_handling,
                         uint8_t upgrade_durability)
{
    visible = true;
    startTime = std::chrono::steady_clock::now();
    raceResults = raceTimes;
    totalResults = totalTimes;
    this->mainPlayerId = mainPlayerId;
    this->updateUpgrades(upgrade_speed, upgrade_acceleration, upgrade_handling, upgrade_durability);
}

void ResultsScreen::hide()
{
    visible = false;
}

void ResultsScreen::updateUpgrades(uint8_t upgrade_speed,
                                    uint8_t upgrade_acceleration,
                                    uint8_t upgrade_handling,
                                    uint8_t upgrade_durability)
{
    this->upgradeSpeed = upgrade_speed;
    this->upgradeAcceleration = upgrade_acceleration;
    this->upgradeHandling = upgrade_handling;
    this->upgradeDurability = upgrade_durability;
}

bool ResultsScreen::shouldAutoDismiss() const
{
    if (!visible) {
        return false;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();

    return elapsed >= RESULTS_DISPLAY_DURATION_MS;
}

std::string ResultsScreen::formatTime(uint32_t ms)
{
    uint32_t minutes = ms / 60000;
    uint32_t seconds = (ms % 60000) / 1000;
    uint32_t millis = ms % 1000;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%u:%02u.%03u", minutes, seconds, millis);
    return std::string(buffer);
}

void ResultsScreen::renderBackground(Renderer& renderer)
{
    renderer.SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    renderer.SetDrawColor(0, 0, 0, OVERLAY_ALPHA);
    renderer.FillRect(Rect(0, 0, screenWidth, screenHeight));
}

void ResultsScreen::renderContent(Renderer& renderer)
{
    if (!contentFont || !headerFont) {
        std::cerr << "[ResultsScreen] Fonts not loaded - cannot render results text" << std::endl;
        std::cerr << "[ResultsScreen] Race results: " << raceResults.size()
                  << " | Total results: " << totalResults.size() << std::endl;
        return;
    }

    constexpr int LEFT_COLUMN_X = 150;
    constexpr int RIGHT_COLUMN_X = 500;
    constexpr int START_Y = 150;
    constexpr int ROW_SPACING = 45;
    constexpr int POSITION_IMAGE_WIDTH = 100;
    constexpr int POSITION_IMAGE_HEIGHT = 25;


    try {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Color greyedYellow = {180, 180, 0, 255};
        SDL_Color grey = {150, 150, 150, 255};

        std::string headerText = "=== RACE RESULTS ===";
        Surface headerSurface = headerFont->RenderText_Blended(headerText, white);
        Texture headerTexture(renderer, headerSurface);
        int headerX = (screenWidth - headerTexture.GetWidth()) / 2;
        renderer.Copy(headerTexture, NullOpt, Rect(headerX, 80, headerTexture.GetWidth(), headerTexture.GetHeight()));

        if (!raceResults.empty()) {
            std::string roundText = "Round " + std::to_string(raceResults[0].round_index + 1);
            Surface roundSurface = contentFont->RenderText_Blended(roundText, white);
            Texture roundTexture(renderer, roundSurface);
            int roundX = (screenWidth - roundTexture.GetWidth()) / 2;
            renderer.Copy(roundTexture, NullOpt, Rect(roundX, 120, roundTexture.GetWidth(), roundTexture.GetHeight()));
        }

        std::vector<ServerMessage::PlayerRaceTime> sortedRaceTimes = raceResults;
        std::sort(sortedRaceTimes.begin(), sortedRaceTimes.end(),
            [](const ServerMessage::PlayerRaceTime& a, const ServerMessage::PlayerRaceTime& b) {
                if (a.disqualified != b.disqualified) return b.disqualified; 
                return a.time_ms < b.time_ms; 
            });

        std::vector<ServerMessage::PlayerTotalTime> sortedTotals = totalResults;
        std::sort(sortedTotals.begin(), sortedTotals.end(),
            [](const ServerMessage::PlayerTotalTime& a, const ServerMessage::PlayerTotalTime& b) {
                return a.total_ms < b.total_ms;
            });

        if (!sortedRaceTimes.empty()) {
            Surface leftHeaderSurface = contentFont->RenderText_Blended("RACE TIMES", white);
            Texture leftHeaderTexture(renderer, leftHeaderSurface);
            renderer.Copy(leftHeaderTexture, NullOpt, Rect(LEFT_COLUMN_X, START_Y - 30, leftHeaderTexture.GetWidth(), leftHeaderTexture.GetHeight()));
        }

        if (!sortedTotals.empty()) {
            Surface rightHeaderSurface = contentFont->RenderText_Blended("STANDINGS", white);
            Texture rightHeaderTexture(renderer, rightHeaderSurface);
            renderer.Copy(rightHeaderTexture, NullOpt, Rect(RIGHT_COLUMN_X, START_Y - 30, rightHeaderTexture.GetWidth(), rightHeaderTexture.GetHeight()));
        }

        for (size_t i = 0; i < sortedRaceTimes.size() && i < 8; ++i) {
            int y = START_Y + i * ROW_SPACING;

            if (positionImages[i]) {
                renderer.Copy(*positionImages[i], NullOpt,
                    Rect(LEFT_COLUMN_X, y, POSITION_IMAGE_WIDTH, POSITION_IMAGE_HEIGHT));
            }

            std::string text = "P" + std::to_string(sortedRaceTimes[i].player_id) + ": " +
                               formatTime(sortedRaceTimes[i].time_ms);
            if (sortedRaceTimes[i].disqualified) {
                text += " (DQ)";
            }

            SDL_Color color;
            bool isMainPlayer = (static_cast<int32_t>(sortedRaceTimes[i].player_id) == mainPlayerId);
            bool isDQ = sortedRaceTimes[i].disqualified;

            if (isMainPlayer && isDQ) {
                color = greyedYellow;
            } else if (isMainPlayer) {
                color = yellow;
            } else if (isDQ) {
                color = grey;
            } else {
                color = white;
            }

            Surface textSurface = contentFont->RenderText_Blended(text, color);
            Texture textTexture(renderer, textSurface);
            renderer.Copy(textTexture, NullOpt,
                Rect(LEFT_COLUMN_X + POSITION_IMAGE_WIDTH + 10, y, textTexture.GetWidth(), textTexture.GetHeight()));
        }

        for (size_t i = 0; i < sortedTotals.size() && i < 8; ++i) {
            int y = START_Y + i * ROW_SPACING;

            if (positionImages[i]) {
                renderer.Copy(*positionImages[i], NullOpt,
                    Rect(RIGHT_COLUMN_X, y, POSITION_IMAGE_WIDTH, POSITION_IMAGE_HEIGHT));
            }

            std::string text = "P" + std::to_string(sortedTotals[i].player_id) + ": " +
                               formatTime(sortedTotals[i].total_ms);

            SDL_Color color = (static_cast<int32_t>(sortedTotals[i].player_id) == mainPlayerId) ? yellow : white;

            Surface textSurface = contentFont->RenderText_Blended(text, color);
            Texture textTexture(renderer, textSurface);
            renderer.Copy(textTexture, NullOpt,
                Rect(RIGHT_COLUMN_X + POSITION_IMAGE_WIDTH + 10, y, textTexture.GetWidth(), textTexture.GetHeight()));
        }

    } catch (const std::exception& e) {
        std::cerr << "[ResultsScreen] Error rendering results content: " << e.what() << std::endl;
    }
}

void ResultsScreen::renderUpgradeIcons(Renderer& renderer)
{
    if (!tallerTexture) return;

    try {
        static constexpr int ICON_SIZE     = 60;
        static constexpr int RIGHT_MARGIN  = 20;
        static constexpr int ICON_SPACING  = 10;
        static constexpr int MAX_DOTS      = 3;
        static constexpr int DOT_SIZE      = 12;
        static constexpr int DOT_OFFSET_X  = 5;
        static constexpr int DOT_OFFSET_Y  = 12;
        static constexpr int DOT_SPACING   = 18;

        const int startX = screenWidth - RIGHT_MARGIN - ICON_SIZE;
        int currentY = 100;  

        const std::array upgrades{
            std::pair{0, upgradeAcceleration},
            std::pair{1, upgradeSpeed},
            std::pair{2, upgradeHandling},
            std::pair{3, upgradeDurability}
        };

        for (auto [spriteIndex, level] : upgrades) {

            renderer.Copy(
                *tallerTexture,
                TALLER_SPRITES[spriteIndex],
                Rect(startX, currentY, ICON_SIZE, ICON_SIZE)
            );

            renderer.SetDrawColor(0, 255, 0, 255);
            const uint8_t dotCount = std::min<uint8_t>(level, MAX_DOTS);

            for (uint8_t i = 0; i < dotCount; ++i) {
                const int dotX = startX + DOT_OFFSET_X + (i * DOT_SPACING);
                const int dotY = currentY + ICON_SIZE - DOT_OFFSET_Y;

                renderer.FillRect(Rect(dotX, dotY, DOT_SIZE, DOT_SIZE));
            }

            currentY += ICON_SIZE + ICON_SPACING;
        }

    } catch (const std::exception& e) {
        std::cerr << "[ResultsScreen] Error rendering upgrade icons: " << e.what() << std::endl;
    }
}


void ResultsScreen::render(Renderer& renderer)
{
    if (!visible) {
        return;
    }

    renderBackground(renderer);
    renderContent(renderer);
    renderUpgradeIcons(renderer);
}

void ResultsScreen::renderCountDown(Renderer& renderer)
{
    if (!countdownActive) return;
    if (!headerFont) return;

    std::string text = std::to_string(currentCountdown);

    Surface surface = headerFont->RenderText_Blended(text, color_map.at(currentCountdown));
    Texture texture(renderer, surface);

    int x = (screenWidth - texture.GetWidth()) / 2;
    int y = 35;

    renderer.Copy(texture, NullOpt,
                  Rect(x, y, texture.GetWidth(), texture.GetHeight()));
}


void ResultsScreen::startCountdown(int seconds) {
    initialCountdown = seconds;
    currentCountdown = seconds;
    countdownStartTime = SDL_GetTicks();
    countdownActive = true;
}

void ResultsScreen::updateCountdown() {
    if (!countdownActive) return;

    Uint32 elapsed = (SDL_GetTicks() - countdownStartTime) / 1000;

    currentCountdown = initialCountdown - elapsed;

    if (currentCountdown <= 0) {
        currentCountdown = 0;
        countdownActive = false;
    }
}