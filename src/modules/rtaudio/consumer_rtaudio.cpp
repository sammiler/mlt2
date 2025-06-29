/*
 * consumer_rtaudio.c -- output through RtAudio audio wrapper
 * Copyright (C) 2011-2023 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
extern "C"{
    #include <gettimeofday.h>
}
#else
    #include <sys/time.h>
#endif
#ifdef USE_INTERNAL_RTAUDIO
#include "RtAudio.h"
#else
#include <RtAudio.h>
#endif

#if defined(RTAUDIO_VERSION_MAJOR) && RTAUDIO_VERSION_MAJOR >= 6
#define RTAUDIO_VERSION_6
#endif

static void consumer_refresh_cb(mlt_consumer sdl, mlt_consumer consumer, mlt_event_data);
static int rtaudio_callback(void *outputBuffer,
                            void *inputBuffer,
                            unsigned int nFrames,
                            double streamTime,
                            RtAudioStreamStatus status,
                            void *userData);
static void *consumer_thread_proxy(void *arg);
static void *video_thread_proxy(void *arg);

static const char *rtaudio_api_str(RtAudio::Api api)
{
    switch (api) {
    case RtAudio::UNSPECIFIED:
        return "UNSPECIFIED";
    case RtAudio::LINUX_ALSA:
        return "LINUX_ALSA";
    case RtAudio::LINUX_PULSE:
        return "LINUX_PULSE";
    case RtAudio::LINUX_OSS:
        return "LINUX_OSS";
    case RtAudio::UNIX_JACK:
        return "UNIX_JACK";
    case RtAudio::MACOSX_CORE:
        return "MACOSX_CORE";
    case RtAudio::WINDOWS_WASAPI:
        return "WINDOWS_WASAPI";
    case RtAudio::WINDOWS_ASIO:
        return "WINDOWS_ASIO";
    case RtAudio::WINDOWS_DS:
        return "WINDOWS_DS";
    case RtAudio::RTAUDIO_DUMMY:
        return "RTAUDIO_DUMMY";
    default:
        break;
    }
    return "UNKNOWN!?!";
}

class RtAudioConsumer
{
public:
    struct mlt_consumer_s consumer;
    RtAudio *rt;
    int device_id;
    mlt_deque queue;
    pthread_t thread;
    int joined;
    int running;
    int out_channels;
    uint8_t audio_buffer[4096 * 10];
    int audio_avail;
    pthread_mutex_t audio_mutex;
    pthread_cond_t audio_cond;
    pthread_mutex_t video_mutex;
    pthread_cond_t video_cond;
    int playing;
    pthread_cond_t refresh_cond;
    pthread_mutex_t refresh_mutex;
    int refresh_count;
    bool is_purge;

    mlt_consumer getConsumer() { return &consumer; }

    RtAudioConsumer()
        : rt(nullptr)
        , device_id(-1)
        , queue(nullptr)
        , joined(0)
        , running(0)
        , audio_avail(0)
        , playing(0)
        , refresh_count(0)
        , is_purge(false)
    {
        memset(&consumer, 0, sizeof(consumer));
    }

    ~RtAudioConsumer()
    {
        // Close the queue
        mlt_deque_close(queue);

        // Destroy mutexes
        pthread_mutex_destroy(&audio_mutex);
        pthread_cond_destroy(&audio_cond);
        pthread_mutex_destroy(&video_mutex);
        pthread_cond_destroy(&video_cond);
        pthread_mutex_destroy(&refresh_mutex);
        pthread_cond_destroy(&refresh_cond);

        if (rt && rt->isStreamOpen())
            rt->closeStream();
        delete rt;
        rt = nullptr;
    }

    bool create_rtaudio(RtAudio::Api api, int channels, int frequency)
    {
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(getConsumer());
        const char *resource = mlt_properties_get(properties, "resource");
        unsigned int bufferFrames = mlt_properties_get_int(properties, "audio_buffer");

        mlt_log_info(getConsumer(),
                     "Attempt to open RtAudio: %s\t%d\t%d\n",
                     rtaudio_api_str(api),
                     channels,
                     frequency);
        rt = new RtAudio(api);

        if (!rt) {
            return false;
        }

        if (rt->getDeviceCount() < 1) {
            mlt_log_warning(getConsumer(), "no audio devices found\n");
            delete rt;
            rt = nullptr;
            return false;
        }

#if defined(RTAUDIO_VERSION_6) || !defined(__LINUX_ALSA__)
        device_id = rt->getDefaultOutputDevice();
#endif
        if (resource && strcmp(resource, "") && strcmp(resource, "default")) {
            // Get device ID by name
            RtAudio::DeviceInfo info;
            unsigned int i;

#ifdef RTAUDIO_VERSION_6
            auto ids = rt->getDeviceIds();
            for (i = 0; i < ids.size(); i++) {
                info = rt->getDeviceInfo(ids[i]);
                mlt_log_verbose(nullptr, "RtAudio device %u = %s\n", ids[i], info.name.c_str());
                if (info.name.find(resource) != std::string::npos) {
                    device_id = info.ID;
                    break;
                }
            }
            if (i == ids.size())
#else
            unsigned int n = rt->getDeviceCount();
            for (i = 0; i < n; i++) {
                info = rt->getDeviceInfo(i);
                mlt_log_verbose(nullptr, "RtAudio device %d = %s\n", i, info.name.c_str());
                if (info.probed && info.name == resource) {
                    device_id = i;
                    break;
                }
            }
            // Name selection failed, try arg as numeric
            if (i == n)
#endif
                device_id = (int) strtol(resource, nullptr, 0);
        }

        RtAudio::StreamParameters parameters;
        parameters.deviceId = device_id;
        parameters.nChannels = channels;
        parameters.firstChannel = 0;
        RtAudio::StreamOptions options;

        if (device_id == -1) {
            options.flags = RTAUDIO_ALSA_USE_DEFAULT;
            parameters.deviceId = 0;
        }
        if (resource) {
#ifdef RTAUDIO_VERSION_6
            auto ids = rt->getDeviceIds();
            for (unsigned i = 0; i < ids.size(); i++) {
                auto info = rt->getDeviceInfo(ids[i]);
                if (info.name == resource) {
                    device_id = parameters.deviceId = info.ID;
                    break;
                }
            }
#else
            unsigned n = rt->getDeviceCount();
            for (unsigned i = 0; i < n; i++) {
                RtAudio::DeviceInfo info = rt->getDeviceInfo(i);
                if (info.name == resource) {
                    device_id = parameters.deviceId = i;
                    break;
                }
            }
#endif
        }

#ifdef RTAUDIO_VERSION_6
        if (rt->isStreamOpen()) {
            rt->closeStream();
        }
        if (rt->openStream(&parameters,
                           nullptr,
                           RTAUDIO_SINT16,
                           frequency,
                           &bufferFrames,
                           &rtaudio_callback,
                           this,
                           &options)
            || rt->startStream()) {
            mlt_log_info(getConsumer(), "%s\n", rt->getErrorText().c_str());
            delete rt;
            rt = nullptr;
            return false;
        }
#else
        try {
            if (rt->isStreamOpen()) {
                rt->closeStream();
            }
            rt->openStream(&parameters,
                           nullptr,
                           RTAUDIO_SINT16,
                           frequency,
                           &bufferFrames,
                           &rtaudio_callback,
                           this,
                           &options);
            rt->startStream();
        }
#ifdef RTERROR_H
        catch (RtError &e) {
#else
        catch (RtAudioError &e) {
#endif
            mlt_log_info(getConsumer(), "%s\n", e.getMessage().c_str());
            delete rt;
            rt = nullptr;
            return false;
        }
#endif
        mlt_log_info(getConsumer(),
                     "Opened RtAudio: %s\t%d\t%d\n",
                     rtaudio_api_str(rt->getCurrentApi()),
                     channels,
                     frequency);
        return true;
    }

    bool find_and_create_rtaudio(int requested_channels, int frequency, int *actual_channels)
    {
        bool result = false;
#ifdef __WINDOWS_DS__
        // Prefer DirectSound on Windows
        RtAudio::Api PREFERRED_API = RtAudio::WINDOWS_DS;
#else
        RtAudio::Api PREFERRED_API = RtAudio::UNSPECIFIED;
#endif
        *actual_channels = requested_channels;

        // First try with preferred API.
        result = create_rtaudio(PREFERRED_API, *actual_channels, frequency);

        if (!result) {
            // If the preferred API fails, try other APIs that are available.
            std::vector<RtAudio::Api> apis;
            RtAudio::getCompiledApi(apis);
            for (size_t i = 0; i < apis.size(); i++) {
                if (apis[i] == PREFERRED_API || apis[i] == RtAudio::RTAUDIO_DUMMY) {
                    continue;
                }
                result = create_rtaudio(apis[i], *actual_channels, frequency);
                if (result) {
                    break;
                }
            }
        }

        if (!result && *actual_channels != 2) {
            // If surround has failed for all APIs, try stereo.
            *actual_channels = 2;
            mlt_log_info(getConsumer(),
                         "Unable to open %d channels. Try %d channels\n",
                         requested_channels,
                         *actual_channels);
            std::vector<RtAudio::Api> apis;
            RtAudio::getCompiledApi(apis);
            for (size_t i = 0; i < apis.size(); i++) {
                if (apis[i] == RtAudio::RTAUDIO_DUMMY) {
                    continue;
                }
                result = create_rtaudio(apis[i], *actual_channels, frequency);
                if (result) {
                    break;
                }
            }
        }

        return result;
    }

    bool open(const char *arg)
    {
        // Create the queue
        queue = mlt_deque_init();

        // get a handle on properties
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(&consumer);

        // Set the default volume
        mlt_properties_set_double(properties, "volume", 1.0);

        // This is the initialisation of the consumer
        pthread_mutex_init(&audio_mutex, nullptr);
        pthread_cond_init(&audio_cond, nullptr);
        pthread_mutex_init(&video_mutex, nullptr);
        pthread_cond_init(&video_cond, nullptr);

        // Default scaler (for now we'll use nearest)
        mlt_properties_set(properties, "rescale", "nearest");
        mlt_properties_set(properties, "consumer.deinterlacer", "onefield");

        // Default buffer for low latency
        mlt_properties_set_int(properties, "buffer", 1);

        // Default audio buffer
        mlt_properties_set_int(properties, "audio_buffer", 1024);

        // Set the resource to the device name arg
        mlt_properties_set(properties, "resource", arg);

        // Ensure we don't join on a non-running object
        joined = 1;

        // Initialize the refresh handler
        pthread_cond_init(&refresh_cond, nullptr);
        pthread_mutex_init(&refresh_mutex, nullptr);
        mlt_events_listen(properties, this, "property-changed", (mlt_listener) consumer_refresh_cb);

        return true;
    }

    int start()
    {
        if (!running) {
            stop();
            running = 1;
            joined = 0;
            pthread_create(&thread, nullptr, consumer_thread_proxy, this);
        }

        return 0;
    }

    int stop()
    {
        if (running && !joined) {
            // Kill the thread and clean up
            joined = 1;
            running = 0;

            // Unlatch the consumer thread
            pthread_mutex_lock(&refresh_mutex);
            pthread_cond_broadcast(&refresh_cond);
            pthread_mutex_unlock(&refresh_mutex);

            // Cleanup the main thread
            pthread_join(thread, nullptr);

            // Unlatch the video thread
            pthread_mutex_lock(&video_mutex);
            pthread_cond_broadcast(&video_cond);
            pthread_mutex_unlock(&video_mutex);

            // Unlatch the audio callback
            pthread_mutex_lock(&audio_mutex);
            pthread_cond_broadcast(&audio_cond);
            pthread_mutex_unlock(&audio_mutex);

            if (rt && rt->isStreamOpen())
#ifdef RTAUDIO_VERSION_6
                if (rt->stopStream()) {
                    mlt_log_error(getConsumer(), "%s\n", rt->getErrorText().c_str());
                }
#else
                try {
                    // Stop the stream
                    rt->stopStream();
                }
#ifdef RTERROR_H
                catch (RtError &e) {
#else
                catch (RtAudioError &e) {
#endif
                    mlt_log_error(getConsumer(), "%s\n", e.getMessage().c_str());
                }
#endif
            delete rt;
            rt = nullptr;
        }

        return 0;
    }

    void purge()
    {
        if (running) {
            pthread_mutex_lock(&video_mutex);
            mlt_frame frame = MLT_FRAME(mlt_deque_peek_back(queue));
            // When playing rewind or fast forward then we need to keep one
            // frame in the queue to prevent playback stalling.
            double speed = frame ? mlt_properties_get_double(MLT_FRAME_PROPERTIES(frame), "_speed")
                                 : 0;
            int n = (speed == 0.0 || speed == 1.0) ? 0 : 1;
            while (mlt_deque_count(queue) > n)
                mlt_frame_close(MLT_FRAME(mlt_deque_pop_back(queue)));
            is_purge = true;
            pthread_cond_broadcast(&video_cond);
            pthread_mutex_unlock(&video_mutex);
        }
    }

    void consumer_thread()
    {
        // Get the properties
        mlt_properties consumer_props = MLT_CONSUMER_PROPERTIES(getConsumer());

        // Video thread
        pthread_t thread;

        // internal initialization
        int init_audio = 1;
        int init_video = 1;
        mlt_frame frame = nullptr;
        mlt_properties properties = nullptr;
        int64_t duration = 0;
        int64_t playtime = mlt_properties_get_int(consumer_props, "video_delay") * 1000;
        struct timespec tm = {0, 100000};
        //	int last_position = -1;

        pthread_mutex_lock(&refresh_mutex);
        refresh_count = 0;
        pthread_mutex_unlock(&refresh_mutex);

        // Loop until told not to
        while (running) {
            // Get a frame from the attached producer
            frame = mlt_consumer_rt_frame(getConsumer());

            // Ensure that we have a frame
            if (frame) {
                // Get the frame properties
                properties = MLT_FRAME_PROPERTIES(frame);

                // Get the speed of the frame
                double speed = mlt_properties_get_double(properties, "_speed");

                // Get refresh request for the current frame
                int refresh = mlt_properties_get_int(consumer_props, "refresh");

                // Clear refresh
                mlt_events_block(consumer_props, consumer_props);
                mlt_properties_set_int(consumer_props, "refresh", 0);
                mlt_events_unblock(consumer_props, consumer_props);

                // Play audio
                init_audio = play_audio(frame, init_audio, &duration);

                // Determine the start time now
                if (playing && init_video) {
                    // Create the video thread
                    pthread_create(&thread, nullptr, video_thread_proxy, this);

                    // Video doesn't need to be initialised any more
                    init_video = 0;
                }

                // Set playtime for this frame in microseconds
                mlt_properties_set_int64(properties, "playtime", playtime);

                while (running && speed != 0 && mlt_deque_count(queue) > 15)
                    nanosleep(&tm, nullptr);

                // Push this frame to the back of the video queue
                if (running && speed) {
                    pthread_mutex_lock(&video_mutex);
                    if (is_purge && speed == 1.0) {
                        mlt_frame_close(frame);
                        is_purge = false;
                    } else {
                        mlt_deque_push_back(queue, frame);
                        pthread_cond_broadcast(&video_cond);
                    }
                    pthread_mutex_unlock(&video_mutex);

                    // Calculate the next playtime
                    playtime += duration;
                } else if (running) {
                    pthread_mutex_lock(&refresh_mutex);
                    if (refresh == 0 && refresh_count <= 0) {
                        play_video(frame);
                        pthread_cond_wait(&refresh_cond, &refresh_mutex);
                    }
                    mlt_frame_close(frame);
                    refresh_count--;
                    pthread_mutex_unlock(&refresh_mutex);
                } else {
                    mlt_frame_close(frame);
                    frame = nullptr;
                }

                // Optimisation to reduce latency
                if (frame && speed == 1.0) {
                    // TODO: disabled due to misbehavior on parallel-consumer
                    //				if ( last_position != -1 && last_position + 1 != mlt_frame_get_position( frame ) )
                    //					mlt_consumer_purge( consumer );
                    //				last_position = mlt_frame_get_position( frame );
                } else if (speed == 0.0) {
                    mlt_consumer_purge(getConsumer());
                    //				last_position = -1;
                }
            }
        }

        // Kill the video thread
        if (init_video == 0) {
            pthread_mutex_lock(&video_mutex);
            pthread_cond_broadcast(&video_cond);
            pthread_mutex_unlock(&video_mutex);
            pthread_join(thread, nullptr);
        }

        while (mlt_deque_count(queue))
            mlt_frame_close((mlt_frame) mlt_deque_pop_back(queue));

        audio_avail = 0;
    }

    int callback(int16_t *outbuf,
                 int16_t *inbuf,
                 unsigned int samples,
                 double streamTime,
                 RtAudioStreamStatus status)
    {
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(getConsumer());
        double volume = mlt_properties_get_double(properties, "volume");
        int len = mlt_audio_format_size(mlt_audio_s16, samples, out_channels);

        pthread_mutex_lock(&audio_mutex);

        // Block until audio received
        while (running && len > audio_avail)
            pthread_cond_wait(&audio_cond, &audio_mutex);

        if (audio_avail >= len) {
            // Place in the audio buffer
            memcpy(outbuf, audio_buffer, len);

            // Remove len from the audio available
            audio_avail -= len;

            // Remove the samples
            memmove(audio_buffer, audio_buffer + len, audio_avail);
        } else {
            // Just to be safe, wipe the stream first
            memset(outbuf, 0, len);

            // Copy what we have
            memcpy(outbuf, audio_buffer, audio_avail);

            // No audio left
            audio_avail = 0;
        }

        if (volume != 1.0) {
            int16_t *p = outbuf;
            int i = samples * out_channels + 1;
            while (--i)
                *p++ *= volume;
        }

        // We're definitely playing now
        playing = 1;

        pthread_cond_broadcast(&audio_cond);
        pthread_mutex_unlock(&audio_mutex);

        return 0;
    }

    int play_audio(mlt_frame frame, int init_audio, int64_t *duration)
    {
        // Get the properties of this consumer
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(getConsumer());
        mlt_audio_format afmt = mlt_audio_s16;

        // Set the preferred params of the test card signal
        int channels = mlt_properties_get_int(properties, "channels");
        int frequency = mlt_properties_get_int(properties, "frequency");
        int scrub = mlt_properties_get_int(properties, "scrub_audio");
        static int counter = 0;
        int samples = mlt_audio_calculate_frame_samples(mlt_properties_get_double(properties, "fps"),
                                                        frequency,
                                                        counter++);
        int16_t *pcm;

        mlt_frame_get_audio(frame, (void **) &pcm, &afmt, &frequency, &channels, &samples);
        *duration = 1000000LL * samples / frequency;

        if (mlt_properties_get_int(properties, "audio_off")) {
            playing = 1;
            return init_audio;
        }

        if (init_audio == 1) {
            if (find_and_create_rtaudio(channels, frequency, &out_channels)) {
                init_audio = 0;
                playing = 1;
            } else {
                rt = nullptr;
                mlt_log_error(getConsumer(), "Unable to initialize RtAudio\n");
                init_audio = 2;
            }
        }

        if (init_audio == 0) {
            mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
            int samples_copied = 0;
            int dst_stride = out_channels * sizeof(*pcm);

            pthread_mutex_lock(&audio_mutex);

            while (running && samples_copied < samples) {
                int sample_space = (sizeof(audio_buffer) - audio_avail) / dst_stride;

                while (running && sample_space == 0) {
                    pthread_cond_wait(&audio_cond, &audio_mutex);
                    sample_space = (sizeof(audio_buffer) - audio_avail) / dst_stride;
                }
                if (running) {
                    int samples_to_copy = samples - samples_copied;
                    if (samples_to_copy > sample_space) {
                        samples_to_copy = sample_space;
                    }
                    int dst_bytes = samples_to_copy * dst_stride;

                    if (scrub || mlt_properties_get_double(properties, "_speed") == 1) {
                        if (channels == out_channels) {
                            memcpy(&audio_buffer[audio_avail], pcm, dst_bytes);
                            pcm += samples_to_copy * channels;
                        } else {
                            int16_t *dest = (int16_t *) &audio_buffer[audio_avail];
                            int i = samples_to_copy + 1;
                            while (--i) {
                                memcpy(dest, pcm, dst_stride);
                                pcm += channels;
                                dest += out_channels;
                            }
                        }
                    } else {
                        memset(&audio_buffer[audio_avail], 0, dst_bytes);
                        pcm += samples_to_copy * channels;
                    }
                    audio_avail += dst_bytes;
                    samples_copied += samples_to_copy;
                }
                pthread_cond_broadcast(&audio_cond);
            }
            pthread_mutex_unlock(&audio_mutex);
        }

        return init_audio;
    }

    int play_video(mlt_frame frame)
    {
        // Get the properties of this consumer
        mlt_properties properties = MLT_CONSUMER_PROPERTIES(getConsumer());
        if (running && !mlt_consumer_is_stopped(getConsumer())) {
            mlt_events_fire(properties, "consumer-frame-show", mlt_event_data_from_frame(frame));
        }

        return 0;
    }

    void video_thread()
    {
        // Obtain time of thread start
        struct timeval now;
        int64_t start = 0;
        int64_t elapsed = 0;
        struct timespec tm;
        mlt_frame next = nullptr;
        mlt_properties consumerProperties = MLT_CONSUMER_PROPERTIES(getConsumer());
        double speed = 0;

        // Get real time flag
        int real_time = mlt_properties_get_int(consumerProperties, "real_time");

        // Get the current time
        gettimeofday(&now, nullptr);

        // Determine start time
        start = (int64_t) now.tv_sec * 1000000 + now.tv_usec;

        while (running) {
            // Pop the next frame
            pthread_mutex_lock(&video_mutex);
            next = (mlt_frame) mlt_deque_pop_front(queue);
            while (next == nullptr && running) {
                pthread_cond_wait(&video_cond, &video_mutex);
                next = (mlt_frame) mlt_deque_pop_front(queue);
            }
            pthread_mutex_unlock(&video_mutex);

            if (!running || next == nullptr)
                break;

            // Get the properties
            mlt_properties properties = MLT_FRAME_PROPERTIES(next);

            // Get the speed of the frame
            speed = mlt_properties_get_double(properties, "_speed");

            // Get the current time
            gettimeofday(&now, nullptr);

            // Get the elapsed time
            elapsed = ((int64_t) now.tv_sec * 1000000 + now.tv_usec) - start;

            // See if we have to delay the display of the current frame
            if (mlt_properties_get_int(properties, "rendered") == 1 && running) {
                // Obtain the scheduled playout time in microseconds
                int64_t scheduled = mlt_properties_get_int64(properties, "playtime");

                // Determine the difference between the elapsed time and the scheduled playout time
                int64_t difference = scheduled - elapsed;

                // Smooth playback a bit
                if (real_time && (difference > 20000 && speed == 1.0)) {
                    tm.tv_sec = difference / 1000000;
                    tm.tv_nsec = (difference % 1000000) * 1000;
                    nanosleep(&tm, nullptr);
                }

                // Show current frame if not too old
                if (!real_time
                    || (difference > -10000 || speed != 1.0 || mlt_deque_count(queue) < 2))
                    play_video(next);

                // If the queue is empty, recalculate start to allow build up again
                if (real_time && (mlt_deque_count(queue) == 0 && speed == 1.0)) {
                    gettimeofday(&now, nullptr);
                    start = ((int64_t) now.tv_sec * 1000000 + now.tv_usec) - scheduled + 20000;
                    start += mlt_properties_get_int(consumerProperties, "video_delay") * 1000;
                }
            }

            // This frame can now be closed
            mlt_frame_close(next);
            next = nullptr;
        }

        if (next != nullptr)
            mlt_frame_close(next);

        mlt_consumer_stopped(getConsumer());
    }
};

static void consumer_refresh_cb(mlt_consumer sdl, mlt_consumer consumer, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);
    if (name && !strcmp(name, "refresh")) {
        RtAudioConsumer *rtaudio = (RtAudioConsumer *) consumer->child;
        pthread_mutex_lock(&rtaudio->refresh_mutex);
        rtaudio->refresh_count = rtaudio->refresh_count <= 0 ? 1 : rtaudio->refresh_count + 1;
        pthread_cond_broadcast(&rtaudio->refresh_cond);
        pthread_mutex_unlock(&rtaudio->refresh_mutex);
    }
}

static int rtaudio_callback(void *outputBuffer,
                            void *inputBuffer,
                            unsigned int nFrames,
                            double streamTime,
                            RtAudioStreamStatus status,
                            void *userData)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) userData;
    return rtaudio->callback((int16_t *) outputBuffer,
                             (int16_t *) inputBuffer,
                             nFrames,
                             streamTime,
                             status);
}

static void *consumer_thread_proxy(void *arg)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) arg;
    rtaudio->consumer_thread();
    return nullptr;
}

static void *video_thread_proxy(void *arg)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) arg;
    rtaudio->video_thread();
    return nullptr;
}

/** Start the consumer.
 */

static int start(mlt_consumer consumer)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) consumer->child;
    return rtaudio->start();
}

/** Stop the consumer.
 */

static int stop(mlt_consumer consumer)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) consumer->child;
    return rtaudio->stop();
}

/** Determine if the consumer is stopped.
 */

static int is_stopped(mlt_consumer consumer)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) consumer->child;
    return !rtaudio->running;
}

static void purge(mlt_consumer consumer)
{
    RtAudioConsumer *rtaudio = (RtAudioConsumer *) consumer->child;
    rtaudio->purge();
}

/** Close the consumer.
 */

static void close(mlt_consumer consumer)
{
    // Stop the consumer
    mlt_consumer_stop(consumer);

    // Close the parent
    consumer->close = nullptr;
    mlt_consumer_close(consumer);

    // Free the memory
    delete (RtAudioConsumer *) consumer->child;
}

extern "C" {

mlt_consumer consumer_rtaudio_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg)
{
    // Allocate the consumer
    RtAudioConsumer *rtaudio = new RtAudioConsumer();
    mlt_consumer consumer = nullptr;

    // If allocated
    if (rtaudio && !mlt_consumer_init(rtaudio->getConsumer(), rtaudio, profile)) {
        // If initialises without error
        if (rtaudio->open(arg ? arg : getenv("AUDIODEV"))) {
            // Setup callbacks
            consumer = rtaudio->getConsumer();
            consumer->close = close;
            consumer->start = start;
            consumer->stop = stop;
            consumer->is_stopped = is_stopped;
            consumer->purge = purge;
        } else {
            mlt_consumer_close(rtaudio->getConsumer());
            delete rtaudio;
        }
    }

    // Return consumer
    return consumer;
}

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    const char *service_type = "consumer";
    snprintf(file, PATH_MAX, "%s/rtaudio/%s_%s.yml", mlt_environment("MLT_DATA"), service_type, id);
    return mlt_properties_parse_yaml(file);
}

MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_consumer_type, "rtaudio", consumer_rtaudio_init);
    MLT_REGISTER_METADATA(mlt_service_consumer_type, "rtaudio", metadata, nullptr);
}

} // extern C
