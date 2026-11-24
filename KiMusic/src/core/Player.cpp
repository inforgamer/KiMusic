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

    buildOrder(0);
    loadAt(0);
    if (audio) audio->setVolume(currentVolume);
    view.volumeChanged(currentVolume);
    notifyStatus();
}

// -----------------------------------------------------------------------------
//  Ordem de reproducao
// -----------------------------------------------------------------------------

uint32_t Player::nextRandom()
{
    // xorshift32: simples, rapido e reproduzivel nos testes.
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

void Player::buildOrder(size_t first)
{
    const size_t count = source->count();
    order.resize(count);
    for (size_t i = 0; i < count; i++) {
        order[i] = i;
    }
    position = 0;
    if (count == 0) {
        return;
    }
    first %= count;

    if (shuffleMode == ShuffleMode::Off) {
        position = first;
        return;
    }

    // Embaralhado: a musica 'first' fica na frente e o resto e sorteado
    // (Fisher-Yates nas posicoes 1..count-1).
    order[0] = first;
    order[first] = 0;
    for (size_t i = count - 1; i > 1; i--) {
        size_t j = 1 + nextRandom() % i;   // j em [1, i]
        size_t tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
}

void Player::setShuffle(ShuffleMode mode, const TrackSource *all)
{
    if (mode == ShuffleMode::All && all && all != source && all->count() > 0) {
        // Troca para todas as musicas, mantendo a que esta tocando se ela existir la.
        size_t keep = 0;
        bool found = false;
        for (size_t i = 0; i < all->count() && !currentPath.empty(); i++) {
            if (all->path(i) == currentPath) {
                keep = i;
                found = true;
                break;
            }
        }
        source = all;
        shuffleMode = mode;
        buildOrder(keep);
        if (!found) {
            loadAt(0);     // a musica atual nao esta na lista geral: comeca outra
        }
        return;
    }

    shuffleMode = mode;
    buildOrder(trackIndex());   // a musica atual continua tocando, so a ordem muda
}

void Player::setSource(const TrackSource &src, size_t startIndex, bool play)
{
    if (src.count() == 0) {
        return;
    }
    source = &src;
    if (shuffleMode == ShuffleMode::All) {
        shuffleMode = ShuffleMode::Playlist;   // escolher uma lista sai do "geral"
    }
    buildOrder(startIndex);
    playing = play;
    loadAt(position);
    notifyStatus();
}

// -----------------------------------------------------------------------------
//  Tempo
// -----------------------------------------------------------------------------

void Player::update(uint32_t nowMs)
{
    now = nowMs;

    // Salto de posicao adiado: so reabre o audio quando o encoder parou.
    if (seekPending && now - seekRequestedAt >= SEEK_DELAY_MS) {
        seekPending = false;
        if (playing) {
            audioPlayFrom(elapsedSeconds);
        }
    }

    if (!playing) {
        return;
    }

    if (usesAudio()) {
        if (audioError) {               // arquivo nao abriu: pula
            audioError = false;
            finishTrack();
            return;
        }
        if (!audioLoaded || seekPending) {
            return;
        }
        if (audio->finished()) {
            finishTrack();
            return;
        }
        uint32_t pos = audio->position();
        if (pos > info.duration && info.duration > 0) pos = info.duration;
        if (pos != elapsedSeconds) {
            elapsedSeconds = pos;
            notifyProgress();
        }
        return;
    }

    // Relogio simulado: conta todos os segundos inteiros que passaram.
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

    bool wasLast = position + 1 >= order.size();
    if (wasLast && shuffleMode != ShuffleMode::Off) {
        buildOrder(nextRandom() % order.size());   // nova volta embaralhada
        if (repeatMode == RepeatMode::Off) {
            playing = false;
        }
        loadAt(0);
    } else {
        if (wasLast && repeatMode == RepeatMode::Off) {
            playing = false;   // fim da lista: volta para a primeira e pausa
        }
        loadAt(position + 1);
    }

    if (!playing) {
        notifyStatus();
    }
}

// -----------------------------------------------------------------------------
//  Troca de musica
// -----------------------------------------------------------------------------

void Player::loadAt(size_t orderPosition)
{
    position = order.empty() ? 0 : orderPosition % order.size();
    size_t index = trackIndex();

    source->load(index, info);
    currentPath = source->path(index);
    elapsedSeconds = 0;
    lastSecond = now;
    seekPending = false;

    view.trackChanged(*source, index, info);
    notifyProgress();

    if (playing) {
        audioPlayFrom(0);
    } else {
        audioHalt();
    }
}

void Player::restartTrack()
{
    elapsedSeconds = 0;
    lastSecond = now;
    seekPending = false;
    notifyProgress();
    if (playing) {
        audioPlayFrom(0);
    } else {
        audioHalt();
    }
}

bool Player::next()
{
    if (currentMode != PlayerMode::Normal) {
        return false;
    }
    if (position + 1 >= order.size() && shuffleMode != ShuffleMode::Off) {
        buildOrder(nextRandom() % order.size());
        loadAt(0);
    } else {
        loadAt(position + 1);
    }
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
        loadAt(position == 0 ? order.size() - 1 : position - 1);
    }
    return true;
}

void Player::playTrack(size_t index)
{
    playing = true;
    if (shuffleMode != ShuffleMode::Off) {
        buildOrder(index);        // a escolhida vira a primeira da nova ordem
        loadAt(0);
    } else {
        loadAt(index);
    }
    notifyStatus();
}

void Player::togglePlay()
{
    playing = !playing;
    lastSecond = now;   // relogio simulado: o proximo segundo comeca a contar agora

    if (usesAudio()) {
        if (playing) {
            if (audioLoaded && !seekPending) {
                audio->resume();
            } else {
                seekPending = false;              // ja abre na posicao atual
                audioPlayFrom(elapsedSeconds);
            }
        } else if (audioLoaded) {
            audio->pause();
        }
    }
    notifyStatus();
}

// -----------------------------------------------------------------------------
//  Audio
// -----------------------------------------------------------------------------

void Player::audioPlayFrom(uint32_t seconds)
{
    if (!usesAudio()) {
        audioLoaded = false;
        return;
    }
    audioLoaded = audio->start(currentPath, info, seconds);
    audioError = !audioLoaded;
}

void Player::audioHalt()
{
    if (audio) {
        audio->stop();
    }
    audioLoaded = false;
    audioError = false;
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
    if (audio) audio->setVolume(currentVolume);
    view.volumeChanged(currentVolume);
}

void Player::seek(int steps)
{
    int64_t target = (int64_t)elapsedSeconds + (int64_t)steps * POSITION_STEP;
    if (target < 0) target = 0;
    if (target > info.duration) target = info.duration;
    elapsedSeconds = (uint32_t)target;
    lastSecond = now;
    notifyProgress();

    // O audio so salta depois que o encoder parar (ver update()).
    if (usesAudio()) {
        seekPending = true;
        seekRequestedAt = now;
        if (!playing) {
            audioHalt();   // pausado: na volta do Play o audio abre ja na nova posicao
        }
    }
}

void Player::cycleRepeat()
{
    repeatMode = (repeatMode == RepeatMode::Off) ? RepeatMode::All :
                 (repeatMode == RepeatMode::All) ? RepeatMode::One : RepeatMode::Off;
}
