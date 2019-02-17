/*
 * Copyright (C) 2012 Alberto Mardegan <info@mardy.it>
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

#include <QDebug>
#include <QImage>
#include <QImageReader>
#include <QTest>

class QtRawTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void corruptedFile();
    void loadRaw();
    void loadRawWithReader();
};

void QtRawTest::initTestCase()
{
}

void QtRawTest::cleanupTestCase()
{
}

void QtRawTest::corruptedFile()
{
    QImageReader reader(DATA_DIR "/corrupted.arw");
    QImage raw = reader.read();
    QCOMPARE(reader.error(), QImageReader::InvalidDataError);
}

void QtRawTest::loadRaw()
{
    QImage raw(DATA_DIR "/testimage.arw");
    QCOMPARE(raw.size(), QSize(4256, 2856));
}

void QtRawTest::loadRawWithReader()
{
    QImageReader reader(DATA_DIR "/testimage.arw");
    QCOMPARE(reader.size(), QSize(4256, 2856));

    reader.setScaledSize(QSize(800,600));
    QImage raw = reader.read();
    QCOMPARE(raw.size(), QSize(800, 600));
}

QTEST_MAIN(QtRawTest)

#include "qtraw-test.moc"
