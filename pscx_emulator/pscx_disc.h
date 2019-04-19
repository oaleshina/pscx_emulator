#pragma once

#include <fstream>

#include "pscx_minutesecondframe.h"

// Size of a CD sector in bytes.
const size_t SECTOR_SIZE = 2352;

// CD-ROM sector sync pattern: 10 0xff surrounded by two 0x00. Not
// used in CD-DA audio tracks.
const uint8_t SECTOR_SYNC_PATTERN[] = {
	0x00,
	0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff,
	0x00
};

// Disc region coding.
enum Region
{
	// Japan (NTSC): SCEI.
	REGION_JAPAN,
	// North Ametica (NTSC): SCEA.
	REGION_NORTH_AMERICA,
	// Europe (PAL): SCEE.
	REGION_EUROPE
};

// Structure representing a single CD-ROM XA sector.
struct XaSector
{
	XaSector();

	// Return payload data byte at "index".
	uint8_t getDataByte(uint16_t index) const;

	enum XaSectorStatus
	{
		XA_SECTOR_STATUS_OK,
		XA_SECTOR_STATUS_INVALID_DATA,
		XA_SECTOR_STATUS_INVALID_INPUT
	};

	struct ResultXaSector
	{
		ResultXaSector(const XaSector* sector, XaSectorStatus status) :
			m_sector(sector),
			m_status(status)
		{
		}
		
		const XaSector* getSectorPtr() const { return m_sector; }
		XaSectorStatus getSectorStatus() const { return m_status; }

	private:
		const XaSector* m_sector;
		XaSectorStatus m_status;
	};

	// Validate CD-ROM XA Mode 1 or 2 sector.
	ResultXaSector validateMode1_2(const MinuteSecondFrame& minuteSecondFrame);

	// Parse and validate CD-ROM XA mode 2 sector.
	// Regular CD-ROM defines mode 2 as just containing 0x920 bytes of
	// "raw" data after the 16 byte sector header. However the CD-ROM XA spec
	// defines two possible "forms" for this mode 2 data, there's an 8 byte sub-header
	// at the beginning of the data that will tell us how to interpret it.
	ResultXaSector validateMode2();

	// CD-ROM XA Mode 2 Form 1: 0x800 bytes of data protected by a
	// 32 bit CRC for error detection and 276 bytes of error correction codes.
	ResultXaSector validateMode2Form1() const;

	// CD-ROM XA Mode 2 Form 2: 0x914 bytes of data without ECC or EDC.
	// Last 4 bytes are reserved for quality control, but the CDi spec doesn't
	// mandare what goes in it exactly, only that it is recommended that the same
	// EDC algorithm should be used here as is used for the Form 1 sectors. If this
	// algorithm is not used, then the reserved bytes are set to 0.
	ResultXaSector validateMode2Form2() const;

	// Return the MinuteSecondFrame structure in the sector's header.
	MinuteSecondFrame getMinuteSecondFrame() const;

	// Return the raw sector as a byte slice.
	const uint8_t* getRawSectorInBytes() const;

//private:
	// The raw array of 2352 bytes contained in the sector.
	uint8_t m_raw[SECTOR_SIZE];
};

// Playstation disc
struct Disc
{
	Disc(std::ifstream&& file, Region region);

	enum DiscStatus
	{
		DISC_STATUS_OK,
		DISC_STATUS_INVALID_PATH,
		DISC_STATUS_INVALID_DATA
	};

	struct ResultDisc
	{
		ResultDisc(const Disc* disc, DiscStatus status) :
			m_disc(disc),
			m_status(status)
		{
		}
		const Disc* m_disc;
		DiscStatus m_status;
	};

	// Reify a disc from file at "path" and attempt to identify it.
	static ResultDisc initializeFromPath(const std::string& path);

	Region getRegion() const;

	// Attempt to discover the region of the disc. This way we know
	// which string to return in the CD-ROM drive's "get id" command
	// and we can decide which BIOS and output video standard to use
	// based on the game disk.
	ResultDisc extractRegion();

	// Read a Mode 1 or 2 CD-ROM XA sector and validate it. The function
	// will return an error if used on a CD-DA raw audio sector.
	XaSector::ResultXaSector readDataSector(const MinuteSecondFrame& minuteSecondFrame);

	// Read a raw CD sector without any validation. For Mode 1 and 2
	// sectors XaSector::validateMode1_2 should be called to make sure
	// the sector is valid.
	XaSector::ResultXaSector readSector(const MinuteSecondFrame& minuteSecondFrame);

private:
	// BIN file
	std::ifstream m_file;
	// Disc region
	Region m_region;
};
