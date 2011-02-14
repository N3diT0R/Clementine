/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "plsparser.h"

#include <QTextStream>
#include <QtDebug>

PLSParser::PLSParser(LibraryBackendInterface* library, QObject* parent)
  : ParserBase(library, parent)
{
}

SongList PLSParser::Load(QIODevice *device, const QString& playlist_path, const QDir &dir) const {
  QMap<int, Song> songs;
  QRegExp n_re("\\d+$");

  while (!device->atEnd()) {
    QString line = QString::fromUtf8(device->readLine()).trimmed();
    int equals = line.indexOf('=');
    QString key = line.left(equals).toLower();
    QString value = line.mid(equals + 1);

    n_re.indexIn(key);
    int n = n_re.cap(0).toInt();

    if (key.startsWith("file")) {
      if (!ParseTrackLocation(value, dir, &songs[n]))
        qWarning() << "Failed to parse location: " << value;

      // Load the song from the library if it's there.
      Song library_song = LoadLibrarySong(songs[n].filename());
      if (library_song.is_valid()) {
        songs[n] = library_song;
      } else {
        songs[n].InitFromFile(songs[n].filename(), -1);
      }
    } else if (key.startsWith("title")) {
      songs[n].set_title(value);
    } else if (key.startsWith("length")) {
      qint64 seconds = value.toLongLong();
      if (seconds > 0) {
        songs[n].set_length_nanosec(seconds * kNsecPerSec);
      }
    }
  }

  return songs.values();
}

void PLSParser::Save(const SongList &songs, QIODevice *device, const QDir &dir) const {
  QTextStream s(device);
  s << "[playlist]" << endl;
  s << "Version=2" << endl;
  s << "NumberOfEntries=" << songs.count() << endl;

  int n = 1;
  foreach (const Song& song, songs) {
    s << "File" << n << "=" << MakeRelativeTo(song.filename(), dir) << endl;
    s << "Title" << n << "=" << song.title() << endl;
    s << "Length" << n << "=" << song.length_nanosec() / kNsecPerSec << endl;
    ++n;
  }
}

bool PLSParser::TryMagic(const QByteArray &data) const {
  return data.contains("[playlist]");
}
