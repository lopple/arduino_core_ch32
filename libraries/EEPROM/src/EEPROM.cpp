/*
  EEPROM - Enables reading and writing to non-volatile storage in the processor.
  
  Ported by Maxint R&D to CH32, based on multiple sources, see EEPROM.h.

  From the CH32V004 Data Sheet:
   "Built-in 1920 bytes of system storage (System FLASH) for system bootloader storage (factory-cured
   bootloader) 64 bytes are used for the system non-volatile configuration information storage area and 64 bytes
   are used for the user select word storage area."
  So next to 16kB of Code FLASH, the chip features 2kB Flash for boot, configuration and storage. That 64 bytes
  user select word storage area is the area we can use to emulate on chip EEPROM memory.

  From the CH32V003 Reference Manual:
   "The user-option bytes is solidified in FLASH and will be reloaded into the corresponding register after system
    reset, and can be erased and programmed by the user at will. The user option bytes information block has a
    total of 8 bytes (4 bytes for write protection, 1 byte for read protection, 1 byte for configuration options, and
    2 bytes for storing user data), and each bit has its inverse code bit for checksum during loading."
   These 8 bytes use 16 bytes of flash. The total storage area page is 64 bytes, leaving 48 bytes available, 
   (including required inverse values). This is 24 bytes that can be used plus the two data0 and data1 bytes. 
   Layout for uint8_t _data[26]: { ob[4], ob[6], ob[16...62] ].
*/
#include <EEPROM.h>

#if defined(CH32V00x)
#define OB_AVAIL_DATA_START 8
#endif

EEPROMClass::EEPROMClass(void) {
}

EEPROMClass::~EEPROMClass() {
  end();
}

uint32_t EEPROMClass::ReadOptionBytes()
{ // Read data0 and data1 option bytes
  // These bytes are special, as in that they are 16-bit words, each having their value in the first byte and the inverse value as second byte.
  // Their registers are automatically filled at boot time, so they can be read before calling EEPROM.begin();
   uint32_t uRes=0;
   uRes|=OB->Data0;
   uRes<<=16;
   uRes|=OB->Data1;
   return uRes;
}


void EEPROMClass::begin(void)
{
#if defined(CH32V00x)
  _size = 26;   // Option bytes available on CH32V003: 64 - 16 = 48; 48/2=24, 2+24=26
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
  _size = 58;   // Option bytes available on V10x/V20x/V30x: 128 - 16 = 112; 112/2=56, 2+56=58
#elif defined(CH32X035) || defined(CH32L10x) || defined(CH32VM00X)
  _size = 122;  // Option bytes available on X035/L10x/VM00X: 256 - 16 = 240; 240/2=120, 2+120=122
#else
  _size = 2;    // Fallback default
#endif

  // Allocate data buffer and copy the current content from storage
  if(!_data)
  {
    // Allocate the RAM data buffer
    _data = new uint8_t[_size];
    
    // Copy data from the option byte words
    // Using a simple (uint8_t) cast will ignore the inversed value in the second half-word
    uint16_t *ob16p=(uint16_t *)OB_BASE;
    _data[0]=(uint8_t)ob16p[2];   // simple cast ignores the inversed second half-word
    _data[1]=(uint8_t)ob16p[3];   // simple cast ignores the inversed second half-word
    
    for(int i=2; i<(int)_size; i++) {
      _data[i]=(uint8_t)ob16p[8+(i-2)];
    }
  }
  _dirty = false;
}


uint8_t * EEPROMClass::getDataPtr() {
  _dirty = true;
  return &_data[0];
}

uint8_t const * EEPROMClass::getConstDataPtr() const {
  return &_data[0];
}

uint8_t EEPROMClass::read( int const idx ) {
  if(_data && idx>=0 && (size_t)idx<_size)
    return(_data[idx]);
  return(0);
}

void EEPROMClass::write( int const idx, uint8_t  const val ) {
  if(_data && idx>=0 && (size_t)idx<_size) {
    _dirty = true;
    _data[idx]=val;
  }
}

void EEPROMClass::erase(void) {
  _dirty = true;
  for (int i=0;i<(int)_size; i++)
    _data[i]=0xFF;
}


bool EEPROMClass::commit()
{ // Write the _data array to the available area for option bytes
  if(!_dirty)
    return(true);

#if defined(CH32V00x)
  volatile uint16_t hold[8]; 		// array to hold reserved values while erasing
  uint32_t *hold32p=(uint32_t *)hold;
  uint32_t *ob32p=(uint32_t *)OB_BASE;
  hold32p[0]=ob32p[0];    // Copy RDPR and USER
  
  // Compute correct inverse values in the high byte of each 16-bit word
  uint16_t data0_val = (((uint16_t)(~_data[0]) & 0xFF) << 8) | _data[0];
  uint16_t data1_val = (((uint16_t)(~_data[1]) & 0xFF) << 8) | _data[1];
  hold32p[1] = data0_val | ((uint32_t)data1_val << 16);

  hold32p[2]=ob32p[2];    // Copy WRPR0 and WEPR1
  hold32p[3]=ob32p[3];    // Copy reserved WRPR2 and WEPR3

  // Unlock both the general Flash and the User-selected words
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  FLASH->OBKEYR = FLASH_KEY1;
  FLASH->OBKEYR = FLASH_KEY2;

  FLASH->CTLR |= CR_OPTER_Set;        // OBER RW Perform user-selected word erasure	
  FLASH->CTLR |= CR_STRT_Set;         // STRT RW1 Start. Set 1 to start an erase action,hw automatically clears to 0
  while (FLASH->STATR & FLASH_BUSY);  // Wait for flash operation to be done
  FLASH->CTLR &= CR_OPTER_Reset;      // Disable erasure mode	

  // Write the held values back one-by-one
  FLASH->CTLR |= CR_OPTPG_Set;   			// OBG  RW Perform user-selected word programming
  uint16_t *ob16p=(uint16_t *)OB_BASE;
  for (int i=0;i<sizeof(hold)/sizeof(hold[0]); i++) {
    ob16p[i]=hold[i];
    while (FLASH->STATR & FLASH_BUSY);	// Wait for flash operation to be done
  }

  // Then write the remainder of the data block
  if(_data && _size)
  {
    for (int i=2;i<(int)_size; i++) {
      if(_data[i]!=0xFF) {
        ob16p[8+(i-2)]=_data[i];
        while (FLASH->STATR & FLASH_BUSY);	// Wait for flash operation to be done
      }
    }
  }
    
  FLASH->CTLR &= CR_OPTPG_Reset;			// Disable programming mode
  FLASH->CTLR|=CR_LOCK_Set;				// Lock flash memories again

#elif defined(CH32V10x)
  // 1. Read existing configuration to preserve them
  uint16_t *ob16p = (uint16_t *)OB_BASE;
  uint8_t user_val = (uint8_t)ob16p[1]; // USER
  uint8_t wrpr0 = (uint8_t)ob16p[4];
  uint8_t wrpr1 = (uint8_t)ob16p[5];
  uint8_t wrpr2 = (uint8_t)ob16p[6];
  uint8_t wrpr3 = (uint8_t)ob16p[7];

  // 2. Erase option bytes
  FLASH_Unlock();
  FLASH_EraseOptionBytes();

  // 3. Rewrite USER and WRPRs
  FLASH_ProgramOptionByteData(0x1FFFF802, user_val);
  FLASH_ProgramOptionByteData(0x1FFFF808, wrpr0);
  FLASH_ProgramOptionByteData(0x1FFFF80A, wrpr1);
  FLASH_ProgramOptionByteData(0x1FFFF80C, wrpr2);
  FLASH_ProgramOptionByteData(0x1FFFF80E, wrpr3);

  // 4. Write Data0, Data1
  FLASH_ProgramOptionByteData(0x1FFFF804, _data[0]);
  FLASH_ProgramOptionByteData(0x1FFFF806, _data[1]);

  // 5. Write the remaining free bytes
  if(_data && _size)
  {
    for (int i = 2; i < (int)_size; i++) {
      if (_data[i] != 0xFF) {
        FLASH_ProgramOptionByteData(0x1FFFF810 + 2 * (i - 2), _data[i]);
      }
    }
  }
  FLASH_Lock();

#elif defined(CH32V20x) || defined(CH32V30x)
  uint16_t hold[64];
  uint32_t Addr = 0x1FFFF800;
  
  // 1. Read current option bytes
  for (int i = 0; i < 64; i++) {
    hold[i] = *(volatile uint16_t *)(Addr + 2 * i);
  }

  // 2. Update Data0 and Data1
  hold[2] = (((uint16_t)(~_data[0]) & 0xFF) << 8) | _data[0];
  hold[3] = (((uint16_t)(~_data[1]) & 0xFF) << 8) | _data[1];

  // 3. Update the free space
  if(_data && _size)
  {
    for (int i = 2; i < (int)_size; i++) {
      hold[8 + (i - 2)] = (((uint16_t)(~_data[i]) & 0xFF) << 8) | _data[i];
    }
  }

  // 4. Erase and write back
  FLASH->OBKEYR = FLASH_KEY1;
  FLASH->OBKEYR = FLASH_KEY2;

  // Erase
  FLASH->CTLR |= CR_OPTER_Set;
  FLASH->CTLR |= CR_STRT_Set;
  while (FLASH->STATR & FLASH_BUSY); // Wait for busy flag
  FLASH->CTLR &= ~CR_OPTER_Set;

  // Write
  FLASH->CTLR |= CR_OPTPG_Set;
  for (int i = 0; i < 64; i++) {
    *(volatile uint16_t *)(Addr + 2 * i) = hold[i];
    while (FLASH->STATR & FLASH_BUSY);
  }
  FLASH->CTLR &= ~CR_OPTPG_Set;

#elif defined(CH32X035) || defined(CH32L10x) || defined(CH32VM00X)
  uint32_t hold[64];
  uint32_t *ob32p = (uint32_t *)OB_BASE;
  
  // 1. Read current option bytes
  for (int i = 0; i < 64; i++) {
    hold[i] = ob32p[i];
  }

  // 2. Update Data0 and Data1
  uint16_t data0_val = (((uint16_t)(~_data[0]) & 0xFF) << 8) | _data[0];
  uint16_t data1_val = (((uint16_t)(~_data[1]) & 0xFF) << 8) | _data[1];
  hold[1] = data0_val | ((uint32_t)data1_val << 16);

  // 3. Update the remaining 120 bytes
  if(_data && _size)
  {
    for (int i = 2; i < (int)_size; i += 2) {
      uint8_t b1 = _data[i];
      uint8_t b2 = (i + 1 < (int)_size) ? _data[i + 1] : 0xFF;
      uint16_t val1 = (((uint16_t)(~b1) & 0xFF) << 8) | b1;
      uint16_t val2 = (((uint16_t)(~b2) & 0xFF) << 8) | b2;
      hold[4 + (i - 2) / 2] = val1 | ((uint32_t)val2 << 16);
    }
  }

  // 4. Erase and write back
  FLASH_EraseOptionBytes();
  FLASH_Unlock_Fast();
  FLASH_BufReset();

  for (int i = 0; i < 64; i++) {
    FLASH_BufLoad((OB_BASE + 4 * i), hold[i]);
  }

  FLASH_ProgramPage_Fast(OB_BASE);
  FLASH_Lock_Fast();
#endif

  _dirty = false;
  return(true);
}

bool EEPROMClass::end() {
  bool retval;

  retval = commit();
  _data = nullptr;
  _size = 0;
  return(retval);
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
EEPROMClass EEPROM;
#endif

