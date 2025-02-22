// license:BSD-3-Clause
// copyright-holders:Fabio Priuli

#include "emu.h"
#include "slot.h"

#define VERBOSE 0
#include "logmacro.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(MONONCOL_CARTSLOT, mononcol_cartslot_device, "mononcol_cartslot", "Monon Color Cartridge Slot")


//**************************************************************************
//  device_mononcol_cart_interface
//**************************************************************************

device_mononcol_cart_interface::device_mononcol_cart_interface(machine_config const &mconfig, device_t &device) :
	device_interface(device, "mononcart")
{
}

device_mononcol_cart_interface::~device_mononcol_cart_interface()
{
}



//**************************************************************************
//  mononcol_cartslot_device
//**************************************************************************

mononcol_cartslot_device::mononcol_cartslot_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_cartrom_image_interface(mconfig, *this),
	device_single_card_slot_interface<device_mononcol_cart_interface>(mconfig, *this),
	m_cart(nullptr)
{
}


mononcol_cartslot_device::mononcol_cartslot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	mononcol_cartslot_device(mconfig, MONONCOL_CARTSLOT, tag, owner, clock)
{
}

mononcol_cartslot_device::~mononcol_cartslot_device()
{
}

void mononcol_cartslot_device::device_start()
{
	m_cart = get_card_device();
}


/*-------------------------------------------------
 call load
 -------------------------------------------------*/

std::error_condition mononcol_cartslot_device::call_load()
{
	if (m_cart)
	{
		memory_region *romregion(loaded_through_softlist() ? memregion("rom") : nullptr);
		if (loaded_through_softlist() && !romregion)
		{
			osd_printf_error("%s: Software list item has no 'rom' data area\n", basename());
			return image_error::BADSOFTWARE;
		}

		const u32 len = loaded_through_softlist() ? get_software_region_length("rom") : length();
		if (!len || ((len - 1) & len))
		{
			osd_printf_error("%s: Cartridge ROM size is not a power of 2\n", basename());
			return image_error::INVALIDLENGTH;
		}

		if (!loaded_through_softlist())
		{
			LOG("Allocating %u byte cartridge ROM region\n", len);
			romregion = machine().memory().region_alloc(subtag("rom"), len, 4, ENDIANNESS_LITTLE);
			const u32 cnt = fread(romregion->base(), len);
			if (cnt != len)
			{
				osd_printf_error("%s: Error reading cartridge file\n", basename());
				return image_error::UNSPECIFIED;
			}
		}

		m_cart->set_spi_region(romregion->base(), romregion->bytes());
	}
	return std::error_condition();
}



/*-------------------------------------------------
 get default card software
 -------------------------------------------------*/

std::string mononcol_cartslot_device::get_default_card_software(get_default_card_software_hook &hook) const
{
	return software_get_default_slot("rom");
}
