// =============================================================================
//  core/Player.cpp  -  Implementacao (ver Player.hpp)
// =============================================================================
#include "core/Player.hpp"

Player::Player(PlayerView &v) : view(v) {}

void Player::begin(const TrackSource &src, uint32_t nowMs)
{
    source = &src;
    now = nowMs;
    playing = false;
    currentMode = PlayerMode::Normal;

    loadTrack(0);
    view.volumeChanged(currentVolume);
    notifyStatus();
}

// -----------------------------------------------------------------------------
//  Tempo
// -----------------------------------------------------------------------------

void Player::update(uint32_t nowMs)
{
    now = nowMs;
    if (!playing) {
        return;
    }

    // Conta todos os segundos inteiros que passaram (o loop pode atrasar,
    // por exemplo enquanto uma capa e decodificada).
    while (playing && now - lastSecond >= 1000) {
        lastSecond += 1000;
        tickSecond();
    }
}

void Player::tickSecond()
{
    if (elapsedSeconds < info.duration) {
        elapsedSeconds++;
        notifyProgress();
    }
    if (elapsedSeconds >= info.duration) {
        finishTrack();
    }
}

void Player::finishTrack()
{
    if (repeatMode == RepeatMode::One) {
        restartTrack();
        return;
    }

    bool wasLast = index + 1 >= source->count();
    loadTrack(index + 1);   // volta para a primeira depois da ultima

    if (wasLast && repeatMode == RepeatMode::Off) {
        playing = false;    // fim da playlist
        notifyStatus();
    }
}

// -----------------------------------------------------------------------------
//  Troca de musica
// -----------------------------------------------------------------------------

void Player::loadTrack(size_t newIndex)
{
    index = newIndex % source->count();
    source->load(index, info);
    elapsedSeconds = 0;
    lastSecond = now;

    view.trackChanged(*source, index, info);
    notifyProgress();
}

void Player::restartTrack()
{
    elapsedSeconds = 0;
    lastSecond = now;
    notifyProgress();
}

bool Player::next()
{
    if (currentMode != PlayerMode::Normal) {
        return false;
    }
    loadTrack(index + 1);
    return true;
}

bool Player::previous()
{
    if (currentMode != PlayerMode::Normal) {
        return false;
    }

    // Ja tocou um pouco: volta ao inicio. Acabou de comecar: musica anterior.
    if (elapsedSeconds > RESTART_THRESHOLD) {
        restartTrack();
    } else {
        loadTrack(index == 0 ? source->count() - 1 : index - 1);
    }
    return true;
}

void Player::playTrack(size_t newIndex)
{
    loadTrack(newIndex);
    playing = true;
    lastSecond = now;
    notifyStatus();
}

void Player::togglePlay()
{
    playing = !playing;
    lastSecond = now;   // o proximo segundo comeca a contar agora
    notifyStatus();
}

// -----------------------------------------------------------------------------
//  Modos e encoder
// -----------------------------------------------------------------------------

bool Player::toggleVolumeMode()
{
    if (currentMode == PlayerMode::Position) {
        return false;
    }
    currentMode = (currentMode == PlayerMode::Volume) ? PlayerMode::Normal : PlayerMode::Volume;
    notifyStatus();
    return true;
}

bool Player::togglePositionMode()
{
    if (currentMode == PlayerMode::Volume) {
        return false;
    }
    currentMode = (currentMode == PlayerMode::Position) ? PlayerMode::Normal : PlayerMode::Position;
    notifyStatus();
    return true;
}

void Player::resetMode()
{
    if (currentMode != PlayerMode::Normal) {
        currentMode = PlayerMode::Normal;
        notifyStatus();
    }
}

void Player::rotate(int delta)
{
    if (currentMode == PlayerMode::Volume) {
        setVolume(currentVolume + delta);
    } else if (currentMode == PlayerMode::Position) {
        seek(delta);
    }
}

void Player::setVolume(int value)
{
    if (value < VOLUME_MIN) value = VOLUME_MIN;
    if (value > VOLUME_MAX) value = VOLUME_MAX;
    currentVolume = value;
    view.volumeChanged(currentVolume);
}

void Player::seek(int steps)
{
    int64_t target = (int64_t)elapsedSeconds + (int64_t)steps * POSITION_STEP;
    if (target < 0) target = 0;
    if (target > info.duration) target = info.duration;
    elapsedSeconds = (uint32_t)target;
    notifyProgress();
}

void Player::cycleRepeat()
{
    repeatMode = (repeatMode == RepeatMode::Off) ? RepeatMode::All :
                 (repeatMode == RepeatMode::All) ? RepeatMode::One : RepeatMode::Off;
}
