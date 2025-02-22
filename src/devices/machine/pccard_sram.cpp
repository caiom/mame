// license: BSD-3-Clause
// copyright-holders: Dirk Best
/***************************************************************************

    SRAM PC Cards

***************************************************************************/

#include "emu.h"
#include "pccard_sram.h"
#include "machine/nvram.h"

//#define LOG_GENERAL (1U << 0)
#define LOG_ATTRIBUTE (1U << 1)

#define VERBOSE (LOG_GENERAL | LOG_ATTRIBUTE)
#include "logmacro.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

// devices without attribute memory
DEFINE_DEVICE_TYPE(PCCARD_SRAM_MITSUBISHI_1M, pccard_mitsubishi_mf31m1_lycat01_device, "mitsubishi_mf31m1_lycat01", "Mitsubishi Melcard 1 MB SRAM")

// devices with attribute memory
DEFINE_DEVICE_TYPE(PCCARD_SRAM_CENTENNIAL_1M, pccard_centennial_sl01m_15_11194_device, "centennial_sl01m_15_11194", "Centennial 1 MB SRAM")
DEFINE_DEVICE_TYPE(PCCARD_SRAM_CENTENNIAL_2M, pccard_centennial_sl02m_15_11194_device, "centennial_sl02m_15_11194", "Centennial 2 MB SRAM")
DEFINE_DEVICE_TYPE(PCCARD_SRAM_CENTENNIAL_4M, pccard_centennial_sl04m_15_11194_device, "centennial_sl04m_15_11194", "Centennial 4 MB SRAM")

//-------------------------------------------------
//  inputs
//-------------------------------------------------

static INPUT_PORTS_START( card )
	PORT_START("switches")
	PORT_CONFNAME(0x01, 0x00, "Battery Failed") PORT_WRITE_LINE_DEVICE_MEMBER(DEVICE_SELF, pccard_sram_device, battery_voltage_1_w)
	PORT_CONFSETTING(   0x01, DEF_STR(Yes))
	PORT_CONFSETTING(   0x00, DEF_STR(No))
	PORT_CONFNAME(0x02, 0x00, "Battery Low")    PORT_WRITE_LINE_DEVICE_MEMBER(DEVICE_SELF, pccard_sram_device, battery_voltage_2_w)
	PORT_CONFSETTING(   0x02, DEF_STR(Yes))
	PORT_CONFSETTING(   0x00, DEF_STR(No))
	PORT_CONFNAME(0x04, 0x04, "Write Protect")  PORT_WRITE_LINE_DEVICE_MEMBER(DEVICE_SELF, pccard_sram_device, write_protect_w)
	PORT_CONFSETTING(   0x04, DEF_STR(No))
	PORT_CONFSETTING(   0x00, DEF_STR(Yes))
INPUT_PORTS_END

ioport_constructor pccard_sram_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(card);
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  pccard_sram_device - constructor
//-------------------------------------------------

pccard_sram_device::pccard_sram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_memory_interface(mconfig, *this),
	device_image_interface(mconfig, *this),
	device_pccard_interface(mconfig, *this),
	m_card_detect(false),
	m_switches(*this, "switches")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void pccard_sram_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void pccard_sram_device::device_reset()
{
	// forward initial state of battery/write protect to slot
	if (m_card_detect)
	{
		m_slot->battery_voltage_1_w(BIT(m_switches->read(), 0));
		m_slot->battery_voltage_2_w(BIT(m_switches->read(), 1));
		m_slot->write_protect_w(BIT(m_switches->read(), 2));
	}
	else
	{
		m_slot->battery_voltage_1_w(0);
		m_slot->battery_voltage_2_w(0);
		m_slot->write_protect_w(0);
	}
}

//-------------------------------------------------
//  memory_space_config - configure memory space
//-------------------------------------------------

device_memory_interface::space_config_vector pccard_sram_device::memory_space_config() const
{
	return space_config_vector
	{
		std::make_pair(0, &m_memory_space_config),
		std::make_pair(1, &m_attribute_space_config)
	};
}

//-------------------------------------------------
//  device_pccard_interface
//-------------------------------------------------

uint16_t pccard_sram_device::read_memory(offs_t offset, uint16_t mem_mask)
{
	uint16_t data = 0xffff;

	if (m_card_detect)
		data = space(0).read_word(offset * 2, mem_mask);

	return data;
}

void pccard_sram_device::write_memory(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (m_card_detect && BIT(m_switches->read(), 2))
		space(0).write_word(offset * 2, data, mem_mask);
}

uint16_t pccard_sram_device::read_reg(offs_t offset, uint16_t mem_mask)
{
	uint16_t data = 0xffff;

	if (has_configured_map(1) && m_card_detect)
		data = space(1).read_word(offset * 2, mem_mask);

	LOGMASKED(LOG_ATTRIBUTE, "attribute memory r: %06x = %04x & %04x\n", offset, data, mem_mask);

	return data & 0x00ff;
}

void pccard_sram_device::write_reg(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	LOGMASKED(LOG_ATTRIBUTE, "attribute memory w: %06x = %04x & %04x\n", offset, data, mem_mask);

	if (has_configured_map(1) && m_card_detect && BIT(m_switches->read(), 2))
		space(1).write_word(offset * 2, data & 0x00ff, mem_mask);
}

void pccard_sram_device::card_inserted(bool state)
{
	m_card_detect = state;
	m_slot->card_detect_w(state ? 1 : 0);
}


/***************************************************************************

    Mitsubishi Melcard

    MF31M1-LYCAT01: 8/16-bit Data Bus Static RAM Card

***************************************************************************/

pccard_mitsubishi_sram_device::pccard_mitsubishi_sram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	pccard_sram_device(mconfig, type, tag, owner, clock),
	m_sram(*this, "sram")
{
}

std::error_condition pccard_mitsubishi_sram_device::call_load()
{
	card_inserted(false);

	if (length() != m_sram.bytes())
		return image_error::INVALIDLENGTH;

	if (fread(&m_sram[0], m_sram.bytes()) != m_sram.bytes())
		return image_error::UNSPECIFIED;

	card_inserted(true);

	return std::error_condition();
}

std::error_condition pccard_mitsubishi_sram_device::call_create(int format_type, util::option_resolution *format_options)
{
	card_inserted(false);

	// clear ram
	std::fill_n(&m_sram[0], m_sram.length(), 0);

	if (fwrite(&m_sram[0], m_sram.bytes()) != m_sram.bytes())
		return image_error::UNSPECIFIED;

	card_inserted(true);

	return std::error_condition();
}

void pccard_mitsubishi_sram_device::call_unload()
{
	if (m_card_detect && !is_readonly())
	{
		fseek(0, SEEK_SET);
		fwrite(&m_sram[0], m_sram.bytes());
	}

	std::fill_n(&m_sram[0], m_sram.length(), 0);

	card_inserted(false);
}

pccard_mitsubishi_mf31m1_lycat01_device::pccard_mitsubishi_mf31m1_lycat01_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	pccard_mitsubishi_sram_device(mconfig, PCCARD_SRAM_MITSUBISHI_1M, tag, owner, clock)
{
	m_memory_space_config = address_space_config("memory", ENDIANNESS_LITTLE, 16, 20, 0, address_map_constructor(FUNC(pccard_mitsubishi_mf31m1_lycat01_device::memory_map), this));
	m_attribute_space_config = address_space_config("attribute", ENDIANNESS_LITTLE, 16, 14, 0);
}

void pccard_mitsubishi_mf31m1_lycat01_device::memory_map(address_map &map)
{
	map(0x000000, 0x0fffff).ram().share("sram");
}


/***************************************************************************

    Centennial SRAM

    SL01M-15-11194: 1 MB SRAM w/ Write Protect - 150 ns Rechargeable Lithium Battery
    SL02M-15-11194: 2 MB SRAM w/ Write Protect - 150 ns Rechargeable Lithium Battery
    SL04M-15-11194: 4 MB SRAM w/ Write Protect - 150 ns Rechargeable Lithium Battery

    TODO:
    - Get a real EEPROM dump from a factory new card and verify attribute
      memory map
    - Add more variants

    Notes:
    - EEPROM type is probably 28C64A, possible variants with 28C16A

***************************************************************************/

pccard_centennial_sram_device::pccard_centennial_sram_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	pccard_sram_device(mconfig, type, tag, owner, clock),
	m_sram(*this, "sram"),
	m_eeprom(*this, "eeprom"),
	m_eeprom_default(*this, "eeprom")
{
}

std::error_condition pccard_centennial_sram_device::call_load()
{
	card_inserted(false);

	if (length() != m_sram.bytes() + m_eeprom.bytes())
		return image_error::INVALIDLENGTH;

	if (fread(&m_sram[0], m_sram.bytes()) != m_sram.bytes())
		return image_error::UNSPECIFIED;

	if (fread(&m_eeprom[0], m_eeprom.bytes()) != m_eeprom.bytes())
		return image_error::UNSPECIFIED;

	card_inserted(true);

	return std::error_condition();
}

std::error_condition pccard_centennial_sram_device::call_create(int format_type, util::option_resolution *format_options)
{
	card_inserted(false);

	// clear ram
	std::fill_n(&m_sram[0], m_sram.length(), 0);

	// initialize eeprom data from default data
	std::copy_n(m_eeprom_default->base(), m_eeprom.length(), &m_eeprom[0]);

	if (fwrite(&m_sram[0], m_sram.bytes()) != m_sram.bytes())
		return image_error::UNSPECIFIED;

	if (fwrite(&m_eeprom[0], m_eeprom.bytes()) != m_eeprom.bytes())
		return image_error::UNSPECIFIED;

	card_inserted(true);

	return std::error_condition();
}

void pccard_centennial_sram_device::call_unload()
{
	if (m_card_detect && !is_readonly())
	{
		fseek(0, SEEK_SET);
		fwrite(&m_sram[0], m_sram.bytes());
		fwrite(&m_eeprom[0], m_eeprom.bytes());
	}

	std::fill_n(&m_sram[0], m_sram.length(), 0);
	std::fill_n(&m_eeprom[0], m_eeprom.length(), 0);

	card_inserted(false);
}

pccard_centennial_sl01m_15_11194_device::pccard_centennial_sl01m_15_11194_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	pccard_centennial_sram_device(mconfig, PCCARD_SRAM_CENTENNIAL_1M, tag, owner, clock)
{
	m_memory_space_config = address_space_config("memory", ENDIANNESS_LITTLE, 16, 20, 0, address_map_constructor(FUNC(pccard_centennial_sl01m_15_11194_device::memory_map), this));
	m_attribute_space_config = address_space_config("attribute", ENDIANNESS_LITTLE, 16, 14, 0, address_map_constructor(FUNC(pccard_centennial_sl01m_15_11194_device::attribute_map), this));
}

void pccard_centennial_sl01m_15_11194_device::memory_map(address_map &map)
{
	map(0x000000, 0x0fffff).ram().share("sram");
}

void pccard_centennial_sl01m_15_11194_device::attribute_map(address_map &map)
{
	map(0x00000, 0x03fff).ram().share("eeprom");
}

ROM_START( eeprom_01 )
	ROM_REGION(0x2000, "eeprom", 0)
	ROM_LOAD("eeprom.bin", 0x0000, 0x2000, BAD_DUMP CRC(2caacff3) SHA1(8141459dccf63a64f4bdf4e2171b0884f2cc390d))
ROM_END

const tiny_rom_entry *pccard_centennial_sl01m_15_11194_device::device_rom_region() const
{
	return ROM_NAME( eeprom_01 );
}

pccard_centennial_sl02m_15_11194_device::pccard_centennial_sl02m_15_11194_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	pccard_centennial_sram_device(mconfig, PCCARD_SRAM_CENTENNIAL_2M, tag, owner, clock)
{
	m_memory_space_config = address_space_config("memory", ENDIANNESS_LITTLE, 16, 21, 0, address_map_constructor(FUNC(pccard_centennial_sl02m_15_11194_device::memory_map), this));
	m_attribute_space_config = address_space_config("attribute", ENDIANNESS_LITTLE, 16, 14, 0, address_map_constructor(FUNC(pccard_centennial_sl02m_15_11194_device::attribute_map), this));
}

void pccard_centennial_sl02m_15_11194_device::memory_map(address_map &map)
{
	map(0x000000, 0x1fffff).ram().share("sram");
}

void pccard_centennial_sl02m_15_11194_device::attribute_map(address_map &map)
{
	map(0x00000, 0x03fff).ram().share("eeprom");
}

ROM_START( eeprom_02 )
	ROM_REGION(0x2000, "eeprom", 0)
	ROM_LOAD("eeprom.bin", 0x0000, 0x2000, BAD_DUMP CRC(0d094f14) SHA1(a542a7395b306b9e34fd0be42d895b7b30013390))
ROM_END

const tiny_rom_entry *pccard_centennial_sl02m_15_11194_device::device_rom_region() const
{
	return ROM_NAME( eeprom_02 );
}

pccard_centennial_sl04m_15_11194_device::pccard_centennial_sl04m_15_11194_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	pccard_centennial_sram_device(mconfig, PCCARD_SRAM_CENTENNIAL_4M, tag, owner, clock)
{
	m_memory_space_config = address_space_config("memory", ENDIANNESS_LITTLE, 16, 22, 0, address_map_constructor(FUNC(pccard_centennial_sl04m_15_11194_device::memory_map), this));
	m_attribute_space_config = address_space_config("attribute", ENDIANNESS_LITTLE, 16, 14, 0, address_map_constructor(FUNC(pccard_centennial_sl04m_15_11194_device::attribute_map), this));
}

void pccard_centennial_sl04m_15_11194_device::memory_map(address_map &map)
{
	map(0x000000, 0x3fffff).ram().share("sram");
}

void pccard_centennial_sl04m_15_11194_device::attribute_map(address_map &map)
{
	map(0x00000, 0x03fff).ram().share("eeprom");
}

ROM_START( eeprom_04 )
	ROM_REGION(0x2000, "eeprom", 0)
	ROM_LOAD("eeprom.bin", 0x0000, 0x2000, BAD_DUMP CRC(ce38fc21) SHA1(155edb39e554cb78547d3b9934a049ee46edc424))
ROM_END

const tiny_rom_entry *pccard_centennial_sl04m_15_11194_device::device_rom_region() const
{
	return ROM_NAME( eeprom_04 );
}
