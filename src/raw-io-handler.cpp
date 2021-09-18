/*
 * Copyright (C) 2012-2020 Alberto Mardegan <info@mardy.it>
 *
 * This file is part of QtRaw.
 *
 * QtRaw is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QtRaw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QtRaw.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "datastream.h"
#include "raw-io-handler.h"

#include <QDebug>
#include <QImage>
#include <QVariant>

#include <libraw.h>

// Same as the jpeg plugin
#define HIGH_QUALITY_THRESHOLD 50

Q_LOGGING_CATEGORY(QTRAW_IO, "qtraw.io", QtWarningMsg)

class RawIOHandlerPrivate
{
public:
    RawIOHandlerPrivate(RawIOHandler *qq):
        raw(0),
        stream(0),
        q(qq)
    {}

    ~RawIOHandlerPrivate();

    bool load(QIODevice *device);

    LibRaw *raw;
    Datastream *stream;
    QSize            defaultSize;
    QSize            scaledSize;
    mutable RawIOHandler *q;

    int quality = 75;
};

RawIOHandlerPrivate::~RawIOHandlerPrivate()
{
    delete raw;
    raw = 0;
    delete stream;
    stream = 0;
}

bool RawIOHandlerPrivate::load(QIODevice *device)
{
    if (device == nullptr) return false;
    if (device->isSequential()) return false;

    qint64 pos = device->pos();

    device->seek(0);
    if (raw != nullptr) return true;

    stream = new Datastream(device);
    raw = new LibRaw;
    if (raw->open_datastream(stream) != LIBRAW_SUCCESS) {
        device->seek(pos);
        delete raw;
        raw = 0;
        delete stream;
        stream = 0;
        return false;
    }

    defaultSize = QSize(raw->imgdata.sizes.width,
                        raw->imgdata.sizes.height);
    if (raw->imgdata.sizes.flip == 5 || raw->imgdata.sizes.flip == 6) {
        defaultSize.transpose();
    }
    return true;
}


RawIOHandler::RawIOHandler():
    d(new RawIOHandlerPrivate(this))
{
}


RawIOHandler::~RawIOHandler()
{
    delete d;
}


bool RawIOHandler::canRead() const
{
    if (canRead(device())) {
        setFormat("raw");
        return true;
    }
    return false;
}


bool RawIOHandler::canRead(QIODevice *device)
{
    if (!device) {
        return false;
    }
    RawIOHandler handler;
    return handler.d->load(device);
}


bool RawIOHandler::read(QImage *image)
{
    if (!d->load(device())) return false;

    QSize finalSize = d->scaledSize.isValid() ?
        d->scaledSize : d->defaultSize;

    const libraw_data_t &imgdata = d->raw->imgdata;
    libraw_processed_image_t *output;
    if (finalSize.width() < imgdata.thumbnail.twidth ||
        finalSize.height() < imgdata.thumbnail.theight) {
        qCDebug(QTRAW_IO) << "Using thumbnail";
        d->raw->unpack_thumb();
        output = d->raw->dcraw_make_mem_thumb();
    } else {
        qCDebug(QTRAW_IO) << "Decoding raw data";
        d->raw->unpack();
        d->raw->dcraw_process();
        output = d->raw->dcraw_make_mem_image();
    }

    QImage unscaled;
    uchar *pixels = 0;
    if (output->type == LIBRAW_IMAGE_JPEG) {
        unscaled.loadFromData(output->data, output->data_size, "JPEG");
        if (imgdata.sizes.flip != 0) {
            QTransform rotation;
            int angle = 0;
            if (imgdata.sizes.flip == 3) angle = 180;
            else if (imgdata.sizes.flip == 5) angle = -90;
            else if (imgdata.sizes.flip == 6) angle = 90;
            if (angle != 0) {
                rotation.rotate(angle);
                unscaled = unscaled.transformed(rotation);
            }
        }
    } else {
        int numPixels = output->width * output->height;
        int colorSize = output->bits / 8;
        int pixelSize = output->colors * colorSize;
        pixels = new uchar[numPixels * 4];
        uchar *data = output->data;
        for (int i = 0; i < numPixels; i++, data += pixelSize) {
            if (output->colors == 3) {
                pixels[i * 4] = data[2 * colorSize];
                pixels[i * 4 + 1] = data[1 * colorSize];
                pixels[i * 4 + 2] = data[0];
            } else {
                pixels[i * 4] = data[0];
                pixels[i * 4 + 1] = data[0];
                pixels[i * 4 + 2] = data[0];
            }
        }
        unscaled = QImage(pixels,
                          output->width, output->height,
                          QImage::Format_RGB32);
    }

    if (unscaled.size() != finalSize) {
        *image = unscaled.scaled(finalSize, Qt::IgnoreAspectRatio,
                d->quality >= HIGH_QUALITY_THRESHOLD ? Qt::SmoothTransformation : Qt::FastTransformation);
    } else {
        *image = unscaled;
        if (output->type == LIBRAW_IMAGE_BITMAP) {
            // make sure that the bits are copied
            uchar *b = image->bits();
            Q_UNUSED(b);
        }
    }
    d->raw->dcraw_clear_mem(output);
    delete pixels;

    return true;
}


QVariant RawIOHandler::option(ImageOption option) const
{
    switch(option) {
    case ImageFormat:
        return QImage::Format_RGB32;
    case Size:
        d->load(device());
        return d->defaultSize;
    case ScaledSize:
        return d->scaledSize;
    case Quality:
        return d->quality;
    default:
        break;
    }
    return QVariant();
}


void RawIOHandler::setOption(ImageOption option, const QVariant & value)
{
    switch(option) {
    case ScaledSize:
        d->scaledSize = value.toSize();
        break;
    case Quality:
        d->quality = value.toInt();
        break;
    default:
        break;
    }
}


bool RawIOHandler::supportsOption(ImageOption option) const
{
    switch (option)
    {
    case ImageFormat:
    case Size:
    case ScaledSize:
    case Quality:
        return true;
    default:
        break;
    }
    return false;
}
