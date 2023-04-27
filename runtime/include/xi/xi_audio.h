#if !defined(XI_AUDIO_H_)
#define XI_AUDIO_H_

enum xiMusicLayerEffect {
    XI_MUSIC_LAYER_EFFECT_INSTANT = 0,
    XI_MUSIC_LAYER_EFFECT_FADE
};

enum xiAudioEventType {
    XI_AUDIO_EVENT_TYPE_STARTED = 0, // audio has been started, posted when sfx starts or music layer is
                                     // enabled
                                     //
    XI_AUDIO_EVENT_TYPE_STOPPED,     // audio has ended and will be stopped, posted when sfx ends or music
                                     // layer is disabled
                                     //
    XI_AUDIO_EVENT_TYPE_LOOP_RESET   // for music, audio has reached the end and looped back to the beginning
};

typedef struct xiSound {
    xiSoundHandle handle;

    xi_b32 enabled;

    xi_u32 sample;

    xi_u32 tag;

    xi_f32 volume;
    xi_f32 target_volume;
    xi_f32 rate;

    xi_u32 next; // for freelist
} xiSound;

typedef struct xiAudioEvent {
    xi_u32 type;

    xi_u32 tag;
    xi_b32 from_music; // true if music caused the event, otherwise false for sfx
    xi_u32 index;      // valid if 'from_music' is true and indicates the layer that caused the event

    xiSoundHandle handle;
} xiAudioEvent;

// :note during initialisation the system will not interact with volume values, meaning if they are
// zero they will stay zero. this could cause no audio to play when it was desired
//
typedef struct xiAudioPlayer {
    xi_u32 sample_rate;
    xi_u32 frame_count;

    xi_f32 volume; // global to all sounds

    struct {
        xi_b32 playing; // setting to false will pause all music

        xi_f32 volume; // all music layers

        xi_u32 layer_count;
        xi_u32 layer_limit;
        xiSound *layers;
    } music;

    struct {
        xi_f32 volume;

        xi_u32 next_free;

        xi_u32 limit;
        xi_u32 count;
        xiSound *sounds;
    } sfx;

    // events are only considered valid for a single frame and should be processed if needed
    //
    xi_u32 event_count;
    xiAudioEvent *events;
} xiAudioPlayer;

// :note these function calls _can_ produce sound events, it is best you process sound events at the
// end of simulate
//

// sound effects are a 'one-shot' playback of a sound, does not loop
//
// tags can be used to identify a sound in a sound event, it is up to the user what the tag is. should be
// unique if used to identify specific sounds. is not used internally by the system.
//
extern XI_API void xi_sound_effect_play(xiAudioPlayer *player, xiSoundHandle sound, xi_u32 tag, xi_f32 volume);

// music is considered to be continuous and thus will loop, layers can be added for playback of multiple
// tracks
//
// add a layer to the music, layers are disabled by default and can be enabled/disabled using the functions
// below. layers are added sequentially, to setup new music first call clear and add new handles
//
// tag works the same as for sound effects
//
extern XI_API xi_u32 xi_music_layer_add(xiAudioPlayer *player, xiSoundHandle sound, xi_u32 tag);

// enable or disable a layer of music via a direct index which was returned from xi_music_layer_add
//
extern XI_API void xi_music_layer_enable_by_index(xiAudioPlayer *player,
        xi_u32 index, xi_u32 effect, xi_f32 rate);

extern XI_API void xi_music_layer_disable_by_index(xiAudioPlayer *player,
        xi_u32 index, xi_u32 effect, xi_f32 rate);

extern XI_API void xi_music_layer_toggle_by_index(xiAudioPlayer *player,
        xi_u32 index, xi_u32 effect, xi_f32 rate);

// enable/disable music layers with a specific tag, if multiple layers contain the same tag only the
// first matching layer will be modified
//
extern XI_API void xi_music_layer_enable_by_tag(xiAudioPlayer *player,
        xi_u32 tag, xi_u32 effect, xi_f32 rate);

extern XI_API void xi_music_layer_disable_by_tag(xiAudioPlayer *player,
        xi_u32 tag, xi_u32 effect, xi_f32 rate);

extern XI_API void xi_music_layer_toggle_by_tag(xiAudioPlayer *player,
        xi_u32 tag, xi_u32 effect, xi_f32 rate);

// clear all music layers for setting up new music
//
extern XI_API void xi_music_layers_clear(xiAudioPlayer *player);

#endif  // XI_AUDIO_H_
