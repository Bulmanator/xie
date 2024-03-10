#if !defined(XI_AUDIO_H_)
#define XI_AUDIO_H_

typedef U32 MusicLayerEffect;
enum MusicLayerEffect {
    MUSIC_LAYER_EFFECT_INSTANT = 0,
    MUSIC_LAYER_EFFECT_FADE
};

typedef U32 AudioEventType;
enum AudioEventType {
    AUDIO_EVENT_TYPE_STARTED = 0, // audio has been started, posted when sfx starts or music layer is
                                  // enabled
                                  //
    AUDIO_EVENT_TYPE_STOPPED,     // audio has ended and will be stopped, posted when sfx ends or music
                                  // layer is disabled
                                  //
    AUDIO_EVENT_TYPE_LOOP_RESET   // for music, audio has reached the end and looped back to the beginning
};

typedef struct PlayingSound PlayingSound;
struct PlayingSound {
    SoundHandle handle;

    B32 enabled;

    U32 sample;

    U32 tag;

    F32 volume;
    F32 target_volume;
    F32 rate;

    U32 next; // for freelist
};

typedef struct AudioEvent AudioEvent;
struct AudioEvent {
    U32 type;

    U32 tag;
    B32 from_music; // true if music caused the event, otherwise false for sfx
    U32 index;      // valid if 'from_music' is true and indicates the layer that caused the event

    SoundHandle handle;
};

// :note during initialisation the system will not interact with volume values, meaning if they are
// zero they will stay zero. this could cause no audio to play when it was desired
//
typedef struct AudioPlayer AudioPlayer;
struct AudioPlayer {
    U32 sample_rate;
    U32 frame_count;

    F32 volume; // global to all sounds

    struct {
        B32 playing; // setting to false will pause all music

        F32 volume; // all music layers

        U32 layer_count;
        U32 layer_limit;
        PlayingSound *layers; // @todo: this should just be a linked list with freelist :sound_freelist
    } music;

    struct {
        F32 volume;

        U32 next_free;

        U32 limit;
        U32 count;
        PlayingSound *sounds; // ... :sound_freelist
    } sfx;

    // events are only considered valid for a single frame and should be processed if needed
    //
    U32 event_count;
    AudioEvent *events;
};

// :note these function calls _can_ produce sound events, it is best you process sound events at the
// end of simulate
//

// sound effects are a 'one-shot' playback of a sound, does not loop
//
// tags can be used to identify a sound in a sound event, it is up to the user what the tag is. should be
// unique if used to identify specific sounds. is not used internally by the system.
//
Func void SoundEffectPlay(AudioPlayer *player, SoundHandle sound, U32 tag, F32 volume);

// music is considered to be continuous and thus will loop, layers can be added for playback of multiple
// tracks
//
// add a layer to the music, layers are disabled by default and can be enabled/disabled using the functions
// below. layers are added sequentially, to setup new music first call clear and add new handles
//
// tag works the same as for sound effects
//
Func U32 MusicLayerAdd(AudioPlayer *player, SoundHandle sound, U32 tag);

// enable or disable a layer of music via a direct index which was returned from xi_music_layer_add
//
Func void  MusicLayerEnableByIndex(AudioPlayer *player, U32 index, MusicLayerEffect effect, F32 rate);
Func void  MusicLayerToggleByIndex(AudioPlayer *player, U32 index, MusicLayerEffect effect, F32 rate);
Func void MusicLayerDisableByIndex(AudioPlayer *player, U32 index, MusicLayerEffect effect, F32 rate);

Func void  MusicLayerEnableByTag(AudioPlayer *player, U32 tag, MusicLayerEffect effect, F32 rate);
Func void  MusicLayerToggleByTag(AudioPlayer *player, U32 tag, MusicLayerEffect effect, F32 rate);
Func void MusicLayerDisableByTag(AudioPlayer *player, U32 tag, MusicLayerEffect effect, F32 rate);

Func void MusicLayerClearAll(AudioPlayer *player);

#endif  // XI_AUDIO_H_
