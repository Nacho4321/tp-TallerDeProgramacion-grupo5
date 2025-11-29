#include "audio_manager.h"
#include <iostream>
#include <cmath>

int BACKGROUND_MUSIC_VOLUME = 40;

AudioManager::AudioManager() : mainCarEngineChannel(-1) {
    try {
        mixer = std::make_unique<SDL2pp::Mixer>(
            MIX_DEFAULT_FREQUENCY,  // 22050 Hz
            MIX_DEFAULT_FORMAT,     // 16-bit
            2,                      // stereo
            2048                    // chunk size
        );

        mixer->AllocateChannels(16);

        loadSoundEffects();

    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to initialize audio mixer: "
                  << e.what() << std::endl;
        mixer.reset();
    }
}

AudioManager::~AudioManager() {
    for (const auto& [carId, channel] : carEngineChannels) {
        if (mixer) {
            mixer->HaltChannel(channel);
        }
    }
    carEngineChannels.clear();

    if (mixer && mainCarEngineChannel >= 0) {
        mixer->HaltChannel(mainCarEngineChannel);
    }
}

void AudioManager::loadSoundEffects() {
    if (!mixer) return;

    try {
        explosionSound = std::make_unique<SDL2pp::Chunk>("data/sounds/explosion.wav");
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to load explosion.wav: " << e.what() << std::endl;
    }

    try {
        collisionSound = std::make_unique<SDL2pp::Chunk>("data/sounds/collision.wav");
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to load collision.wav: " << e.what() << std::endl;
    }

    try {
        engineSound = std::make_unique<SDL2pp::Chunk>("data/sounds/engine_loop.wav");
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to load engine_loop.wav: " << e.what() << std::endl;
    }
}

int AudioManager::calculateVolume(float soundX, float soundY,
                                   float listenerX, float listenerY,
                                   float maxDistance) {
    float dx = soundX - listenerX;
    float dy = soundY - listenerY;
    float distanceSquared = dx * dx + dy * dy;
    float maxDistanceSquared = maxDistance * maxDistance;

    if (distanceSquared >= maxDistanceSquared) {
        return 0;
    }

    float distance = std::sqrt(distanceSquared);
    float ratio = 1.0f - (distance / maxDistance);
    float volumeRatio = ratio * ratio;  

    return static_cast<int>(MAX_VOLUME * volumeRatio);
}

int AudioManager::allocateChannel() {
    if (!mixer) return -1;

    for (int i = 1; i < 16; i++) {
        if (!mixer->IsChannelPlaying(i)) {
            return i;
        }
    }

    return mixer->GetGroupOldestChannel(-1); 
}

void AudioManager::playBackgroundMusic(const std::string& musicPath) {
    if (!mixer) {
        std::cerr << "Warning: Mixer not initialized, cannot play music" << std::endl;
        return;
    }

    try {
        backgroundMusic = std::make_unique<SDL2pp::Music>(musicPath);
        mixer->PlayMusic(*backgroundMusic, -1);  
        mixer->SetMusicVolume(BACKGROUND_MUSIC_VOLUME);  
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to load/play music: "
                  << e.what() << std::endl;
        backgroundMusic.reset();
    }
}

void AudioManager::stopMusic() {
    if (mixer) {
        mixer->HaltMusic();
    }
    backgroundMusic.reset();
}

void AudioManager::setMusicVolume(int volume) {
    if (mixer) {
        mixer->SetMusicVolume(volume);
    }
}

void AudioManager::playExplosionSound(float worldX, float worldY,
                                       float listenerX, float listenerY) {
    if (!mixer || !explosionSound) return;

    int volume = calculateVolume(worldX, worldY, listenerX, listenerY, MAX_HEARING_DISTANCE_EXPLOSION);
    if (volume == 0) return;

    try {
        int channel = allocateChannel();
        if (channel >= 0) {
            int scaledVolume = (volume * 2 * masterVolume) / 128;
            explosionSound->SetVolume(scaledVolume);
            mixer->PlayChannel(channel, *explosionSound, 0);  
        }
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to play explosion sound: " << e.what() << std::endl;
    }
}

void AudioManager::playCollisionSound(float worldX, float worldY,
                                       float listenerX, float listenerY) {
    if (!mixer || !collisionSound) return;

    int volume = calculateVolume(worldX, worldY, listenerX, listenerY,
                                  MAX_HEARING_DISTANCE_COLLISION);
    if (volume == 0) return;  

    try {
        int channel = allocateChannel();
        if (channel >= 0) {
            int scaledVolume = (volume * masterVolume) / 128;
            collisionSound->SetVolume(scaledVolume);
            mixer->PlayChannel(channel, *collisionSound, 0); 
        }
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to play collision sound: " << e.what() << std::endl;
    }
}

void AudioManager::startCarEngine(int carId, float worldX, float worldY,
                                   float listenerX, float listenerY, bool isMainCar) {
    if (!mixer || !engineSound) return;

    if (carEngineChannels.count(carId) > 0) {
        updateCarEngineVolume(carId, worldX, worldY, listenerX, listenerY);
        return;
    }

    try {
        int channel;
        int volume;

        if (isMainCar) {
            channel = 0;
            volume = MAIN_CAR_ENGINE_VOLUME;
            mainCarEngineChannel = channel;
        } else {
            channel = allocateChannel();
            if (channel < 0) return; 

            volume = calculateVolume(worldX, worldY, listenerX, listenerY,
                                      MAX_HEARING_DISTANCE_ENGINE);
            if (volume == 0) return;  
        }

        mixer->PlayChannel(channel, *engineSound, -1);  
        int scaledVolume = (volume * masterVolume) / 128;
        mixer->SetVolume(channel, scaledVolume);  
        carEngineChannels[carId] = channel;

    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to start engine sound for car " << carId
                  << ": " << e.what() << std::endl;
    }
}

void AudioManager::updateCarEngineVolume(int carId, float worldX, float worldY,
                                          float listenerX, float listenerY) {
    if (!mixer) return;

    auto it = carEngineChannels.find(carId);
    if (it == carEngineChannels.end()) return;

    int channel = it->second;

    if (carId == -1 || channel == mainCarEngineChannel) {
        int scaledVolume = (MAIN_CAR_ENGINE_VOLUME * masterVolume) / 128;
        mixer->SetVolume(channel, scaledVolume);
        return;
    }

    int volume = calculateVolume(worldX, worldY, listenerX, listenerY,
                                  MAX_HEARING_DISTANCE_ENGINE);

    if (volume == 0) {
        stopCarEngine(carId);
        return;
    }

    try {
        int scaledVolume = (volume * masterVolume * 0.25) / 128;
        mixer->SetVolume(channel, scaledVolume);
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to update engine volume for car " << carId
                  << ": " << e.what() << std::endl;
    }
}

void AudioManager::stopCarEngine(int carId) {
    if (!mixer) return;

    auto it = carEngineChannels.find(carId);
    if (it == carEngineChannels.end()) return;

    int channel = it->second;

    try {
        mixer->HaltChannel(channel);
        carEngineChannels.erase(it);
    } catch (const SDL2pp::Exception& e) {
        std::cerr << "Warning: Failed to stop engine sound for car " << carId
                  << ": " << e.what() << std::endl;
    }
}

bool AudioManager::isEnginePlayingForCar(int carId) const {
    return carEngineChannels.count(carId) > 0;
}

void AudioManager::stopEnginesExcept(const std::set<int>& activeCarIds) {
    if (!mixer) return;

    std::vector<int> carsToStop;
    for (const auto& [carId, channel] : carEngineChannels) {
        if (carId != -1 && activeCarIds.count(carId) == 0) {
            carsToStop.push_back(carId);
        }
    }

    for (int carId : carsToStop) {
        stopCarEngine(carId);
    }
}

void AudioManager::increaseMasterVolume() {
    masterVolume = std::min(masterVolume + VOLUME_STEP, MAX_VOLUME_LIMIT);
    applyMasterVolume();
    std::cout << "[Audio] Master volume: " << masterVolume << "/128" << std::endl;
}

void AudioManager::decreaseMasterVolume() {
    masterVolume = std::max(masterVolume - VOLUME_STEP, MIN_VOLUME);
    applyMasterVolume();
    std::cout << "[Audio] Master volume: " << masterVolume << "/128" << std::endl;
}

void AudioManager::applyMasterVolume() {
    if (mixer && backgroundMusic) {
        int scaledMusicVol = (BACKGROUND_MUSIC_VOLUME * masterVolume) / 128;
        mixer->SetMusicVolume(scaledMusicVol);
    }
}
