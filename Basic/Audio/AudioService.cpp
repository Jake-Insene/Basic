#include "Basic/Audio/AudioService.hpp"

#include <math/funcs.h>

#include "Basic/Resource/Sound.hpp"


namespace Basic
{

AudioService::Mixer::Mixer(Mem::Allocator& allocator, Collections::StringView name)
: name(allocator, 0, name), volume(1.F), plays(allocator, 4, {})
{}

AudioService::AudioService(Mem::Allocator& allocator)
: allocator(allocator), mixers(allocator, 4, {}), output_buffer()
{
    Audio::output_start();
    mixer_create("Master");
    
    output_buffer = allocator.array<Audio::Frame>(Audio::output_get_samples_per_sec());
}

AudioService::~AudioService()
{
    allocator.free(Mem::to_bytes(output_buffer));
    Audio::output_stop();
}

void AudioService::update()
{
    u32 frame_count = Audio::output_get_frame_count();
    if(frame_count == 0)
    {
        return;
    }

    Slice samples = output_buffer.slice(frame_count);

    // getting enqueue plays
    Slice mixers_slice = mixers.slice();

    for(usize frame_i = 0; frame_i < frame_count; frame_i++)
    {
        Audio::Frame frame = Audio::Frame();
        for(AudioService::Mixer& mixer : mixers_slice)
        {
            _mixer_mix(&mixer, &frame);
        }
        
        Audio::Frame final_frame = Audio::Frame(
            Math::clamp<f32>(frame.left, -32768.0F, 32767.0F),
            Math::clamp<f32>(frame.right, -32768.0F, 32767.0F)
        );
        final_frame.mul(1/32768.0F);
        samples[frame_i] = final_frame;
    }

    Audio::output_send_frames(samples);
}

u32 AudioService::mixer_create(Collections::StringView mixer_name)
{
    (void)mixers.emplace(allocator, mixer_name);
    return mixers.count - 1;
}

u32 AudioService::mixer_get_by_name(Collections::StringView mixer_name)
{
    for(u32 i = 0; i < mixers.count; i++)
    {
        Mixer& mix = mixers.get(i);
        if(mix.name.equals(mixer_name))
        {
            return i;
        }
    }

    return Core::MaxValue<u32>;
}

u32 AudioService::mixer_count()
{
    return mixers.count;
}

f32 AudioService::mixer_get_volume(u32 mixer)
{
    return mixers.get(mixer).volume;
}

void AudioService::mixer_set_volume(u32 mixer, f32 new_volume)
{
    mixers.get(mixer).volume = new_volume;
}

void AudioService::mixer_play(u32 mixer, Sound* sound, const PlayInfo& play_info)
{
    Mixer& mix = mixers.get(mixer);

    (void)mix.plays.add(
        EnqueuePlay
        {
            .sound = sound,
            .play_info = play_info,
            .frame_index = 0,
        }
    );
}

void AudioService::_mixer_mix(Mixer* mixer, Audio::Frame* frame)
{
    if(mixer->plays.count == 0)
    {
        return;
    }

    f32 normalize = 1 / f32(mixer->plays.count);
    for(usize play_i = 0; play_i < mixer->plays.count; play_i++)
    {
        AudioService::EnqueuePlay& enqueue_play = mixer->plays.get(play_i);

        if(enqueue_play.frame_index >= enqueue_play.sound->get_frame_count())
        {
            if(enqueue_play.play_info.loop)
            {
                enqueue_play.frame_index = 0;
            }
            else
            {
                // remove
                mixer->plays.remove_at(play_i);
                play_i--;
                continue;
            }
        }

        Audio::Frame sample = enqueue_play.sound->get_frame(enqueue_play.frame_index);

        sample.mul(mixer->volume * enqueue_play.play_info.volume * normalize);
        frame->add(sample);
        enqueue_play.frame_index += 1;
    }
}

}
