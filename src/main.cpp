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

#include <QByteArray>
#include <QImageIOHandler>
#include <QIODevice>
#include <QStringList>

#include "raw-io-handler.h"

class RawPlugin: public QImageIOPlugin
{
    Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "raw.json")
#endif

public:
    Capabilities capabilities(QIODevice *device,
                              const QByteArray &format) const override;
    QImageIOHandler *create(QIODevice *device,
                            const QByteArray &format = QByteArray()) const override;
};

QImageIOPlugin::Capabilities
RawPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    static const QSet<QString> keys = {
        "arw",
        "crw", "cr2",
        "dng",
        "nef",
        "raf",
        "kderaw",
        "dcr",
        "k25",
        "kdc",
        "mrw",
        "nrw",
        "orf",
        "raw",
        "raw2",
        "rw",
        "rw2",
        "pef",
        "srw",
        "x3f",
        "sr2",
        "srf"
    };
    if (keys.contains(format) ||
        format == "tif" ||
        format == "tiff") {
        return Capabilities(CanRead);
    }
    if (!format.isEmpty()) {
        return Capabilities();
    }

    Capabilities cap;
    if (device && device->isReadable() && RawIOHandler::canRead(device)) {
        cap |= CanRead;
    }
    return cap;
}

QImageIOHandler *RawPlugin::create(QIODevice *device,
                                   const QByteArray &format) const
{
    RawIOHandler *handler = new RawIOHandler();
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

#include "main.moc"
