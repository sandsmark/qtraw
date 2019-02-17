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
    void canRead();
    void loadRaw_data();
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

void QtRawTest::canRead()
{
    QImageReader reader(DATA_DIR "/testimage.arw");
    QVERIFY(reader.canRead());
}

void QtRawTest::loadRaw_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QSize>("expectedSize");

    QTest::newRow("sony") <<
        DATA_DIR "/testimage.arw" <<
        QSize(4256, 2856);

    QTest::newRow("nikon") <<
        DATA_DIR "/RAW_NIKON_D70.NEF" <<
        QSize(2014, 3039);
}

void QtRawTest::loadRaw()
{
    QFETCH(QString, fileName);
    QFETCH(QSize, expectedSize);

    QImage raw(fileName);
    QCOMPARE(raw.size(), expectedSize);
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
