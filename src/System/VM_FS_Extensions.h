#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

Strings System::VM_FileSystem::mediaFileExtensions =
{
	"A64", "264", "265", "302", "3G2", "3GP", "722", "A64", "AA", "AA3", "AAC", "AC3", "ACM", "ADF", "ADP", "ADS", "ADTS",
	"ADX", "AEA", "AFC", "AIF", "AIFC", "AIFF", "AL", "AMR", "ANS", "APE", "APL", "APNG", "AQT", "ART", "ASC", "ASF", "ASS",
	"AST", "AU", "AVC", "AVI", "AVR", "BCSTM", "BFSTM", "BIN", "BIT", "BMP", "BMV", "BRSTM", "CAF", "CAVS", "CDATA", "CDG",
	"CDXL", "CGI", "CHK", "CIF", "DAUD", "DIF", "DIZ", "DNXHD", "DPX", "DRC", "DSS", "DTK", "DTS", "DTSHD", "DV", "DVD",
	"EAC3", "F4V", "FAP", "FFM", "FFMETA", "FLAC", "FLM", "FLV", "FSB", "G722", "G723_1", "G729", "GENH", "GIF", "GSM", "GXF",
	"H261", "H263", "H264", "H265", "H26L", "HEVC", "ICE", "ICO", "IDF", "IDX", "IM1", "IM24", "IM8", "IRCAM", "ISMA", "ISMV",
	"IVF", "IVR", "J2C", "J2K", "JLS", "JP2", "JPEG", "JPG", "JS", "JSS", "LATM", "LBC", "LJPG", "LOAS", "LRC", "LVF", "M1V",
	"M2A", "M2T", "M2TS", "M2V", "M3U8", "M4A", "M4V", "MAC", "MJ2", "MJPEG", "MJPG", "MK3D", "MKA", "MKS", "MKV", "MLP", "MMF",
	"MOV", "MP2", "MP3", "MP4", "MPA", "MPC", "MPEG", "MPG", "MPL2", "MPO", "MSF", "MTS", "MVI", "MXF", "MXG", "NIST",
	"NUT", "OGA", "OGG", "OGM", "OGV", "OMA", "OMG", "OPUS", "PAF", "PAM", "PBM", "PCX", "PGM", "PGMYUV", "PIX", "PJS", "PNG", "PPM",
	"PSP", "PVF", "QCIF", "RA", "RAS", "RCO", "RCV", "RGB", "RM", "ROQ", "RS", "RSD", "RSO", "RT", "SAMI", "SB", "SBG", "SDR2",
	"SF", "SGI", "SHN", "SLN", "SMI", "SON", "SOX", "SPDIF", "SPH", "SPX", "SRT", "SS2", "SSA", "STL", "STR", "SUB", "SUN",
	"SUNRAS", "SUP", "SVAG", "SW", "SWF", "TAK", "TCO", "TGA", "THD", "TIF", "TIFF", "TS", "TTA", "UB", "UL", "UW", "V",
	"V210", "VAG", "VB", "VC1", "VC2", "VIV", "VOB", "VOC", "VPK", "VQE", "VQF", "VQL", "VT", "VTT", "W64", "WAV", "WEBM", "WEBP",
	"WMA", "WMV", "WTV", "WV", "XBM", "XFACE", "XL", "XMV", "XVAG", "XWD", "Y", "Y4M", "YOP", "YUV", "YUV10"
};

Strings System::VM_FileSystem::subFileExtensions =
{
	"ASS", "IDX", "JS", "JSS", "MPL2", "PJS", "RT",
	"SAMI", "SMI", "SRT", "SSA", "STL", "SUB", "TXT", "VTT"
};

Strings System::VM_FileSystem::systemFileExtensions =
{
	"__INCOMPLETE___", "!UT", "ANTIFRAG", "BC", "BC!", "DCTMP", "FB!", "OB!", "7Z",
	"A", "ACE", "APK", "BAT", "BIN", "BZ2", "C", "CAB", "CFG", "CHM", "CMD", "CONFIG",
	"CPP", "CS", "CSS", "DJVU", "DLL", "DMG", "DOC", "DOCX", "DLL", "DYLIB", "EFI", "EPUB",
	"EXE", "GZ", "H", "HTM", "HTML", "ISO", "JAR", "JAVA", "JS", "MSI", "LA", "LOG",
	"MDB", "O", "ODT", "PDF", "PHP", "PL", "PPT", "PPTX", "PS1", "PY", "RAR", "RTF",
	"SH", "SO", "SYS", "TAR", "TGZ", "TORRENT", "TXT", "XLS", "XLSX", "XML", "XZ", "ZIP"
};
