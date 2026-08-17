#pragma once
#include <audio/audio.h>
#include <Collections/Array.hpp>
#include <Collections/String.hpp>
#include <Mem/Allocator.hpp>


struct Sound;

struct AudioService
{
    DisableCopy(AudioService);
    DisableMove(AudioService);

    struct PlayInfo
    {
        f32 volume;
        bool loop;
    };

    struct EnqueuePlay
    {
        Sound* sound;
        PlayInfo play_info;

        usize frame_index;
    };

    struct Mixer
    {
        Collections::String name;
        f32 volume;

        Collections::Array<EnqueuePlay> plays;

        Mixer(Mem::Allocator& allocator, Collections::StringView name);
    };

    Mem::Allocator& allocator;
    
    Collections::Array<Mixer> mixers;
    Slice<Audio::Frame> output_buffer;

    AudioService(Mem::Allocator& allocator);
    ~AudioService();

    void update();

    u32 mixer_create(Collections::StringView mixer_name);
    u32 mixer_get_by_name(Collections::StringView mixer_name);
    u32 mixer_count();
    
    f32 mixer_get_volume(u32 mixer);
    void mixer_set_volume(u32 mixer, f32 new_volume);

    void mixer_play(u32 mixer, Sound* sound, const PlayInfo& play_info);

    void _mixer_mix(Mixer* mixer, Audio::FrameF* frame);
};
