#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>

#include "pscx_disc.h"
#include "pscx_crc.h"

// ********************** XaSector implementation **********************
XaSector::XaSector()
{
	memset(m_raw, 0x0, sizeof(m_raw));
}

uint8_t XaSector::getDataByte(uint16_t index) const
{
	return m_raw[index];
}

XaSector::ResultXaSector XaSector::validateMode1_2(const MinuteSecondFrame& minuteSecondFrame)
{
	// Check sync pattern.
	for (size_t i = 0; i < _countof(SECTOR_SYNC_PATTERN); ++i)
	{
		if (m_raw[i] != SECTOR_SYNC_PATTERN[i])
		{
			return ResultXaSector(nullptr, XaSectorStatus::XA_SECTOR_STATUS_INVALID_DATA);
		}
	}

	// Check that the expected MSF matches the one we have in the header.
	//if (getMinuteSecondFrame() != MinuteSecondFrame::fromBCD(minuteSecondFrame.getMinute(), minuteSecondFrame.getSecond(), minuteSecondFrame.getFrame()))
	if (getMinuteSecondFrame() != minuteSecondFrame)
	{
		return ResultXaSector(nullptr, XaSectorStatus::XA_SECTOR_STATUS_INVALID_DATA);
	}

	uint8_t mode = m_raw[15];
	switch (mode)
	{
	case 1:
	{
		assert(("Unhandled Mode 1 sector", false));
	}
	case 2:
	{
		return validateMode2();
	}
	}
	return ResultXaSector(nullptr, XaSectorStatus::XA_SECTOR_STATUS_INVALID_DATA);
}

XaSector::ResultXaSector XaSector::validateMode2()
{
	// Mode 2 sub-header
	// byte 16: File number
	// byte 17: Channel number
	// byte 18: Submode
	// byte 19: Coding information
	// byte 20: File number
	// byte 21: Channel number
	// byte 22: Submode
	// byte 23: Coding information

	// Make sure that the two copies of the subcode are the same,
	// otherwise the sector is probably corrupted or is in the wrong format.
	uint8_t submode = m_raw[18];
	uint8_t submodeCopy = m_raw[22];

	if (submode != submodeCopy)
	{
		// Sector msf mode 2 submode missmatch.
		return ResultXaSector(nullptr, XaSectorStatus::XA_SECTOR_STATUS_INVALID_DATA);
	}

	// Look for form in submode bit 5.
	if (submode & 0x20)
		return validateMode2Form2();
	return validateMode2Form1();
}

XaSector::ResultXaSector XaSector::validateMode2Form1() const
{
	 // Slice ?
	std::vector<uint8_t> rawSlice;
	rawSlice.resize(2056);

	memcpy(rawSlice.data(), m_raw + 16, 2056 * sizeof(uint8_t));

	// Validate CRC.
	uint32_t crc = crc32(rawSlice);

	uint32_t sectorCrc = (uint32_t)m_raw[2072]
		| ((uint32_t)m_raw[2073] << 8)
		| ((uint32_t)m_raw[2074] << 16)
		| ((uint32_t)m_raw[2075] << 24);

	if (crc != sectorCrc)
	{
		// Sector appears corrupted.
		// Mode 2 Form 1 CRC missmatch.
		return ResultXaSector(nullptr, XaSectorStatus::XA_SECTOR_STATUS_INVALID_DATA);
	}
	return ResultXaSector(this, XaSectorStatus::XA_SECTOR_STATUS_OK);
}

XaSector::ResultXaSector XaSector::validateMode2Form2() const
{
	//assert(0, "Unhandled Mode 2 Form 2 sector");
	return ResultXaSector(this, XaSectorStatus::XA_SECTOR_STATUS_OK);
}

MinuteSecondFrame XaSector::getMinuteSecondFrame() const
{
	// The MSF is recorded just after the sync pattern.
	return MinuteSecondFrame::fromBCD(m_raw[12], m_raw[13], m_raw[14]);
}

const uint8_t* XaSector::getRawSectorInBytes() const
{
	return m_raw;
}

// ********************** Disc implementation **********************
Disc::Disc(std::ifstream&& file, Region region) :
	m_file(std::move(file)), // ?
	m_region(region)
{
}

Disc::ResultDisc Disc::initializeFromPath(const std::string& path)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.good())
		return ResultDisc(nullptr, DiscStatus::DISC_STATUS_INVALID_PATH);

	// Use dummy id for now.
	Disc* disc = new Disc(std::move(file), Region::REGION_JAPAN);
	return disc->extractRegion();
}

Region Disc::getRegion() const
{
	return m_region;
}

Disc::ResultDisc Disc::extractRegion()
{
	// In order to identify the type of disc we're going to use
	// sector 00:02:04 which should contain the "Licensed by..." string.
	MinuteSecondFrame minuteSecondFrame = MinuteSecondFrame::fromBCD(0x0, 0x2, 0x4);

	XaSector::ResultXaSector sector = readDataSector(minuteSecondFrame);
	if (sector.getSectorStatus() != XaSector::XaSectorStatus::XA_SECTOR_STATUS_OK)
		return ResultDisc(nullptr, DiscStatus::DISC_STATUS_INVALID_DATA);

	// An ASCII license string is in the first 76 bytes.
	const XaSector* ptr = sector.getSectorPtr();
	const uint8_t* rawSectorInBytes = ptr->getRawSectorInBytes() + 24;

	const uint32_t licenseBlobSize = 76;
	std::string licenseBlob(rawSectorInBytes, rawSectorInBytes + licenseBlobSize);

	// There are spaces everywhere in the string ( including in the middle of some words ).
	// Let's clean it up to identify the region of the disc.
	licenseBlob.erase(std::remove_if(licenseBlob.begin(), licenseBlob.end(), [](char ch) { return ch < 'A' || ch > 'z'; }), licenseBlob.end());

	if (licenseBlob.compare("LicensedbySonyComputerEntertainmentInc") == 0)
		m_region = Region::REGION_JAPAN;
	else if (licenseBlob.compare("LicensedbySonyComputerEntertainmentAmerica") == 0)
		m_region = Region::REGION_NORTH_AMERICA;
	else if (licenseBlob.compare("LicensedbySonyComputerEntertainmentEurope") == 0)
		m_region = Region::REGION_EUROPE;
	else
	{
		// Couldn't identify the disc region string
		return ResultDisc(nullptr, DiscStatus::DISC_STATUS_INVALID_DATA);
	}

	return ResultDisc(this, DiscStatus::DISC_STATUS_OK);
}

XaSector::ResultXaSector Disc::readDataSector(const MinuteSecondFrame& minuteSecondFrame)
{
	XaSector::ResultXaSector sector = readSector(minuteSecondFrame);
	if (sector.getSectorStatus() == XaSector::XaSectorStatus::XA_SECTOR_STATUS_OK)
	{
		XaSector* ptr = const_cast<XaSector*>(sector.getSectorPtr());
		return ptr->validateMode1_2(minuteSecondFrame);
	}
	return sector;
}

XaSector::ResultXaSector Disc::readSector(const MinuteSecondFrame& minuteSecondFrame)
{
	uint32_t sectorIndex = minuteSecondFrame.getSectorIndex() - 150;

	// Convert in a byte offset in the bin file
	uint64_t byteOffset = (uint64_t)sectorIndex * SECTOR_SIZE;
	m_file.seekg(byteOffset, std::ios_base::beg);

	XaSector* sector = new XaSector;
	if (!m_file.read((char*)sector->m_raw, SECTOR_SIZE))
		return XaSector::ResultXaSector(nullptr, XaSector::XaSectorStatus::XA_SECTOR_STATUS_INVALID_INPUT);
	
	return XaSector::ResultXaSector(sector, XaSector::XaSectorStatus::XA_SECTOR_STATUS_OK);
}
