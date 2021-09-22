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
#include <QBuffer>
#include <QImage>
#include <QVariant>
#include <QImageReader>
#include <QElapsedTimer>
#include <QFileDevice>

#include <libraw.h>

// Same as the jpeg plugin
#define HIGH_QUALITY_THRESHOLD 50
#define DEFAULT_QUALITY 75

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

    int quality = DEFAULT_QUALITY;
};

static void printError(const char *what, int error, bool warning = true)
{
    if (warning) {
        if (error < 0) {
            qCWarning(QTRAW_IO) << what << libraw_strerror(error);
        } else if (error > 0) {
            qCWarning(QTRAW_IO) << what << strerror(error);
        }
    } else {
        if (error < 0) {
            qCDebug(QTRAW_IO) << what << libraw_strerror(error);
        } else if (error > 0) {
            qCDebug(QTRAW_IO) << what << strerror(error);
        }
    }
}

RawIOHandlerPrivate::~RawIOHandlerPrivate()
{
    delete raw;
    raw = 0;
    delete stream;
    stream = 0;
}

bool RawIOHandlerPrivate::load(QIODevice *device)
{
    qCDebug(QTRAW_IO) << "Asked if we can open a file";
    if (device == nullptr) return false;
    if (device->isSequential()) {
        qCDebug(QTRAW_IO) << "Can't read sequential";
        return false;
    }

    if (raw != nullptr) return true;

    qint64 pos = device->pos();
    device->seek(0);


    stream = new Datastream(device);
    raw = new LibRaw;

    // Tiny optimization, but w/e
    //int ret = 0;
    //QFileDevice *file = qobject_cast<QFileDevice*>(device);
    //if (file) {
    //    ret = raw->open_file(file->fileName().toLocal8Bit().constData());
    //} else {
    //    ret = raw->open_datastream(stream);
    //}

    int ret = raw->open_datastream(stream);
    if (ret != LIBRAW_SUCCESS) {
        printError("Opening file failed", ret, false);
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
    qCDebug(QTRAW_IO) << "We can open" << device;
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
    if (!d->load(device())) {
        return false;
    }

    const QSize finalSize = d->scaledSize.isValid() ?
        d->scaledSize : d->defaultSize;


    const libraw_data_t &imgdata = d->raw->imgdata;
    QSize thumbnailSize(imgdata.thumbnail.twidth, imgdata.thumbnail.theight);
    if (d->raw->imgdata.sizes.flip == 5 || d->raw->imgdata.sizes.flip == 6) {
        thumbnailSize.transpose();
    }
    qCDebug(QTRAW_IO) << "Preview size:" << thumbnailSize;
    qCDebug(QTRAW_IO) << "Full size:" << finalSize;

    const bool useThumbnail =
        (// Less than 1% difference
         d->quality <= DEFAULT_QUALITY &&
         finalSize.width() / 100 == thumbnailSize.width() / 100 &&
         finalSize.height() / 100 == thumbnailSize.height() / 100
        )
        ||
        (
            finalSize.width() <= thumbnailSize.width() &&
            finalSize.height() <= thumbnailSize.height()
        );


    libraw_processed_image_t *output = nullptr;
    if (useThumbnail) {
        qCDebug(QTRAW_IO) << "Using thumbnail";
        int ret = d->raw->unpack_thumb();
        if (ret != LIBRAW_SUCCESS) {
            printError("Unpacking preview failed", ret);
            return false;
        }
        output = d->raw->dcraw_make_mem_thumb();
    } else {
        qCDebug(QTRAW_IO) << "Using full image";
        int ret = d->raw->unpack();
        d->raw->dcraw_process();
        if (ret != LIBRAW_SUCCESS) {
            printError("Processing full image failed", ret);
            return false;
        }
        output = d->raw->dcraw_make_mem_image();
    }

    QImage unscaled;
    uchar *pixels = nullptr;
    if (output->type == LIBRAW_IMAGE_JPEG) {
        // fromRawData does not copy data
        QByteArray bufferData =
            QByteArray::fromRawData(reinterpret_cast<char*>(output->data), output->data_size);

        QBuffer buffer(&bufferData);
        // We use QImageReader so we can set quality, qjpeg reads faster when lower quality is set
        QImageReader jpegReader(&buffer);
        jpegReader.setAutoDetectImageFormat(false);
        jpegReader.setFormat("JPG");
        jpegReader.setQuality(d->quality);
        jpegReader.setScaledSize(finalSize);
        if (!jpegReader.read(&unscaled)) {
            return false;
        }

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
        Qt::TransformationMode mode = Qt::FastTransformation;

        // If we've loaded a thumbnail, do a smooth transformation
        if (d->quality >= HIGH_QUALITY_THRESHOLD || useThumbnail) {
            mode = Qt::SmoothTransformation;
        }
        *image = unscaled.scaled(finalSize, Qt::IgnoreAspectRatio, mode);
    } else {
        if (output->type == LIBRAW_IMAGE_BITMAP) {
            // make sure that the bits are copied
            *image = unscaled.copy();
        } else {
            *image = unscaled;
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
        if (d->quality == -1) {
            d->quality = DEFAULT_QUALITY;
        }
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
