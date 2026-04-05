#ifndef _WIDGET_HISTOGRAM_H_
#define _WIDGET_HISTOGRAM_H_

#include "property.h"
#include "widget.h"
#include "rgb.h"

#define HISTOGRAM_MAX_SAMPLES 320

typedef struct WIDGET_HISTOGRAM {
    /* gdImage to visible must be in this order to match WIDGET_IMAGE */
    void *gdImage;
    RGBA *bitmap;
    int width, height;        /* must match WIDGET_IMAGE */
    int oldheight;
     PROPERTY dummy;     /* padding to match WIDGET_IMAGE layout */
    /* custom fields */
    PROPERTY value;
    PROPERTY prop_width;      /* bar width config */
    PROPERTY prop_length;     /* bar length config */
    PROPERTY update;
    PROPERTY reload;
    PROPERTY visible;
    PROPERTY inverted;
    PROPERTY center;
    PROPERTY color;
    PROPERTY background;
    PROPERTY expr_min;
    PROPERTY expr_max;
    double min;
    double max;
    /* history ring buffer */
    double samples[HISTOGRAM_MAX_SAMPLES];
    int    head;
    int    count;
} WIDGET_HISTOGRAM;

extern WIDGET_CLASS Widget_Histogram;

#endif

