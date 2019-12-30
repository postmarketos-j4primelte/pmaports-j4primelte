/*
Copyright (C) 2019 Daniele Debernardi <drebrez@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Based on:
- https://gitlab.freedesktop.org/mesa/drm/blob/master/tests/modetest/modetest.c
- https://gitlab.freedesktop.org/mesa/drm/blob/master/tests/util/kms.c

*/

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xf86drm.h"
#include "xf86drmMode.h"

static void dump_connectors(drmModeRes *res, int fd);
static void dump_encoders(drmModeRes *res, int fd);
static void dump_framebuffers(drmModeRes *res, int fd);
static void dump_crtcs(drmModeRes *res, int fd);
static void dump_planes(drmModeRes *res, int fd);

const char *util_lookup_connector_status_name(unsigned int status);
const char *util_lookup_connector_type_name(unsigned int type);
const char *util_lookup_encoder_type_name(unsigned int type);

void usage(char *appname)
{
    printf("Usage: %s [-d DEV] [-c] [-e] [-f] [-p] [-h]\n\
    -d  DRM device (default /dev/dri/card0)\n\n\
    Query options:\n\
        -c  list connectors\n\
        -e  list encoders\n\
        -f  list framebuffers\n\
        -p  list CRTCs and planes (pipes)\n\
    -h  Show this help\n",
        appname);
}

int main(int argc, char **argv)
{
    const char *drm_device;
    int drm_fd;
    drmModeRes *res;

    // parse command line options
    drm_device = "/dev/dri/card0";
    bool list_connectors = false;
    bool list_encoders = false;
    bool list_framebuffers = false;
    bool list_crtcs = false;

    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }
    int opt;
    while ((opt = getopt(argc, argv, "d:cefph")) != -1) {
        switch (opt) {
        case 'd':
            drm_device = optarg;
            break;
        case 'c':
            list_connectors = true;
            break;
        case 'e':
            list_encoders = true;
            break;
        case 'f':
            list_framebuffers = true;
            break;
        case 'p':
            list_crtcs = true;
            break;
        case 'h':
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    // open the DRM device
    drm_fd = open(drm_device, O_RDWR);
    if (drm_fd < 0) {
        printf("Unable to open %s.\n", drm_device);
        return 1;
    }

    // By default only overlay planes are available, this allows to
    // receive a universal plane list containing all plane types
    drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

    // retrieve current display configuration information
    res = drmModeGetResources(drm_fd);
    if (!res) {
        printf("Unable to get resources: %s\n", strerror(errno));
        return 1;
    }
 
    if (list_connectors)
        dump_connectors(res, drm_fd);

    if (list_encoders)
        dump_encoders(res, drm_fd);

    if (list_framebuffers)
        dump_framebuffers(res, drm_fd);

    if (list_crtcs) {
        dump_crtcs(res, drm_fd);
        dump_planes(res, drm_fd);
    }

    // close the DRM device
    drmModeFreeResources(res);
    close(drm_fd);
}

static float mode_vrefresh(drmModeModeInfo *mode)
{
    return  mode->clock * 1000.00
            / (mode->htotal * mode->vtotal);
}

static void dump_mode(drmModeModeInfo *mode)
{
    printf("\t%s %.2f %d %d %d %d %d %d %d %d %d",
           mode->name,
           mode_vrefresh(mode),
           mode->hdisplay,
           mode->hsync_start,
           mode->hsync_end,
           mode->htotal,
           mode->vdisplay,
           mode->vsync_start,
           mode->vsync_end,
           mode->vtotal,
           mode->clock);

    /*printf(" flags: ");
    mode_flag_str(mode->flags);
    printf("; type: ");
    mode_type_str(mode->type);*/
    printf("\n");
}

static void dump_connectors(drmModeRes *res, int fd)
{
    drmModeConnector *connector;
    int i, j;
    char *name;

    printf("Connectors (%d):\n", res->count_connectors);
    printf("id\tencoder\tstatus\t\tname\t\tsize (mm)\tmodes\tencoders\n");
    for (i = 0; i < res->count_connectors; i++) {
        connector = drmModeGetConnector(fd, res->connectors[i]);
        if (!connector)
            continue;

        asprintf(&name, "%s-%u",
            util_lookup_connector_type_name(connector->connector_type),
            connector->connector_type_id);

        printf("%d\t%d\t%s\t%-15s\t%dx%d\t\t%d\t",
               connector->connector_id,
               connector->encoder_id,
               util_lookup_connector_status_name(connector->connection),
               name,
               connector->mmWidth, connector->mmHeight,
               connector->count_modes);

        for (j = 0; j < connector->count_encoders; j++)
            printf("%s%d", j > 0 ? ", " : "", connector->encoders[j]);

        printf("\n");

        /*if (connector->count_modes) {
            printf("  modes:\n");
            printf("\tname refresh (Hz) hdisp hss hse htot vdisp vss vse vtot)\n");
            for (j = 0; j < connector->count_modes; j++)
                dump_mode(&connector->modes[j]);
        }*/

        /*if (_connector->props) {
            printf("  props:\n");
            for (j = 0; j < (int)_connector->props->count_props; j++)
                dump_prop(dev, _connector->props_info[j],
                      _connector->props->props[j],
                      _connector->props->prop_values[j]);
        }*/

        drmModeFreeConnector(connector);
        free(name);
    }
    printf("\n");
}

static void dump_encoders(drmModeRes *res, int fd)
{
    drmModeEncoder *encoder;
    int i;

    printf("Encoders (%d):\n", res->count_encoders);
    printf("id\tcrtc\ttype\tpossible crtcs\tpossible clones\t\n");
    for (i = 0; i < res->count_encoders; i++) {
        encoder = drmModeGetEncoder(fd, res->encoders[i]);
        if (!encoder)
            continue;

        printf("%d\t%d\t%s\t0x%08x\t0x%08x\n",
               encoder->encoder_id,
               encoder->crtc_id,
               util_lookup_encoder_type_name(encoder->encoder_type),
               encoder->possible_crtcs,
               encoder->possible_clones);

        drmModeFreeEncoder(encoder);
    }
    printf("\n");
}

static void dump_framebuffers(drmModeRes *res, int fd)
{
    drmModeFB *fb;
    int i;

    printf("Frame buffers (%d):\n", res->count_fbs);
    printf("id\tsize\tpitch\n");
    for (i = 0; i < res->count_fbs; i++) {
        fb = drmModeGetFB(fd, res->fbs[i]);
        if (!fb)
            continue;

        printf("%u\t(%ux%u)\t%u\n",
               fb->fb_id,
               fb->width, fb->height,
               fb->pitch);

        drmModeFreeFB(fb);
    }
    printf("\n");
}

static void dump_crtcs(drmModeRes *res, int fd)
{
    drmModeCrtc *crtc;
    int i;
    uint32_t j;

    printf("CRTCs (%d):\n", res->count_crtcs);
    printf("id\tfb\tpos\tsize\n");
    for (i = 0; i < res->count_crtcs; i++) {
        crtc = drmModeGetCrtc(fd, res->crtcs[i]);
        if (!crtc)
            continue;

        printf("%d\t%d\t(%d,%d)\t(%dx%d)\n",
               crtc->crtc_id,
               crtc->buffer_id,
               crtc->x, crtc->y,
               crtc->width, crtc->height);
        //dump_mode(&crtc->mode);

        /*if (_crtc->props) {
            printf("  props:\n");
            for (j = 0; j < _crtc->props->count_props; j++)
                dump_prop(dev, _crtc->props_info[j],
                      _crtc->props->props[j],
                      _crtc->props->prop_values[j]);
        } else {
            printf("  no properties found\n");
        }*/

        drmModeFreeCrtc(crtc);
    }
    printf("\n");
}

static void dump_fourcc(uint32_t fourcc)
{
    printf(" %c%c%c%c",
        fourcc,
        fourcc >> 8,
        fourcc >> 16,
        fourcc >> 24);
}

static void dump_planes(drmModeRes *res, int fd)
{
    drmModePlaneRes *plane_res;
    drmModePlane *ovr;
    unsigned int i, j;

    plane_res = drmModeGetPlaneResources(fd);
    if (!plane_res) {
        printf("Unable to get plane resources: %s\n", strerror(errno));
        return;
    }

    printf("Planes (%d):\n", plane_res->count_planes);
    printf("id\tcrtc\tfb\tCRTC x,y\tx,y\tgamma size\tpossible crtcs\n");

    for (i = 0; i < plane_res->count_planes; i++) {
        ovr = drmModeGetPlane(fd, plane_res->planes[i]);
        if (!ovr)
            continue;

        printf("%d\t%d\t%d\t%d,%d\t\t%d,%d\t%-8d\t0x%08x\n",
               ovr->plane_id, ovr->crtc_id, ovr->fb_id,
               ovr->crtc_x, ovr->crtc_y, ovr->x, ovr->y,
               ovr->gamma_size, ovr->possible_crtcs);

        if (!ovr->count_formats)
            continue;

        printf("  formats:");
        for (j = 0; j < ovr->count_formats; j++)
            dump_fourcc(ovr->formats[j]);
        printf("\n");

        /*if (plane->props) {
            printf("  props:\n");
            for (j = 0; j < plane->props->count_props; j++)
                dump_prop(dev, plane->props_info[j],
                      plane->props->props[j],
                      plane->props->prop_values[j]);
        } else {
            printf("  no properties found\n");
        }*/

        drmModeFreePlane(ovr);
    }
    printf("\n");

    drmModeFreePlaneResources(plane_res);
}


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

struct type_name {
    unsigned int type;
    const char *name;
};

static const char *util_lookup_type_name(unsigned int type,
    const struct type_name *table,
    unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++)
        if (table[i].type == type)
            return table[i].name;

    return NULL;
}

static const struct type_name connector_status_names[] = {
    { DRM_MODE_CONNECTED, "connected" },
    { DRM_MODE_DISCONNECTED, "disconnected" },
    { DRM_MODE_UNKNOWNCONNECTION, "unknown" },
};

const char *util_lookup_connector_status_name(unsigned int status)
{
    return util_lookup_type_name(status, connector_status_names,
        ARRAY_SIZE(connector_status_names));
}

static const struct type_name connector_type_names[] = {
    { DRM_MODE_CONNECTOR_Unknown, "unknown" },
    { DRM_MODE_CONNECTOR_VGA, "VGA" },
    { DRM_MODE_CONNECTOR_DVII, "DVI-I" },
    { DRM_MODE_CONNECTOR_DVID, "DVI-D" },
    { DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
    { DRM_MODE_CONNECTOR_Composite, "composite" },
    { DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
    { DRM_MODE_CONNECTOR_LVDS, "LVDS" },
    { DRM_MODE_CONNECTOR_Component, "component" },
    { DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
    { DRM_MODE_CONNECTOR_DisplayPort, "DP" },
    { DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
    { DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
    { DRM_MODE_CONNECTOR_TV, "TV" },
    { DRM_MODE_CONNECTOR_eDP, "eDP" },
    { DRM_MODE_CONNECTOR_VIRTUAL, "Virtual" },
    { DRM_MODE_CONNECTOR_DSI, "DSI" },
    { DRM_MODE_CONNECTOR_DPI, "DPI" },
};

const char *util_lookup_connector_type_name(unsigned int type)
{
    return util_lookup_type_name(type, connector_type_names,
        ARRAY_SIZE(connector_type_names));
}

static const struct type_name encoder_type_names[] = {
    { DRM_MODE_ENCODER_NONE, "none" },
    { DRM_MODE_ENCODER_DAC, "DAC" },
    { DRM_MODE_ENCODER_TMDS, "TMDS" },
    { DRM_MODE_ENCODER_LVDS, "LVDS" },
    { DRM_MODE_ENCODER_TVDAC, "TVDAC" },
    { DRM_MODE_ENCODER_VIRTUAL, "Virtual" },
    { DRM_MODE_ENCODER_DSI, "DSI" },
    { DRM_MODE_ENCODER_DPMST, "DPMST" },
    { DRM_MODE_ENCODER_DPI, "DPI" },
};

const char *util_lookup_encoder_type_name(unsigned int type)
{
    return util_lookup_type_name(type, encoder_type_names,
                     ARRAY_SIZE(encoder_type_names));
}