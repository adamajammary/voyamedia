## Voya Media
### Media player that easily plays all your music, pictures and videos
#### Copyright (C) 2012 Adam A. Jammary (Jammary Consulting)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

![Voya Media screenshot](https://sourceforge.net/p/voyamedia/screenshot/VoyaMedia-8-1920.png)

Voya Media is a cross-platform media player that easily plays all your music, pictures and videos.

- Privacy (no spyware, ads or user tracking)
- Music (AAC, AC3, DTS, FLAC, MP3, TrueHD, Vorbis, WMA)
- Pictures (BMP, DDS, GIF, ICO, JPEG, PNG, PSD, TIFF, WebP)
- Videos (H.264, H.265, DivX, MPEG, Theora, WMV, XviD)
- Subtitles (MicroDVD, PGS, SSA/ASS, SubRip, VobSub)
- Music Metadata (APE, ASF/WMA, ID3, M4A, Vorbis)
- Picture Metadata (Exif)
- Movie and TV show details (TMDb API)
- Streaming internet radio stations (SHOUTcast API)
- YouTube videos (*1) (YouTube API)
- Add files from Dropbox, UPnP devices or iCloud Photo Library (iOS)
- Share files with other UPnP devices (*2)
- No support for DRM-encrypted media (*3)

1. (*) Many videos are restricted from being played outside of youtube.com (mostly music videos)
2. (*) Cannot share/host files on iOS devices because of limited local/persistent storage
3. (*) For example: WMDRM (WMA), CSS (DVD), AACS (Blu-ray) etc.

## Supported Platforms

Platform | Minimum Version | CPU Architecture
-------- | --------------- | ----------------
[Android](https://play.google.com/store/apps/details?id=com.voyamedia.android) | 8.0 (Oreo) | ARMv8/ARM64
[iOS](https://itunes.apple.com/us/app/voya-media/id1009917954) | 10.0 | ARMv8/ARM64
[Linux](https://sourceforge.net/projects/voyamedia/files/VoyaMedia/3.x/Linux/) | Fedora 20, Ubuntu 14.04 | x86_64
[Mac OS X](https://itunes.apple.com/us/app/voya-media/id1009333985) | 10.11 (El Capitan) | x86_64
[Windows](https://www.microsoft.com/store/apps/9NBLGGH52684) | 10 | x86_64
[Windows](https://sourceforge.net/projects/voyamedia/files/VoyaMedia/3.x/Windows/) | 7 | x86_64

## 3rd Party Libraries

Library | Version | License
------- | ------- | -------
[FFmpeg](https://ffmpeg.org/) | [4.1.3](https://www.ffmpeg.org/releases/ffmpeg-4.1.3.tar.gz) | [LGPL v.2.1 (GNU Lesser General Public License)](https://ffmpeg.org/legal.html)
[FreeImage](http://freeimage.sourceforge.net/download.html) | [3.18.0](http://downloads.sourceforge.net/freeimage/FreeImage3180.zip) | [FIPL (FreeImage Public License)](http://freeimage.sourceforge.net/license.html)
[Freetype2](https://www.freetype.org/) | [2.4.12](https://sourceforge.net/projects/freetype/files/freetype2/2.10.0/freetype-2.10.0.tar.bz2) | [FTL (FreeType License)](https://www.freetype.org/license.html)
[libCURL](https://curl.haxx.se/libcurl/) | [7.64.1](https://curl.haxx.se/download/curl-7.64.1.tar.gz) | [MIT/X derivate license](https://curl.haxx.se/docs/copyright.html)
[libUPNP](http://pupnp.sourceforge.net/) | [1.8.4](https://sourceforge.net/projects/pupnp/files/pupnp/libupnp-1.8.4/libupnp-1.8.4.tar.bz2/download) | [BSD (Berkeley Standard Distribution)](http://pupnp.sourceforge.net/#license)
[libXML2](http://xmlsoft.org/) | [2.9.9](http://xmlsoft.org/sources/libxml2-2.9.9.tar.gz) | [MIT License](https://opensource.org/licenses/mit-license.html)
[M's JSON parser](https://sourceforge.net/projects/mjson/) | [1.7.0](https://sourceforge.net/projects/mjson/files/mjson/mjson-1.7.0.tar.gz/download) | [LGPL v.2.1 (GNU Lesser General Public License)](https://sourceforge.net/projects/mjson/)
[OpenSSL](https://www.openssl.org/) | [1.1.1b](https://www.openssl.org/source/openssl-1.1.1b.tar.gz) | [Dual (OpenSSL / Original SSLeay License)](https://www.openssl.org/source/license.html)
[SDL2](https://www.libsdl.org/) | [2.0.9](https://www.libsdl.org/release/SDL2-2.0.9.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[SDL2_ttf](https://www.libsdl.org/projects/SDL_ttf/) | [2.0.15](https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[SQLite](https://www.sqlite.org/) | [3.28.0](https://www.sqlite.org/2019/sqlite-autoconf-3280000.tar.gz) | [Public Domain](https://www.sqlite.org/copyright.html)
[zLib](http://www.zlib.net/) | [1.2.11](https://downloads.sourceforge.net/project/libpng/zlib/1.2.11/zlib-1.2.11.tar.gz) | [zlib license](http://www.zlib.net/zlib_license.html)

## Fonts

Font | License
---- | -------
[Google Noto](https://www.google.com/get/noto/) | [OFL](http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL)

## APIs

API | License
--- | -------
[Dropbox](https://www.dropbox.com/developers) | [Developer branding guide](https://www.dropbox.com/developers/reference/branding-guide)
[Google Maps](https://developers.google.com/maps/documentation/geocoding/start) | [Terms of Service](https://developers.google.com/maps/terms)
[SHOUTcast Radio Directory API](http://wiki.shoutcast.com/wiki/SHOUTcast_Radio_Directory_API) | [License Agreement](http://wiki.shoutcast.com/wiki/SHOUTcast_API_License_Agreement)
[TMDb](https://www.themoviedb.org/documentation/api) | [Terms of Use](https://www.themoviedb.org/terms-of-use)
[YouTube Data API](https://developers.google.com/youtube/v3/) | [Terms of Service](https://developers.google.com/youtube/terms/api-services-terms-of-service)
