XI_INTERNAL void xi_audio_player_init(xiArena *arena, xiAudioPlayer *player) {
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
        player->frame_count = (xi_u32) ((0.05f * player->sample_rate) + 0.5f);
    }

    if (player->music.layer_limit == 0) { player->music.layer_limit = 4;  }
    if (player->sfx.limit         == 0) { player->sfx.limit         = 32; }

    xi_u32 event_limit = player->music.layer_limit + player->sfx.limit;

    player->music.layers = xi_arena_push_array(arena, xiSound, player->music.layer_limit);
    player->sfx.sounds   = xi_arena_push_array(arena, xiSound, player->sfx.limit);
    player->events       = xi_arena_push_array(arena, xiAudioEvent, event_limit);

    for (xi_u32 it = 0; it < player->sfx.limit - 1; ++it) {
        player->sfx.sounds[it].next = it + 1;
    }

    player->music.layer_count = 0;
    player->sfx.count         = 0;
    player->sfx.next_free     = 0;
}

XI_INTERNAL void xi_audio_event_push(xiAudioPlayer *player,
        xi_u32 type, xi_u32 tag, xi_b32 from_music, xi_u32 index, xiSoundHandle handle)
{
    xi_u32 event_limit = (player->music.layer_limit + player->sfx.limit);
    XI_ASSERT(player->event_count < event_limit);

    xiAudioEvent *event = &player->events[player->event_count];
    player->event_count += 1;

    event->type       = type;
    event->tag        = tag;
    event->from_music = from_music;
    event->index      = index;
    event->handle     = handle;
}

void xi_sound_effect_play(xiAudioPlayer *player, xiSoundHandle handle, xi_u32 tag, xi_f32 volume) {
    if (player->sfx.count < player->sfx.limit) {
        xiSound *sound = &player->sfx.sounds[player->sfx.next_free];
        player->sfx.next_free = sound->next;

        sound->enabled = true;

        sound->handle = handle;
        sound->tag    = tag;
        sound->sample = 0;
        sound->volume = volume;
        sound->target_volume = 0; // @todo: maybe we should use this for effects as well

        xi_audio_event_push(player, XI_AUDIO_EVENT_TYPE_STARTED, tag, false, 0, handle);
    }
}

xi_u32 xi_music_layer_add(xiAudioPlayer *player, xiSoundHandle handle, xi_u32 tag) {
    xi_u32 result = XI_U32_MAX;

    if (player->music.layer_count < player->music.layer_limit) {
        result = player->music.layer_count;
        xiSound *layer = &player->music.layers[result];

        layer->enabled = false;
        layer->handle  = handle;
        layer->sample  = 0;
        layer->tag     = tag;
        layer->volume  = 0.0f;

        player->music.layer_count += 1;
    }

    return result;
}

void xi_music_layer_enable_by_index(xiAudioPlayer *player, xi_u32 index, xi_u32 effect, xi_f32 rate) {
    if (index < player->music.layer_count) {
        xiSound *sound = &player->music.layers[index];

        if (rate <= 0) { rate = 1; }

        if (!sound->enabled) {
            sound->enabled       = true;
            sound->volume        = (effect == XI_MUSIC_LAYER_EFFECT_FADE) ? 0.0f : 1.0f;
            sound->target_volume = 1.0f;
            sound->rate          = rate;

            xi_audio_event_push(player, XI_AUDIO_EVENT_TYPE_STARTED, sound->tag, true, index, sound->handle);
        }
    }
}

void xi_music_layer_disable_by_index(xiAudioPlayer *player, xi_u32 index, xi_u32 effect, xi_f32 rate) {
    if (index < player->music.layer_count) {
        xiSound *layer = &player->music.layers[index];

        if (rate <= 0) { rate = 1; }

        layer->enabled       = false;
        layer->target_volume = 0.0f;
        layer->rate          = -rate;

        if (effect == XI_MUSIC_LAYER_EFFECT_INSTANT) {
            layer->volume  = 0.0f;
            layer->enabled = false;

            // instant will push a sound effect because the audio turns of right away,
            // if the fade effect is used instead, it will push an audio event when the
            // music volume actually reaches zero
            //
            xi_audio_event_push(player, XI_AUDIO_EVENT_TYPE_STOPPED,
                    layer->target_volume, true, index, layer->handle);
        }
    }
}

void xi_music_layer_toggle_by_index(xiAudioPlayer *player, xi_u32 index, xi_u32 effect, xi_f32 rate) {
    if (index < player->music.layer_count) {
        xiSound *sound = &player->music.layers[index];

        if (rate <= 0) { rate = 1; }

        if (sound->enabled) {
            sound->enabled       = false;
            sound->target_volume = 0.0f;
            sound->rate          = -rate;

            if (effect == XI_MUSIC_LAYER_EFFECT_INSTANT) { sound->volume = 0.0f; }
        }
        else {
            sound->enabled       = true;
            sound->volume        = (effect == XI_MUSIC_LAYER_EFFECT_FADE) ? 0.0f : 1.0f;
            sound->target_volume = 1.0f;
            sound->rate          = rate;
        }

        if (sound->enabled || (effect == XI_MUSIC_LAYER_EFFECT_INSTANT)) {
            xi_u32 type = (sound->enabled ? XI_AUDIO_EVENT_TYPE_STARTED : XI_AUDIO_EVENT_TYPE_STOPPED);
            xi_audio_event_push(player, type, sound->tag, true, index, sound->handle);
        }
    }
}

void xi_music_layer_enable_by_tag(xiAudioPlayer *player, xi_u32 tag, xi_u32 effect, xi_f32 rate) {
     for (xi_u32 it = 0; it < player->music.layer_count; ++it) {
        xiSound *layer = &player->music.layers[it];
        if (layer->tag == tag) {
            if (!layer->enabled) {
                xi_music_layer_enable_by_index(player, it, effect, rate);
            }

            break;
        }
    }
}

void xi_music_layer_disable_by_tag(xiAudioPlayer *player, xi_u32 tag, xi_u32 effect, xi_f32 rate) {
    for (xi_u32 it = 0; it < player->music.layer_count; ++it) {
        xiSound *layer = &player->music.layers[it];
        if (layer->tag == tag) {
            if (layer->enabled) {
                xi_music_layer_disable_by_index(player, it, effect, rate);
            }

            break;
        }
    }
}

void xi_music_layer_toggle_by_tag(xiAudioPlayer *player, xi_u32 tag, xi_u32 effect, xi_f32 rate) {
    for (xi_u32 it = 0; it < player->music.layer_count; ++it) {
        xiSound *layer = &player->music.layers[it];
        if (layer->tag == tag) {
            xi_music_layer_toggle_by_index(player, it, effect, rate);
            break;
        }
    }
}

void xi_music_layers_clear(xiAudioPlayer *player) {
    player->music.layer_count = 0;
}

XI_INTERNAL void xi_sound_effect_remove(xiAudioPlayer *player, xiSound *sound, xi_u32 index) {
    xi_audio_event_push(player, XI_AUDIO_EVENT_TYPE_STOPPED, sound->tag, false, 0, sound->handle);

    sound->enabled = false;

    sound->handle.value = 0;
    sound->sample = 0;
    sound->tag    = 0;
    sound->volume = 0.0f;

    sound->next = player->sfx.next_free;
    player->sfx.next_free = index;

}

XI_INTERNAL void xi_audio_player_update(xiAudioPlayer *player, xiAssetManager *assets,
        xi_s16 *samples, xi_u32 frame_count, xi_f32 dt)
{
    player->event_count = 0; // reset events :multiple_updates

    xiArena *temp = xi_temp_get();

    // mix in 32-bit float space to prevent clipping and for easier volume control
    //
    // @todo: this is probably really easy to put in simd because it is just some for loops
    // and additions as long as we align the number of samples to 8 we should be good
    //
    xi_f32 *left  = xi_arena_push_array(temp, xi_f32, frame_count);
    xi_f32 *right = xi_arena_push_array(temp, xi_f32, frame_count);

    if (player->music.playing) {
        // mix music
        //
        xi_f32 music_volume = player->volume * player->music.volume;
        for (xi_u32 it = 0; it < player->music.layer_count; ++it) {
            xiSound *layer = &player->music.layers[it];
            if (layer->enabled || layer->volume > 0) {
                layer->target_volume = layer->enabled ? 1.0f : 0.0f;

                layer->volume += (layer->rate * dt);
                layer->volume  = XI_CLAMP(layer->volume, 0.0f, 1.0f);

                if (layer->volume <= 0) {
                    layer->enabled = false;

                    xi_audio_event_push(player, XI_AUDIO_EVENT_TYPE_STOPPED,
                            layer->target_volume, true, it, layer->handle);
                }

                xiaSoundInfo *info = xi_sound_info_get(assets, layer->handle);
                xi_s16 *data = xi_sound_data_get(assets, layer->handle);

                if (info) {
                    if (layer->sample + (2 * frame_count) >= info->sample_count) {
                        // we will loop during mixing so push a sound event
                        //
                        xi_audio_event_push(player, XI_AUDIO_EVENT_TYPE_LOOP_RESET,
                                layer->tag, true, it, layer->handle);
                    }

                    for (xi_u32 si = 0; si < frame_count; ++si) {
                        left[si]  += (music_volume * layer->volume * data[layer->sample + 0]);
                        right[si] += (music_volume * layer->volume * data[layer->sample + 1]);

                        layer->sample += 2;
                        layer->sample %= info->sample_count;
                    }
                }
            }
        }
    }

    // mix sound effects
    //
    // @todo: this could easily be an indexed linked list for playing sounds rather than looping
    // over all sounds and seeing if they're enabled, doesn't really matter
    //
    xi_f32 sfx_volume = player->volume * player->sfx.volume;
    for (xi_u32 it = 0; it < player->sfx.limit; ++it) {
        xiSound *sound = &player->sfx.sounds[it];
        if (sound->enabled) {
            xiaSoundInfo *info = xi_sound_info_get(assets, sound->handle);
            xi_s16 *data = xi_sound_data_get(assets, sound->handle);
            if (info && data) {
                data += sound->sample;

                xi_u32 count  = XI_MIN(frame_count, (info->sample_count - sound->sample) >> 1);
                for (xi_u32 si = 0; si < count; ++si) {
                    left[si]  += (sfx_volume * sound->volume * (*data++));
                    right[si] += (sfx_volume * sound->volume * (*data++));
                }

                sound->sample += (2 * count);
                XI_ASSERT(sound->sample <= info->sample_count); // sanity checking

                if (sound->sample == info->sample_count) {
                    xi_sound_effect_remove(player, sound, it);
                }
            }
        }
    }

    // downsample to s16 for our sample format
    //
    xi_s16 *out = samples;
    for (xi_u32 it = 0; it < frame_count; ++it) {
        *out++ = (xi_s16) (left[it]  + 0.5f);
        *out++ = (xi_s16) (right[it] + 0.5f);
    }
}

