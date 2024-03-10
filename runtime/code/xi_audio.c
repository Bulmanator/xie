FileScope void AudioPlayerInit(M_Arena *arena, AudioPlayer *player) {
    switch (player->sample_rate) {
        case 8000:
        case 11025:
        case 22050:
        case 32000:
        case 44100:
        case 48000:
        case 96000: {} break;
        default: {
            player->sample_rate = 48000;
        }
        break;
    }

    if (player->frame_count == 0) {
        // 50ms
        //
        player->frame_count = cast(U32) ((0.05f * player->sample_rate) + 0.5f);
    }

    if (player->music.layer_limit == 0) { player->music.layer_limit = 4;  }
    if (player->sfx.limit         == 0) { player->sfx.limit         = 32; }

    U32 event_limit = player->music.layer_limit + player->sfx.limit;

    // @todo: this is also bad the whole audio system is kinda bad implementation wise, just use
    // linked lists instead of fixed arrays
    //

    player->music.layers = M_ArenaPush(arena, PlayingSound, player->music.layer_limit);
    player->sfx.sounds   = M_ArenaPush(arena, PlayingSound, player->sfx.limit);
    player->events       = M_ArenaPush(arena, AudioEvent,   event_limit);

    for (U32 it = 0; it < player->sfx.limit - 1; ++it) {
        player->sfx.sounds[it].next = it + 1;
    }

    player->music.layer_count = 0;
    player->sfx.count         = 0;
    player->sfx.next_free     = 0;
}

FileScope void AudioEventPush(AudioPlayer *player, U32 type, U32 tag, B32 from_music, U32 index, SoundHandle handle) {
    U32 event_limit = (player->music.layer_limit + player->sfx.limit);
    Assert(player->event_count < event_limit);

    AudioEvent *event    = &player->events[player->event_count];
    player->event_count += 1;

    event->type       = type;
    event->tag        = tag;
    event->from_music = from_music;
    event->index      = index;
    event->handle     = handle;
}

void SoundEffectPlay(AudioPlayer *player, SoundHandle handle, U32 tag, F32 volume) {
    if (player->sfx.count < player->sfx.limit) {
        PlayingSound *sound   = &player->sfx.sounds[player->sfx.next_free];
        player->sfx.next_free = sound->next;

        sound->enabled = true;

        sound->handle = handle;
        sound->tag    = tag;
        sound->sample = 0;
        sound->volume = volume;
        sound->target_volume = 0; // @todo: maybe we should use this for effects as well

        AudioEventPush(player, AUDIO_EVENT_TYPE_STARTED, tag, false, 0, handle);
    }
}

U32 MusicLayerAdd(AudioPlayer *player, SoundHandle handle, U32 tag) {
    U32 result = U32_MAX;

    if (player->music.layer_count < player->music.layer_limit) {
        result = player->music.layer_count;
        PlayingSound *layer = &player->music.layers[result];

        layer->enabled = false;
        layer->handle  = handle;
        layer->sample  = 0;
        layer->tag     = tag;
        layer->volume  = 0.0f;

        player->music.layer_count += 1;
    }

    return result;
}

void MusicLayerEnableByIndex(AudioPlayer *player, U32 index, MusicLayerEffect effect, F32 rate) {
    if (index < player->music.layer_count) {
        PlayingSound *sound = &player->music.layers[index];

        if (rate <= 0) { rate = 1; }

        if (!sound->enabled) {
            sound->enabled       = true;
            sound->volume        = (effect == MUSIC_LAYER_EFFECT_FADE) ? 0.0f : 1.0f;
            sound->target_volume = 1.0f;
            sound->rate          = rate;

            AudioEventPush(player, AUDIO_EVENT_TYPE_STARTED, sound->tag, true, index, sound->handle);
        }
    }
}

void MusicLayerToggleByIndex(AudioPlayer *player, U32 index, MusicLayerEffect effect, F32 rate) {
    if (index < player->music.layer_count) {
        PlayingSound *sound = &player->music.layers[index];

        if (rate <= 0) { rate = 1; }

        if (sound->enabled) {
            sound->enabled       = false;
            sound->target_volume = 0.0f;
            sound->rate          = -rate;

            if (effect == MUSIC_LAYER_EFFECT_INSTANT) { sound->volume = 0.0f; }
        }
        else {
            sound->enabled       = true;
            sound->volume        = (effect == MUSIC_LAYER_EFFECT_FADE) ? 0.0f : 1.0f;
            sound->target_volume = 1.0f;
            sound->rate          = rate;
        }

        if (sound->enabled || (effect == MUSIC_LAYER_EFFECT_INSTANT)) {
            U32 type = (sound->enabled ? AUDIO_EVENT_TYPE_STARTED : AUDIO_EVENT_TYPE_STOPPED);
            AudioEventPush(player, type, sound->tag, true, index, sound->handle);
        }
    }
}

void MusicLayerDisableByIndex(AudioPlayer *player, U32 index, MusicLayerEffect effect, F32 rate) {
    if (index < player->music.layer_count) {
        PlayingSound *layer = &player->music.layers[index];

        if (layer->enabled) {
            if (rate <= 0) { rate = 1; }

            layer->enabled       = false;
            layer->target_volume = 0.0f;
            layer->rate          = -rate;

            if (effect == MUSIC_LAYER_EFFECT_INSTANT) {
                layer->volume = 0.0f;

                // instant will push a sound effect because the audio turns of right away,
                // if the fade effect is used instead, it will push an audio event when the
                // music volume actually reaches zero
                //
                AudioEventPush(player, AUDIO_EVENT_TYPE_STOPPED, layer->tag, true, index, layer->handle);
            }
        }
    }
}
void MusicLayerEnableByTag(AudioPlayer *player, U32 tag, MusicLayerEffect effect, F32 rate) {
     for (U32 it = 0; it < player->music.layer_count; ++it) {
        PlayingSound *layer = &player->music.layers[it];
        if (layer->tag == tag) {
            if (!layer->enabled) {
                MusicLayerEnableByIndex(player, it, effect, rate);
            }

            break;
        }
    }
}

void MusicLayerToggleByTag(AudioPlayer *player, U32 tag, MusicLayerEffect effect, F32 rate) {
    for (U32 it = 0; it < player->music.layer_count; ++it) {
        PlayingSound *layer = &player->music.layers[it];
        if (layer->tag == tag) {
            MusicLayerToggleByIndex(player, it, effect, rate);
            break;
        }
    }
}

void MusicLayerDisableByTag(AudioPlayer *player, U32 tag, MusicLayerEffect effect, F32 rate) {
    for (U32 it = 0; it < player->music.layer_count; ++it) {
        PlayingSound *layer = &player->music.layers[it];
        if (layer->tag == tag) {
            if (layer->enabled) {
                MusicLayerDisableByIndex(player, it, effect, rate);
            }

            break;
        }
    }
}

void MusicLayerClearAll(AudioPlayer *player) {
    player->music.layer_count = 0;
}

FileScope void SoundEffectRemove(AudioPlayer *player, PlayingSound *sound, U32 index) {
    AudioEventPush(player, AUDIO_EVENT_TYPE_STOPPED, sound->tag, false, 0, sound->handle);

    sound->enabled = false;

    sound->handle.value = 0;
    sound->sample = 0;
    sound->tag    = 0;
    sound->volume = 0.0f;

    sound->next = player->sfx.next_free;
    player->sfx.next_free = index;
}

FileScope void AudioPlayerUpdate(AudioPlayer *player, AssetManager *assets, S16 *samples, U32 frame_count, F32 dt) {
    player->event_count = 0; // reset events :multiple_updates

    M_Arena *temp = M_TempGet();

    // mix in 32-bit float space to prevent clipping and for easier volume control
    //
    // @todo: this is probably really easy to put in simd because it is just some for loops
    // and additions as long as we align the number of samples to 8 we should be good
    //
    F32 *left  = M_ArenaPush(temp, F32, frame_count);
    F32 *right = M_ArenaPush(temp, F32, frame_count);

    if (player->music.playing) {
        // mix music
        //
        F32 music_volume = player->volume * player->music.volume;
        for (U32 it = 0; it < player->music.layer_count; ++it) {
            PlayingSound *layer = &player->music.layers[it];

            layer->target_volume = layer->enabled ? 1.0f : 0.0f;

            layer->volume += (layer->rate * dt);
            layer->volume  = Clamp01(layer->volume);

            if (layer->enabled && layer->volume <= 0) {
                layer->enabled = false;

                AudioEventPush(player, AUDIO_EVENT_TYPE_STOPPED, layer->tag, true, it, layer->handle);
            }

            Xia_SoundInfo *info = SoundInfoGet(assets, layer->handle);
            S16           *data = SoundDataGet(assets, layer->handle);

            if (info && data) {
                if (layer->sample + (2 * frame_count) >= info->num_samples) {
                    // we will loop during mixing so push a sound event
                    //
                    AudioEventPush(player, AUDIO_EVENT_TYPE_LOOP_RESET, layer->tag, true, it, layer->handle);
                }

                for (U32 si = 0; si < frame_count; ++si) {
                    if (layer->enabled || (layer->volume > 0)) {
                        // we keep layers in sync by iterating frames regardless of if they are
                        // enabled or not, we just don't physically output the samples if they are
                        // disabled
                        //
                        left[si]  += (music_volume * layer->volume * data[layer->sample + 0]);
                        right[si] += (music_volume * layer->volume * data[layer->sample + 1]);
                    }

                    layer->sample += 2;
                    layer->sample %= info->num_samples;
                }
            }
        }
    }

    // mix sound effects
    //
    // @todo: this could easily be an indexed linked list for playing sounds rather than looping
    // over all sounds and seeing if they're enabled, doesn't really matter
    //
    F32 sfx_volume = player->volume * player->sfx.volume;
    for (U32 it = 0; it < player->sfx.limit; ++it) {
        PlayingSound *sound = &player->sfx.sounds[it];
        if (sound->enabled) {
            Xia_SoundInfo *info = SoundInfoGet(assets, sound->handle);
            S16           *data = SoundDataGet(assets, sound->handle);

            if (info && data) {
                data += sound->sample;

                U32 count = Min(frame_count, (info->num_samples - sound->sample) >> 1);
                for (U32 si = 0; si < count; ++si) {
                    left[si]  += (sfx_volume * sound->volume * (*data++));
                    right[si] += (sfx_volume * sound->volume * (*data++));
                }

                sound->sample += (2 * count);
                Assert(sound->sample <= info->num_samples); // sanity checking

                if (sound->sample == info->num_samples) {
                    SoundEffectRemove(player, sound, it);
                }
            }
        }
    }

    // downsample to s16 for our sample format
    //
    S16 *out = samples;
    for (U32 it = 0; it < frame_count; ++it) {
        *out++ = cast(S16) (left[it]  + 0.5f);
        *out++ = cast(S16) (right[it] + 0.5f);
    }
}

