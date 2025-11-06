#include "allegro.h"
#include "include/internal/aintern.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

ALLEGRO_DEBUG_CHANNEL("a425")

static int digi_reserve = -1;             /* how many voices to reserve */

void adjust_sample(AL_CONST SAMPLE * spl, int vol, int pan, int freq, int loop) {
   if (spl->stream) {
      al_set_audio_stream_playmode(spl->stream, (loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE));
      al_set_audio_stream_speed(spl->stream, freq / 1000.0f);
      al_set_audio_stream_pan(spl->stream, (pan - 128) / 128.0f);
      al_set_audio_stream_gain(spl->stream, vol / 255.0f);
   } else
      ALLEGRO_ERROR("Not implemented for non-streaming samples\n");
}

/* allegro4 uses 0 as ok values */
static int is_ok(int code){
    if (code){
        return 0;
    }
    return -1;
}

void reserve_voices(int digi_voices, int midi_voices){
    digi_reserve = digi_voices;
}

int install_sound(int digi, int midi, AL_CONST char *cfg_path){
    if (al_install_audio() && al_init_acodec_addon()) {
        if (digi_reserve >= 0)
            al_reserve_samples(digi_reserve);
        else
            al_reserve_samples(MIXER_DEF_SFX);
        return 0;
    } else {
        return -1;
    }
}

void release_voice(int voice){
    /* FIXME */
}

static void lazily_create_sample(SAMPLE * sample){
    if (sample->real == NULL){
        ALLEGRO_CHANNEL_CONF channels = ALLEGRO_CHANNEL_CONF_1;
        ALLEGRO_AUDIO_DEPTH depth = ALLEGRO_AUDIO_DEPTH_UINT8;
        switch (sample->stereo){
            case 0: channels = ALLEGRO_CHANNEL_CONF_1; break;
            case 1: channels = ALLEGRO_CHANNEL_CONF_2; break;
        }
        switch (sample->bits){
            case 8: depth = ALLEGRO_AUDIO_DEPTH_UINT8; break;
            case 16: depth = ALLEGRO_AUDIO_DEPTH_UINT16; break;
        }
        sample->real = al_create_sample(sample->data, sample->len, sample->freq, depth, channels, false);
    }
}

AUDIOSTREAM* play_audio_stream(int len, int bits, int stereo, int freq, int vol, int pan)
{
   AUDIOSTREAM* stream = al_malloc(sizeof(AUDIOSTREAM));
   if (!stream) return NULL;

   stream->locked = NULL;
   /* This is ugly but expected. For example, see adjust_sample() and alogg code.
    * this is a streaming sample, so sample buffer is irrelevant with a5.
    */
   stream->samp = al_malloc(sizeof(SAMPLE));
   stream->samp->data = NULL; // alogg_get_output_wave_oggstream might return it. Make sure it is NULL.
   stream->samp->stream = al_create_audio_stream(2, len, freq,
       ((bits >> 3) - 1) | ALLEGRO_AUDIO_DEPTH_UNSIGNED,
       stereo ? ALLEGRO_CHANNEL_CONF_2 : ALLEGRO_CHANNEL_CONF_1);
   if (!al_attach_audio_stream_to_mixer(stream->samp->stream, al_get_default_mixer())) {
      ALLEGRO_ERROR("Failed to attach output audio stream to mixer!\n");
      al_destroy_audio_stream(stream->samp->stream);
      al_free(stream);
      return NULL;
   }

   al_set_audio_stream_playing(stream->samp->stream, true);
   al_set_audio_stream_playmode(stream->samp->stream, ALLEGRO_PLAYMODE_LOOP);
   al_set_audio_stream_pan(stream->samp->stream, (pan - 128) / 128.0f);
   al_set_audio_stream_gain(stream->samp->stream, vol / 255.0f);
   // al_register_event_source(system_event_queue, al_get_audio_stream_event_source(stream->samp->stream));

   return stream;
}

void stop_audio_stream(AUDIOSTREAM* stream)
{
   if (stream) {
      if (stream->samp) {
         // al_unregister_event_source(system_event_queue, al_get_audio_stream_event_source(stream->samp->stream));
         al_destroy_audio_stream(stream->samp->stream);
         al_free(stream->samp);
      }
      al_free(stream);
   }
}

int play_sample(AL_CONST SAMPLE * sample, int volume, int pan, int frequency, int loop){
    int a5_loop = ALLEGRO_PLAYMODE_ONCE;
    lazily_create_sample((SAMPLE*) sample);
    switch (loop){
        case 1: a5_loop = ALLEGRO_PLAYMODE_LOOP; break;
        default: a5_loop = ALLEGRO_PLAYMODE_ONCE; break;
    }
    return is_ok(al_play_sample(sample->real, volume / 255.0, (pan - 128.0) / 128.0, frequency / 1000.0, a5_loop, NULL));
}

SAMPLE *load_sample(AL_CONST char *filename)
{
    SAMPLE *sample = al_calloc(sizeof *sample, 1);
    sample->real = al_load_sample(filename);
    switch (al_get_sample_depth(sample->real)) {
        case ALLEGRO_AUDIO_DEPTH_UINT8: sample->bits = 8; break;
        case ALLEGRO_AUDIO_DEPTH_UINT16: sample->bits = 16; break;
        default: break;
    }
    switch (al_get_sample_channels(sample->real)) {
        case ALLEGRO_CHANNEL_CONF_2: sample->stereo = 1; break;
        default: break;
    }
    return sample;
}

void destroy_sample(SAMPLE *sample){
    if (!sample) return;

    if (sample->real) {
        al_destroy_sample(sample->real);
    }
    al_free(sample);
}

void stop_midi(){
    /* FIXME */
}

int play_midi(MIDI *midi, int loop){
    /* FIXME */
    return -1;
}

void set_volume(int digi_volume, int midi_volume){
    /* FIXME */
}
