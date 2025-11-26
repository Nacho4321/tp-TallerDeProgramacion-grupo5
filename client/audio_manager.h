#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SDL2pp/SDL2pp.hh>
#include <memory>
#include <string>
#include <map>
#include <set>

class AudioManager {
private:
    std::unique_ptr<SDL2pp::Mixer> mixer;
    std::unique_ptr<SDL2pp::Music> backgroundMusic;

    std::unique_ptr<SDL2pp::Chunk> explosionSound;
    std::unique_ptr<SDL2pp::Chunk> collisionSound;
    std::unique_ptr<SDL2pp::Chunk> engineSound;

    std::map<int, int> carEngineChannels;
    int mainCarEngineChannel;
    int masterVolume = 64;

    static constexpr float MAX_HEARING_DISTANCE_EXPLOSION = 1000.0f;
    static constexpr float MAX_HEARING_DISTANCE_COLLISION = 400.0f;
    static constexpr float MAX_HEARING_DISTANCE_ENGINE = 400.0f;
    static constexpr int MAIN_CAR_ENGINE_VOLUME = 20;           
    static constexpr int BACKGROUND_MUSIC_VOLUME = 40;     
    static constexpr int MAX_VOLUME = 40;
    static constexpr int VOLUME_STEP = 8;
    static constexpr int MIN_VOLUME = 0;
    static constexpr int MAX_VOLUME_LIMIT = 128;

    void loadSoundEffects();
    int calculateVolume(float soundX, float soundY, float listenerX, float listenerY, float maxDistance);
    int allocateChannel();
    void applyMasterVolume();

public:
    AudioManager();
    ~AudioManager();

    void playBackgroundMusic(const std::string& musicPath);
    void stopMusic();
    void setMusicVolume(int volume);  // 0-128

    void playExplosionSound(float worldX, float worldY, float listenerX, float listenerY);
    void playCollisionSound(float worldX, float worldY, float listenerX, float listenerY);

    void startCarEngine(int carId, float worldX, float worldY, float listenerX, float listenerY, bool isMainCar = false);
    void updateCarEngineVolume(int carId, float worldX, float worldY, float listenerX, float listenerY);
    void stopCarEngine(int carId);
    bool isEnginePlayingForCar(int carId) const;

    void stopEnginesExcept(const std::set<int>& activeCarIds);

    void increaseMasterVolume();
    void decreaseMasterVolume();
    int getMasterVolume() const { return masterVolume; }
};

#endif // AUDIO_MANAGER_H
