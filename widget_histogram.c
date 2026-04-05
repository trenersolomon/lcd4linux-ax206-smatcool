/* Histogram widget for lcd4linux
 * Shows scrolling history graph
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_GD_GD_H
#include <gd/gd.h>
#else
#ifdef HAVE_GD_H
#include <gd.h>
#else
#error "gd.h not found!"
#endif
#endif

#include "debug.h"
#include "cfg.h"
#include "property.h"
#include "timer_group.h"
#include "layout.h"
#include "widget.h"
#include "widget_histogram.h"
#include "drv_generic.h"
#include "drv_generic_graphic.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

static int getcolorint(PROPERTY * colorprop, void *Image)
{
    char *colorstr;
    int colorint;
    char *e;
    unsigned long l;
    unsigned char r, g, b, a;
    colorstr = P2S(colorprop);
    if (strlen(colorstr) == 8) {
        l = strtoul(colorstr, &e, 16);
        r = (l >> 24) & 0xff;
        g = (l >> 16) & 0xff;
        b = (l >> 8) & 0xff;
        a = (l & 0xff) / 2;
        colorint = gdImageColorAllocateAlpha(Image, r, g, b, a);
    } else {
        l = strtoul(colorstr, &e, 16);
        r = (l >> 16) & 0xff;
        g = (l >> 8) & 0xff;
        b = l & 0xff;
        colorint = gdImageColorAllocate(Image, r, g, b);
    }
    return colorint;
}

static void widget_histogram_render(const char *Name, WIDGET_HISTOGRAM * H)
{
    int x, y;
    int colorbar, colorbg;
    gdImagePtr img;
    int W = H->width;
    int HH = H->height;
    double dmin, dmax;

    (void)Name;

    if (H->gdImage) {
        gdImageDestroy(H->gdImage);
        H->gdImage = NULL;
    }

    H->gdImage = gdImageCreateTrueColor(W, HH);
    gdImageSaveAlpha(H->gdImage, 1);
    if (!H->gdImage)
        return;

    img = H->gdImage;

    colorbg  = getcolorint(&H->background, img);
    colorbar = getcolorint(&H->color, img);

    /* TEST: fill with explicit blue */
    int testblue = gdImageColorAllocate(img, 0, 0x11, 0x33);
    gdImageFill(img, 0, 0, testblue);

    if (property_valid(&H->expr_min))
        dmin = P2N(&H->expr_min);
    else
        dmin = H->min;

    if (property_valid(&H->expr_max))
        dmax = P2N(&H->expr_max);
    else
        dmax = H->max;

    if (dmax <= dmin) dmax = dmin + 1;

    int n = H->count;
    if (n == 0)
        goto done;
    if (n > W) n = W;

    for (int i = 0; i < n; i++) {
        int idx = (H->head - n + i + HISTOGRAM_MAX_SAMPLES) % HISTOGRAM_MAX_SAMPLES;
        double val = H->samples[idx];
        if (val < dmin) val = dmin;
        if (val > dmax) val = dmax;
        int bar_h = (int)((val - dmin) / (dmax - dmin) * (HH - 1));
        x = W - n + i;
        for (y = HH - 1; y >= HH - 1 - bar_h; y--) {
            gdImageSetPixel(img, x, y, colorbar);
        }
    }

    if (H->bitmap)
        free(H->bitmap);
    H->bitmap = malloc(W * HH * sizeof(RGBA));
    if (!H->bitmap)
        return;

    for (y = 0; y < HH; y++) {
        for (x = 0; x < W; x++) {
            int p = gdImageGetTrueColorPixel(img, x, y);
            int a = gdTrueColorGetAlpha(p);
            int i = y * W + x;
            H->bitmap[i].R = gdTrueColorGetRed(p);
            H->bitmap[i].G = gdTrueColorGetGreen(p);
            H->bitmap[i].B = gdTrueColorGetBlue(p);
            H->bitmap[i].A = (a == 127) ? 0 : 255 - 2 * a;
        }
    }

    done:
    return;
}

static void widget_histogram_update(void *Self)
{
    WIDGET *W = (WIDGET *) Self;
    WIDGET_HISTOGRAM *H = W->data;

    if (W->parent == NULL) {
        property_eval(&H->value);
        property_eval(&H->update);
        property_eval(&H->reload);
        property_eval(&H->visible);

        double val = P2N(&H->value);
        H->samples[H->head] = val;
        H->head = (H->head + 1) % HISTOGRAM_MAX_SAMPLES;
        if (H->count < HISTOGRAM_MAX_SAMPLES)
            H->count++;

        widget_histogram_render(W->name, H);
    }

    if (W->class->draw)
        W->class->draw(W);

    if (P2N(&H->update) > 0) {
        timer_add_widget(widget_histogram_update, Self, P2N(&H->update), 1);
    }
}

int widget_histogram_init(WIDGET * Self)
{
    char *section;
    WIDGET_HISTOGRAM *H;

    if (Self->parent == NULL) {
        section = malloc(strlen(Self->name) + 8);
        strcpy(section, "Widget:");
        strcat(section, Self->name);

        H = malloc(sizeof(WIDGET_HISTOGRAM));
        memset(H, 0, sizeof(WIDGET_HISTOGRAM));

        H->bitmap = NULL;
        H->min = 0;
        H->max = 100;
        H->head = 0;
        H->count = 0;

        property_load(section, "expression", NULL,     &H->value);
        property_load(section, "width",      NULL,     &H->prop_width);
        property_load(section, "length",     NULL,     &H->prop_length);
        property_load(section, "update",     "500",    &H->update);
        property_load(section, "reload",     "1",      &H->reload);
        property_load(section, "visible",    "1",      &H->visible);
        property_load(section, "inverted",   "0",      &H->inverted);
        property_load(section, "center",     "0",      &H->center);
        property_load(section, "color",      "00aaff", &H->color);
        property_load(section, "background", "001133", &H->background);
        property_load(section, "min",        NULL,     &H->expr_min);
        property_load(section, "max",        NULL,     &H->expr_max);

        if (!property_valid(&H->value)) {
            error("Warning: widget %s has no expression", section);
        }
        if (!property_valid(&H->prop_length)) {
            error("Warning: widget %s has no length", section);
            return 1;
        }
        if (!property_valid(&H->prop_width)) {
            error("Warning: widget %s has no width", section);
            return 1;
        }

        property_eval(&H->prop_width);
        property_eval(&H->prop_length);
        property_eval(&H->color);
        property_eval(&H->background);
        property_eval(&H->update);
        property_eval(&H->reload);
        property_eval(&H->visible);
        property_eval(&H->value);

        property_eval(&H->expr_min);
        property_eval(&H->expr_max);

        H->width  = P2N(&H->prop_length);
        H->height = P2N(&H->prop_width);
        fprintf(stderr, "init: width=%d height=%d\n", H->width, H->height);

        free(section);
        Self->data = H;
        Self->x2 = Self->col + H->width - 1;
        Self->y2 = Self->row + H->height - 1;

    } else {
        Self->data = Self->parent->data;
    }

    widget_histogram_update(Self);
    return 0;
}

int widget_histogram_quit(WIDGET * Self)
{
    if (Self) {
        WIDGET_HISTOGRAM *H = Self->data;
        if (H) {
            if (H->gdImage) {
                gdImageDestroy(H->gdImage);
                H->gdImage = NULL;
            }
            if (H->bitmap) {
                free(H->bitmap);
                H->bitmap = NULL;
            }
            free(H);
        }
        Self->data = NULL;
    }
    return 0;
}


WIDGET_CLASS Widget_Histogram = {
    .name  = "Histogram",
    .type  = WIDGET_TYPE_XY,
    .init  = widget_histogram_init,
    .draw  = NULL,
    .quit  = widget_histogram_quit,
};
