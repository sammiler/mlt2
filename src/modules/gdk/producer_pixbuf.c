/*
 * producer_pixbuf.c -- raster image loader based upon gdk-pixbuf
 * Copyright (C) 2003-2023 Meltytech, LLC
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt_cache.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_tokeniser.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef USE_EXIF
#include <libexif/exif-data.h>
#endif

#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#ifdef _MSC_VER
    #include <io.h>      // For _close, _write
    #include <stdio.h>
    #include <BaseTsd.h> // For SSIZE_T

    // Define ssize_t for MSVC
    typedef SSIZE_T ssize_t;

// --- 使用函数包装器来避免与结构体成员冲突 ---

// close 的包装器
static inline int close(int fd)
{
    return _close(fd);
}

// write 的包装器
static inline int write(int fd, const void *buffer, unsigned int count)
{
    return _write(fd, buffer, count);
}
#endif

// this protects concurrent access to gdk_pixbuf
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct producer_pixbuf_s *producer_pixbuf;

struct producer_pixbuf_s
{
    struct mlt_producer_s parent;

    // File name list
    mlt_properties filenames;
    mlt_position *outs;
    int count;
    int image_idx;
    int pixbuf_idx;
    int width;
    int height;
    uint8_t *alpha;
    uint8_t *image;
    mlt_cache_item image_cache;
    mlt_cache_item alpha_cache;
    mlt_cache_item pixbuf_cache;
    GdkPixbuf *pixbuf;
    mlt_image_format format;
};

static void load_filenames(producer_pixbuf self, mlt_properties producer_properties);
static int refresh_pixbuf(producer_pixbuf self, mlt_frame frame);
static int producer_get_frame(mlt_producer parent, mlt_frame_ptr frame, int index);
static void producer_close(mlt_producer parent);

static void refresh_length(mlt_properties properties, producer_pixbuf self)
{
    if (self->count > mlt_properties_get_int(properties, "length")
        || mlt_properties_get_int(properties, "autolength")) {
        int ttl = mlt_properties_get_int(properties, "ttl");
        mlt_position length = self->count * ttl;
        mlt_properties_set_position(properties, "length", length);
        mlt_properties_set_position(properties, "out", length - 1);
    }
}

static void on_property_changed(mlt_service owner, mlt_producer producer, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);
    if (name && !strcmp(name, "ttl"))
        refresh_length(MLT_PRODUCER_PROPERTIES(producer), producer->child);
}

mlt_producer producer_pixbuf_init(char *filename)
{
    producer_pixbuf self = calloc(1, sizeof(struct producer_pixbuf_s));
    if (self != NULL && mlt_producer_init(&self->parent, self) == 0) {
        mlt_producer producer = &self->parent;

        // Get the properties interface
        mlt_properties properties = MLT_PRODUCER_PROPERTIES(&self->parent);

        // Reject if animation.
        GError *error = NULL;
        pthread_mutex_lock(&g_mutex);
        GdkPixbufAnimation *anim = gdk_pixbuf_animation_new_from_file(filename, &error);
        if (anim) {
            gboolean is_anim = !gdk_pixbuf_animation_is_static_image(anim);
            g_object_unref(anim);
            if (is_anim) {
                pthread_mutex_unlock(&g_mutex);
                mlt_producer_close(&self->parent);
                free(self);
                return NULL;
            }
        }
        pthread_mutex_unlock(&g_mutex);

        // Callback registration
        producer->get_frame = producer_get_frame;
        producer->close = (mlt_destructor) producer_close;

        // Set the default properties
        mlt_properties_set(properties, "resource", filename);
        mlt_properties_set_int(properties, "ttl", 25);
        mlt_properties_set_int(properties, "aspect_ratio", 1);
        mlt_properties_set_int(properties, "progressive", 1);
        mlt_properties_set_int(properties, "seekable", 1);
        mlt_properties_set_int(properties, "loop", 1);
        mlt_properties_set_int(properties, "meta.media.progressive", 1);

        // Validate the resource
        if (filename)
            load_filenames(self, properties);
        if (self->count) {
            mlt_frame frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));
            if (frame) {
                mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
                mlt_properties_set_data(frame_properties, "producer_pixbuf", self, 0, NULL, NULL);
                mlt_frame_set_position(frame, mlt_producer_position(producer));
                refresh_pixbuf(self, frame);
                mlt_cache_item_close(self->pixbuf_cache);
                mlt_frame_close(frame);
            }
        }
        if (self->width == 0) {
            producer_close(producer);
            producer = NULL;
        } else {
            mlt_events_listen(properties,
                              self,
                              "property-changed",
                              (mlt_listener) on_property_changed);
        }
        return producer;
    }
    free(self);
    return NULL;
}

static int load_svg(producer_pixbuf self, mlt_properties properties, const char *filename)
{
    int result = 0;

    // Read xml string
    if (strstr(filename, "<svg")) {
        // Generate a temporary file for the svg
        char fullname[1024] = "/tmp/mlt.XXXXXX";
        int fd = g_mkstemp(fullname);

        if (fd > -1) {
            // Write the svg into the temp file
            ssize_t remaining_bytes;
            const char *xml = filename;

            // Strip leading crap
            while (xml[0] != '<')
                xml++;

            remaining_bytes = strlen(xml);
            while (remaining_bytes > 0)
                remaining_bytes -= write(fd, xml + strlen(xml) - remaining_bytes, remaining_bytes);
            close(fd);

            mlt_properties_set(self->filenames, "0", fullname);

            // Teehe - when the producer closes, delete the temp file and the space allo
            mlt_properties_set_data(properties,
                                    "__temporary_file__",
                                    fullname,
                                    0,
                                    (mlt_destructor) unlink,
                                    NULL);
            result = 1;
        }
    }
    return result;
}

static int load_sequence_sprintf(producer_pixbuf self,
                                 mlt_properties properties,
                                 const char *filename)
{
    int result = 0;

    // Obtain filenames with pattern
    if (strchr(filename, '%') != NULL) {
        // handle picture sequences
        int i = mlt_properties_get_int(properties, "begin");
        int gap = 0;
        char full[1024];
        int keyvalue = 0;
        char key[50];

        while (gap < 100) {
            struct stat buf;
            snprintf(full, 1023, filename, i++);
            if (mlt_stat(full, &buf) == 0) {
                sprintf(key, "%d", keyvalue++);
                mlt_properties_set(self->filenames, key, full);
                gap = 0;
            } else {
                gap++;
            }
        }
        if (mlt_properties_count(self->filenames) > 0) {
            mlt_properties_set_int(properties, "ttl", 1);
            result = 1;
        }
    }
    return result;
}

static int load_sequence_deprecated(producer_pixbuf self,
                                    mlt_properties properties,
                                    const char *filename)
{
    int result = 0;
    const char *start;

    // This approach is deprecated in favor of the begin querystring parameter.
    // Obtain filenames with pattern containing a begin value, e.g. foo%1234d.png
    if ((start = strchr(filename, '%'))) {
        const char *end = ++start;
        while (isdigit(*end))
            end++;
        if (end > start && (end[0] == 'd' || end[0] == 'i' || end[0] == 'u')) {
            int n = end - start;
            char *s = calloc(1, n + 1);
            strncpy(s, start, n);
            mlt_properties_set(properties, "begin", s);
            free(s);
            s = calloc(1, strlen(filename) + 2);
            strncpy(s, filename, start - filename);
            sprintf(s + (start - filename), ".%d%s", n, end);
            result = load_sequence_sprintf(self, properties, s);
            free(s);
        }
    }
    return result;
}

static int load_sequence_querystring(producer_pixbuf self,
                                     mlt_properties properties,
                                     const char *filename)
{
    int result = 0;

    // Obtain filenames with pattern and begin value in query string
    if (strchr(filename, '%') && strchr(filename, '?')) {
        // Split filename into pattern and query string
        char *s = strdup(filename);
        char *querystring = strrchr(s, '?');
        *querystring++ = '\0';
        if (strstr(filename, "begin="))
            mlt_properties_set(properties, "begin", strstr(querystring, "begin=") + 6);
        else if (strstr(filename, "begin:"))
            mlt_properties_set(properties, "begin", strstr(querystring, "begin:") + 6);
        // Coerce to an int value so serialization does not have any extra query string cruft
        mlt_properties_set_int(properties, "begin", mlt_properties_get_int(properties, "begin"));
        result = load_sequence_sprintf(self, properties, s);
        free(s);
    }
    return result;
}

static int load_folder(producer_pixbuf self, mlt_properties properties, const char *filename)
{
    int result = 0;

    // Obtain filenames with folder
    if (strstr(filename, "/.all.") != NULL) {
        char wildcard[1024];
        char *dir_name = strdup(filename);
        char *extension = strrchr(dir_name, '.');

        *(strstr(dir_name, "/.all.") + 1) = '\0';
        sprintf(wildcard, "*%s", extension);

        mlt_properties_dir_list(self->filenames, dir_name, wildcard, 1);

        free(dir_name);
        result = 1;
    }
    return result;
}

static int load_sequence_csv(producer_pixbuf self, mlt_properties properties, const char *filename)
{
    int result = 0;
    const char *csv_extension = strstr(filename, ".csv");

    if (csv_extension != NULL && csv_extension[4] == '\0') {
        FILE *csv = fopen(filename, "r");
        if (csv != NULL) {
            int keyvalue = 0;
            int out = 0;

            int nlines = 0;
            while (!feof(csv)) {
                char line[1024];
                if (fgets(line, 1024, csv) != NULL) {
                    nlines++;
                }
            }

            self->outs = (mlt_position *) malloc(nlines * sizeof(mlt_position));

            fseek(csv, 0, SEEK_SET);
            int index = 0;
            while (!feof(csv)) {
                char line[1024];
                if (fgets(line, 1024, csv) != NULL) {
                    char *sep = strchr(line, ';');
                    if (sep != NULL) {
                        int ttl = 0;
                        int n = 0;
                        char key[50];
                        struct stat buf;

                        *sep++ = '\0';
                        n = sscanf(sep, "%d", &ttl);
                        if (n == 0) {
                            break;
                        }

                        if (mlt_stat(line, &buf) != 0) {
                            break;
                        }

                        out += ttl;
                        mlt_log_debug(MLT_PRODUCER_SERVICE(&self->parent),
                                      "file:'%s' ttl=%d out=%d\n",
                                      line,
                                      ttl,
                                      out);

                        sprintf(key, "%d", keyvalue++);
                        mlt_properties_set(self->filenames, key, line);
                        self->outs[index++] = out;
                    }
                }
            }

            fclose(csv);
            result = 1;
        }
    }

    return result;
}

static void load_filenames(producer_pixbuf self, mlt_properties properties)
{
    char *filename = mlt_properties_get(properties, "resource");
    self->filenames = mlt_properties_new();
    self->outs = NULL;

    if (!load_svg(self, properties, filename)
        && !load_sequence_querystring(self, properties, filename)
        && !load_sequence_sprintf(self, properties, filename)
        && !load_sequence_deprecated(self, properties, filename)
        && !load_sequence_csv(self, properties, filename)
        && !load_folder(self, properties, filename)) {
        mlt_properties_set(self->filenames, "0", filename);
    }
    self->count = mlt_properties_count(self->filenames);
    refresh_length(properties, self);
}

static GdkPixbuf *reorient_with_exif(producer_pixbuf self, int image_idx, GdkPixbuf *pixbuf)
{
#ifdef USE_EXIF
    mlt_properties producer_props = MLT_PRODUCER_PROPERTIES(&self->parent);
    ExifData *d = exif_data_new_from_file(mlt_properties_get_value(self->filenames, image_idx));
    ExifEntry *entry;
    int exif_orientation = 0;

    /* get orientation and rotate image accordingly if necessary */
    if (d) {
        if ((entry = exif_content_get_entry(d->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION)))
            exif_orientation = exif_get_short(entry->data, exif_data_get_byte_order(d));

        /* Free the EXIF data */
        exif_data_unref(d);
    }

    // Remember EXIF value, might be useful for someone
    mlt_properties_set_int(producer_props, "_exif_orientation", exif_orientation);

    if (exif_orientation > 1) {
        GdkPixbuf *processed = NULL;
        GdkPixbufRotation matrix = GDK_PIXBUF_ROTATE_NONE;

        // Rotate image according to exif data
        switch (exif_orientation) {
        case 2:
            processed = gdk_pixbuf_flip(pixbuf, TRUE);
            break;
        case 3:
            matrix = GDK_PIXBUF_ROTATE_UPSIDEDOWN;
            processed = pixbuf;
            break;
        case 4:
            processed = gdk_pixbuf_flip(pixbuf, FALSE);
            break;
        case 5:
            matrix = GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE;
            processed = gdk_pixbuf_flip(pixbuf, TRUE);
            break;
        case 6:
            matrix = GDK_PIXBUF_ROTATE_CLOCKWISE;
            processed = pixbuf;
            break;
        case 7:
            matrix = GDK_PIXBUF_ROTATE_CLOCKWISE;
            processed = gdk_pixbuf_flip(pixbuf, TRUE);
            break;
        case 8:
            matrix = GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE;
            processed = pixbuf;
            break;
        }
        if (processed) {
            pixbuf = gdk_pixbuf_rotate_simple(processed, matrix);
            g_object_unref(processed);
        }
    }
#endif
    return pixbuf;
}

static int refresh_pixbuf(producer_pixbuf self, mlt_frame frame)
{
    // Obtain properties of frame and producer
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_producer producer = &self->parent;
    mlt_properties producer_props = MLT_PRODUCER_PROPERTIES(producer);

    // Check if user wants us to reload the image
    if (mlt_properties_get_int(producer_props, "force_reload")) {
        self->pixbuf = NULL;
        self->image = NULL;
        mlt_properties_set_int(producer_props, "force_reload", 0);
    }

    // Get the original position of this frame
    mlt_position position = mlt_frame_original_position(frame);
    position += mlt_producer_get_in(producer);

    int loop = mlt_properties_get_int(producer_props, "loop");

    // Image index
    int current_idx = 0;
    if (!self->outs) {
        // Get the time to live for each frame
        double ttl = mlt_properties_get_int(producer_props, "ttl");
        if (loop) {
            current_idx = (int) floor((double) position / ttl) % self->count;
        } else {
            current_idx = MIN((double) position / ttl, self->count - 1);
        }
    } else {
        int total_ttl = (int) floor(self->outs[self->count - 1]);
        mlt_position looped_pos = loop ? (int) floor(position) % total_ttl : position;

        while (current_idx < self->count) {
            if (self->outs[current_idx] > looped_pos) {
                break;
            }

            current_idx++;
        }

        if (current_idx >= self->count) {
            current_idx = self->count - 1;
        }

        mlt_log_debug(MLT_PRODUCER_SERVICE(&self->parent),
                      "position=%d current_idx=%d\n",
                      position,
                      current_idx);
    }

    int disable_exif = mlt_properties_get_int(producer_props, "disable_exif");

    if (current_idx != self->pixbuf_idx)
        self->pixbuf = NULL;
    if (!self->pixbuf || mlt_properties_get_int(producer_props, "_disable_exif") != disable_exif) {
        GError *error = NULL;

        self->image = NULL;
        pthread_mutex_lock(&g_mutex);
        self->pixbuf = gdk_pixbuf_new_from_file(mlt_properties_get_value(self->filenames,
                                                                         current_idx),
                                                &error);
        if (self->pixbuf) {
            // Read the exif value for this file
            if (!disable_exif)
                self->pixbuf = reorient_with_exif(self, current_idx, self->pixbuf);

            // Register this pixbuf for destruction and reuse
            mlt_cache_item_close(self->pixbuf_cache);
            mlt_service_cache_put(MLT_PRODUCER_SERVICE(producer),
                                  "pixbuf.pixbuf",
                                  self->pixbuf,
                                  0,
                                  (mlt_destructor) g_object_unref);
            self->pixbuf_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer),
                                                       "pixbuf.pixbuf");
            self->pixbuf_idx = current_idx;

            // Store the width/height of the pixbuf temporarily
            self->width = gdk_pixbuf_get_width(self->pixbuf);
            self->height = gdk_pixbuf_get_height(self->pixbuf);

            mlt_events_block(producer_props, NULL);
            mlt_properties_set_int(producer_props, "meta.media.width", self->width);
            mlt_properties_set_int(producer_props, "meta.media.height", self->height);
            mlt_properties_set_int(producer_props, "_disable_exif", disable_exif);
            int has_alpha = gdk_pixbuf_get_has_alpha(self->pixbuf);
            mlt_properties_set_int(properties, "format", has_alpha ? mlt_image_rgba : mlt_image_rgb);
            mlt_events_unblock(producer_props, NULL);
        }
        pthread_mutex_unlock(&g_mutex);
    }

    // Set width/height of frame
    mlt_properties_set_int(properties, "width", self->width);
    mlt_properties_set_int(properties, "height", self->height);

    return current_idx;
}

static void refresh_image(
    producer_pixbuf self, mlt_frame frame, mlt_image_format format, int width, int height)
{
    // Obtain properties of frame and producer
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_producer producer = &self->parent;

    // Get index and pixbuf
    int current_idx = refresh_pixbuf(self, frame);

    // optimization for subsequent iterations on single picture
    if (current_idx != self->image_idx || width != self->width || height != self->height)
        self->image = NULL;
    mlt_log_debug(MLT_PRODUCER_SERVICE(producer),
                  "image %p pixbuf %p idx %d current_idx %d pixbuf_idx %d width %d\n",
                  self->image,
                  self->pixbuf,
                  current_idx,
                  self->image_idx,
                  self->pixbuf_idx,
                  width);

    // If we have a pixbuf and we need an image
    if (self->pixbuf
        && (!self->image
            || (format != mlt_image_none && format != mlt_image_movit && format != self->format))) {
        char *interps = mlt_properties_get(properties, "consumer.rescale");
        if (interps)
            interps = strdup(interps);
        int interp = GDK_INTERP_BILINEAR;

        if (!interps) {
            // Keep bilinear by default
        } else if (strcmp(interps, "nearest") == 0)
            interp = GDK_INTERP_NEAREST;
        else if (strcmp(interps, "tiles") == 0)
            interp = GDK_INTERP_TILES;
        else if (strcmp(interps, "hyper") == 0 || strcmp(interps, "bicubic") == 0)
            interp = GDK_INTERP_HYPER;
        free(interps);

        // Note - the original pixbuf is already safe and ready for destruction
        pthread_mutex_lock(&g_mutex);
        GdkPixbuf *pixbuf = gdk_pixbuf_scale_simple(self->pixbuf, width, height, interp);

        // Store width and height
        self->width = width;
        self->height = height;

        // Allocate/define image
        int has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
        int src_stride = gdk_pixbuf_get_rowstride(pixbuf);
        int dst_stride = self->width * (has_alpha ? 4 : 3);
        self->format = has_alpha ? mlt_image_rgba : mlt_image_rgb;
        int image_size = mlt_image_format_size(self->format, width, height, NULL);
        self->image = mlt_pool_alloc(image_size);
        self->alpha = NULL;

        if (src_stride != dst_stride) {
            int y = self->height;
            uint8_t *src = gdk_pixbuf_get_pixels(pixbuf);
            uint8_t *dst = self->image;
            while (y--) {
                memcpy(dst, src, dst_stride);
                dst += dst_stride;
                src += src_stride;
            }
        } else {
            memcpy(self->image, gdk_pixbuf_get_pixels(pixbuf), src_stride * height);
        }
        pthread_mutex_unlock(&g_mutex);

        // Convert image to requested format
        if (format != mlt_image_none && format != mlt_image_movit && format != self->format
            && frame->convert_image) {
            // cache copies of the image and alpha buffers
            uint8_t *buffer = self->image;
            if (buffer) {
                mlt_frame_set_image(frame, self->image, image_size, mlt_pool_release);
                mlt_properties_set_int(properties, "width", self->width);
                mlt_properties_set_int(properties, "height", self->height);
                mlt_properties_set_int(properties, "format", self->format);

                if (!frame->convert_image(frame, &self->image, &self->format, format)) {
                    buffer = self->image;
                    image_size
                        = mlt_image_format_size(self->format, self->width, self->height, NULL);
                    self->image = mlt_pool_alloc(image_size);
                    memcpy(self->image,
                           buffer,
                           mlt_image_format_size(self->format, self->width, self->height, NULL));
                }
            }
            if ((buffer = mlt_frame_get_alpha(frame))) {
                self->alpha = mlt_pool_alloc(width * height);
                memcpy(self->alpha, buffer, width * height);
            }
        }

        // Update the cache
        mlt_cache_item_close(self->image_cache);
        mlt_service_cache_put(MLT_PRODUCER_SERVICE(producer),
                              "pixbuf.image",
                              self->image,
                              image_size,
                              mlt_pool_release);
        self->image_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer), "pixbuf.image");
        self->image_idx = current_idx;
        mlt_cache_item_close(self->alpha_cache);
        self->alpha_cache = NULL;
        if (self->alpha) {
            mlt_service_cache_put(MLT_PRODUCER_SERVICE(producer),
                                  "pixbuf.alpha",
                                  self->alpha,
                                  width * height,
                                  mlt_pool_release);
            self->alpha_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer),
                                                      "pixbuf.alpha");
        }

        // Finished with pixbuf now
        g_object_unref(pixbuf);
    }

    // Set width/height of frame
    mlt_properties_set_int(properties, "width", self->width);
    mlt_properties_set_int(properties, "height", self->height);
}

static int producer_get_image(mlt_frame frame,
                              uint8_t **buffer,
                              mlt_image_format *format,
                              int *width,
                              int *height,
                              int writable)
{
    int error = 0;

    // Obtain properties of frame and producer
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    producer_pixbuf self = mlt_properties_get_data(properties, "producer_pixbuf", NULL);
    mlt_producer producer = &self->parent;

    // Use the width and height suggested by the rescale filter because we can do our own scaling.
    if (mlt_properties_get_int(properties, "rescale_width") > 0)
        *width = mlt_properties_get_int(properties, "rescale_width");
    if (mlt_properties_get_int(properties, "rescale_height") > 0)
        *height = mlt_properties_get_int(properties, "rescale_height");

    // Restore pixbuf and image
    mlt_service_lock(MLT_PRODUCER_SERVICE(producer));
    self->pixbuf_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer), "pixbuf.pixbuf");
    self->pixbuf = mlt_cache_item_data(self->pixbuf_cache, NULL);
    self->image_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer), "pixbuf.image");
    self->image = mlt_cache_item_data(self->image_cache, NULL);
    self->alpha_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer), "pixbuf.alpha");
    self->alpha = mlt_cache_item_data(self->alpha_cache, NULL);

    // Refresh the image
    refresh_image(self, frame, *format, *width, *height);

    // Get width and height (may have changed during the refresh)
    *width = self->width;
    *height = self->height;
    *format = self->format;

    // NB: Cloning is necessary with this producer (due to processing of images ahead of use)
    // The fault is not in the design of mlt, but in the implementation of the pixbuf producer...
    if (self->image) {
        // Clone the image
        int image_size = mlt_image_format_size(self->format, self->width, self->height, NULL);
        uint8_t *image_copy = mlt_pool_alloc(image_size);
        memcpy(image_copy,
               self->image,
               mlt_image_format_size(self->format, self->width, self->height, NULL));
        // Now update properties so we free the copy after
        mlt_frame_set_image(frame, image_copy, image_size, mlt_pool_release);
        // We're going to pass the copy on
        *buffer = image_copy;
        mlt_log_debug(MLT_PRODUCER_SERVICE(&self->parent),
                      "%dx%d (%s)\n",
                      self->width,
                      self->height,
                      mlt_image_format_name(*format));
        // Clone the alpha channel
        if (self->alpha) {
            image_copy = mlt_pool_alloc(self->width * self->height);
            memcpy(image_copy, self->alpha, self->width * self->height);
            mlt_frame_set_alpha(frame, image_copy, self->width * self->height, mlt_pool_release);
        }
    } else {
        error = 1;
    }

    // Release references and locks
    mlt_cache_item_close(self->pixbuf_cache);
    mlt_cache_item_close(self->image_cache);
    mlt_cache_item_close(self->alpha_cache);
    mlt_service_unlock(MLT_PRODUCER_SERVICE(&self->parent));

    return error;
}

static int producer_get_frame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    // Get the real structure for this producer
    producer_pixbuf self = producer->child;

    // Fetch the producers properties
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);

    if (self->filenames == NULL && mlt_properties_get(producer_properties, "resource") != NULL)
        load_filenames(self, producer_properties);

    // Generate a frame
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));

    if (*frame != NULL && self->count > 0) {
        // Obtain properties of frame and producer
        mlt_properties properties = MLT_FRAME_PROPERTIES(*frame);

        // Set the producer on the frame properties
        mlt_properties_set_data(properties, "producer_pixbuf", self, 0, NULL, NULL);

        // Update timecode on the frame we're creating
        mlt_frame_set_position(*frame, mlt_producer_position(producer));

        // Refresh the pixbuf
        self->pixbuf_cache = mlt_service_cache_get(MLT_PRODUCER_SERVICE(producer), "pixbuf.pixbuf");
        self->pixbuf = mlt_cache_item_data(self->pixbuf_cache, NULL);
        refresh_pixbuf(self, *frame);
        mlt_cache_item_close(self->pixbuf_cache);

        // Set producer-specific frame properties
        mlt_properties_set_int(properties,
                               "progressive",
                               mlt_properties_get_int(producer_properties, "progressive"));

        double force_ratio = mlt_properties_get_double(producer_properties, "force_aspect_ratio");
        if (force_ratio > 0.0)
            mlt_properties_set_double(properties, "aspect_ratio", force_ratio);
        else
            mlt_properties_set_double(properties,
                                      "aspect_ratio",
                                      mlt_properties_get_double(producer_properties,
                                                                "aspect_ratio"));

        // Push the get_image method
        mlt_frame_push_get_image(*frame, producer_get_image);
    }

    // Calculate the next timecode
    mlt_producer_prepare_next(producer);

    return 0;
}

static void producer_close(mlt_producer parent)
{
    producer_pixbuf self = parent->child;
    parent->close = NULL;
    mlt_service_cache_purge(MLT_PRODUCER_SERVICE(parent));
    mlt_producer_close(parent);
    free(self->outs);
    self->outs = NULL;
    mlt_properties_close(self->filenames);
    free(self);
}
