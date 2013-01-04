
#include <SD.h>

///////////////////////////////////////////
///////////////////////////////////////////

const int SD_CS_PIN = 53;

const int MBC_NOT_SUPPORTED = -1;
const int MBC_NONE = 0;
const int MBC_1 = 1;
const int MBC_2 = 2;
const int MBC_3 = 3;
const int MBC_5 = 5;

char romTitle[16];
byte romVersion;
byte cartridgeType;
int mbcType;
boolean colorGameBoy;
boolean superGameBoy;
byte romSize;
byte ramSize;
byte romBanks;
byte ramBanks;
boolean cartridgeRTC;
boolean cartridgeRumble;
boolean cartridgeBattery;

File dumpFile;

char romFileName[] = "DUMP00.GB";
char ramFileName[] = "DUMP00.RAM";

///////////////////////////////////////////
///////////////////////////////////////////

void InitSD()
{
  if (!SD.begin(SD_CS_PIN))
  {
    Serial.println("SD CARD FAILED!");
    Die();
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void CreateROMFileInSD()
{
  for (uint8_t i = 0; i < 100; i++)
  {
    romFileName[4] = i/10 + '0';
    romFileName[5] = i%10 + '0';
    if (!SD.exists(romFileName))
    {
      dumpFile = SD.open(romFileName, FILE_WRITE);
      break;
    }
  }
 
  if (!dumpFile)
  {
    Serial.println("ROM FILE FAILED!");
    Die();
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void CreateRAMFileInSD()
{
  for (uint8_t i = 0; i < 100; i++)
  {
    ramFileName[4] = i/10 + '0';
    ramFileName[5] = i%10 + '0';
    if (!SD.exists(ramFileName))
    {
      dumpFile = SD.open(ramFileName, FILE_WRITE);
      break;
    }
  }
 
  if (!dumpFile)
  {
    Serial.println("RAM FILE FAILED!");
    Die();
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void DataBusAsInput()
{
  DDRF = B00000000;
}

///////////////////////////////////////////
///////////////////////////////////////////

void DataBusAsOutput()
{
  DDRF = B11111111; 
}

///////////////////////////////////////////
///////////////////////////////////////////

byte GetByte(word address)
{
  WriteAddress(address);
  PORTL = B00000101;
  delayMicroseconds(10);
  byte result = PINF;
  PORTL = B00000111;
  delayMicroseconds(10);
  return result;
}

///////////////////////////////////////////
///////////////////////////////////////////

byte GetRAMByte(word address)
{
  WriteAddress(address);
  PORTL = B00000001;
  delayMicroseconds(10);
  byte result = PINF;
  PORTL = B00000111;
  delayMicroseconds(10);
  return result;
}

///////////////////////////////////////////
///////////////////////////////////////////

void PutByte(word address, byte data)
{ 
  WriteAddress(address);
  PORTF = data;
  PORTL = B00000110;
  delayMicroseconds(10);
  PORTL = B00000111;
  delayMicroseconds(10);
}

///////////////////////////////////////////
///////////////////////////////////////////

void WriteAddress(word address)
{ 
  PORTA = address & 0xFF;
  PORTK = (address >> 8) & 0xFF;
}

///////////////////////////////////////////
///////////////////////////////////////////

void DumpROM()
{
  CreateROMFileInSD();
  
  for (byte i = 0; i < romBanks; i++)
  {
    DumpROMBank(i);
  }
  
  dumpFile.close();
  
  Serial.print("ROM DUMPED TO: ");
  Serial.println(romFileName);
}

///////////////////////////////////////////
///////////////////////////////////////////

void DumpROMBank(byte bank)
{
  Serial.print("DUMPING ROM BANK ");
  Serial.println(bank);
  
  word offset = 0;
  
  if (bank > 0)
  {
    offset = 0x4000;  
    SwitchROMBank(bank);
  }
  
  for (word address = 0; address < 0x4000; address++)
  {
    byte data = GetByte(address + offset);
    dumpFile.write(data);
  }
  
  dumpFile.flush();
  
  Serial.println("DUMPED!");
}

///////////////////////////////////////////
///////////////////////////////////////////

void SwitchROMBank(byte bank)
{
  DataBusAsOutput();
  
  switch (mbcType)
  {
    case MBC_1:
    {
      PutByte(0x2100, bank);
      break;
    }
    case MBC_2:
    case MBC_3:
    {
      PutByte(0x2100, bank);
      break;
    }
    case MBC_5:
    {
      PutByte(0x2100, bank);
      break;
    }
  }
  
  DataBusAsInput();
}

///////////////////////////////////////////
///////////////////////////////////////////

void DumpRAM()
{
  if (cartridgeBattery)
  {
    CreateRAMFileInSD();
    EnableRAM();
    
    int maxBanks = ramBanks;
    
    if (mbcType == MBC_2)
    {
      maxBanks = 1;
    }
    
    for (byte i = 0; i < maxBanks; i++)
    {
      DumpRAMBank(i);
    }
    
    DisableRAM();
    
    dumpFile.close();
    
    Serial.print("RAM DUMPED TO: ");
    Serial.println(ramFileName);
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void DumpRAMBank(byte bank)
{
  word maxAddress = 0xC000;
  
  if (mbcType == MBC_2)
  {
    Serial.println("DUMPING MBC2 RAM");
    maxAddress = 0xA200;
  }
  else
  {
    Serial.print("DUMPING RAM BANK ");
    Serial.println(bank);
  }
    
  SwitchRAMBank(bank);
    
  for (word address = 0xA000; address < maxAddress; address++)
  {
    byte data = GetRAMByte(address);
    dumpFile.write(data);
  }
  
  dumpFile.flush();
  
  Serial.println("DUMPED!");
}

///////////////////////////////////////////
///////////////////////////////////////////

void SwitchRAMBank(byte bank)
{
  if (ramBanks > 1)
  {
    DataBusAsOutput();
    
    switch (mbcType)
    {
      case MBC_1:
      case MBC_3:
      case MBC_5:
      {
        PutByte(0x4000, bank);
        break;
      }
    }
    
    DataBusAsInput();
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void EnableRAM()
{
  DataBusAsOutput();
  PutByte(0x0000, 0x0A); 
  delayMicroseconds(10);
  DataBusAsInput();
}

///////////////////////////////////////////
///////////////////////////////////////////

void DisableRAM()
{
  DataBusAsOutput();
  PutByte(0x0000, 0x00);
  delay(50); 
  DataBusAsInput();
}

///////////////////////////////////////////
///////////////////////////////////////////

void GetTitle()
{
  for (int i = 0x134; i < 0x143; i++)
  {
    romTitle[i - 0x0134] = GetByte(i);
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void GetRomBanks()
{
  switch (romSize)
  {
    case 0x00:
      romBanks = 2;
      break;
    case 0x01:
      romBanks = 4;
      break;
    case 0x02:
      romBanks = 8;
      break;
    case 0x03:
      romBanks = 16;
      break;
    case 0x04:
      romBanks = 32;
      break;
    case 0x05:
      romBanks = 64;
      break;
    case 0x06:
      romBanks = 128;
      break;
    case 0x07:
      romBanks = 256;
      break;
    default:
      Serial.println("INVALID ROM SIZE!");
      romBanks = 2;
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void GetRamBanks()
{
  switch (ramSize)
  {
    case 0x00:
      ramBanks = 0;
      break;
    case 0x01:
      ramBanks = 1;
      break;
    case 0x02:
      ramBanks = 1;
      break;
    case 0x03:
      ramBanks = 4;
      break;
    case 0x04:
      ramBanks = 16;
      break;
    default:
      Serial.println("INVALID RAM SIZE!");
      ramBanks = 0;
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void GetType()
{
  cartridgeType = GetByte(0x0147);
  
  switch (cartridgeType)
  {
    case 0x00:
      // NO MBC
    case 0x08:
      // ROM   
      // SRAM 
    case 0x09:
      // ROM
      // SRAM
      // BATT
      mbcType = MBC_NONE;
      break;
    case 0x01:
      // MBC1
    case 0x02:
      // MBC1
      // SRAM
    case 0x03:
      // MBC1
      // SRAM
      // BATT
    case 0xFF:
      // Hack to accept HuC1 as a MBC1
      mbcType = MBC_1;
      break;
    case 0x05:
      // MBC2
      // SRAM
    case 0x06:
      // MBC2
      // SRAM
      // BATT
      mbcType = MBC_2;
      break;
    case 0x0F:
      // MBC3
      // TIMER
      // BATT
    case 0x10:
      // MBC3
      // TIMER
      // BATT
      // SRAM
    case 0x11:
      // MBC3
    case 0x12:
      // MBC3
      // SRAM
    case 0x13:
      // MBC3
      // BATT
      // SRAM
    case 0xFC:
      // Game Boy Camera
      mbcType = MBC_3;
      break;
    case 0x19:
      // MBC5
    case 0x1A:
      // MBC5
      // SRAM
    case 0x1B:
      // MBC5
      // BATT
      // SRAM
    case 0x1C:
      // RUMBLE
    case 0x1D:
      // RUMBLE
      // SRAM
    case 0x1E:
      // RUMBLE
      // BATT
      // SRAM
      mbcType = MBC_5;
      break;
    case 0x0B:
      // MMMO1
    case 0x0C:
      // MMM01   
      // SRAM 
    case 0x0D:
      // MMM01
      // SRAM
      // BATT
    case 0x15:
      // MBC4
    case 0x16:
      // MBC4
      // SRAM 
    case 0x17:
      // MBC4
      // SRAM
      // BATT
    case 0x22:
      // MBC7
      // BATT
      // SRAM
    case 0x55:
      // GG
    case 0x56:
      // GS3
    case 0xFD:
      // TAMA 5
    case 0xFE:
      // HuC3
      mbcType = MBC_NOT_SUPPORTED;
      break;
    default:
      mbcType = MBC_NOT_SUPPORTED;
  }
  
  switch (cartridgeType)
  {
    case 0x03:
    case 0x06:
    case 0x09:
    case 0x0D:
    case 0x0F:
    case 0x10:
    case 0x13:
    case 0x17:
    case 0x1B:
    case 0x1E:
    case 0x22:
    case 0xFD:
    case 0xFF:
      cartridgeBattery = true;
      break;
    default:
      cartridgeBattery = false;
  }

  switch (cartridgeType)
  {
    case 0x0F:
    case 0x10:
      cartridgeRTC = true;
      break;
    default:
      cartridgeRTC = false;
  }

  switch (cartridgeType)
  {
    case 0x1C:
    case 0x1D:
    case 0x1E:
      cartridgeRumble = true;
      break;
    default:
      cartridgeRumble = false;
  }
}

///////////////////////////////////////////
///////////////////////////////////////////

void GatherMetadata()
{
  GetTitle();
  GetType();
  byte color = GetByte(0x143);
  colorGameBoy = (color == 0x80) || (color == 0xC0);
  superGameBoy = GetByte(0x146) == 0x03;
  romSize = GetByte(0x148);
  ramSize = GetByte(0x149);
  GetRomBanks();
  GetRamBanks();
  romVersion = GetByte(0x14C);
  
  Serial.print("TITLE: ");
  Serial.println(romTitle);
  Serial.print("VERSION: ");
  Serial.println(romVersion);
  Serial.print("TYPE: 0x");
  Serial.println(cartridgeType, HEX);
  Serial.print("MBC: ");
  Serial.println(mbcType);
  Serial.print("ROM SIZE: 0x");
  Serial.println(romSize, HEX);
  Serial.print("ROM BANKS: ");
  Serial.println(romBanks);
  Serial.print("RAM SIZE: 0x");
  Serial.println(ramSize, HEX);
  Serial.print("RAM BANKS: ");
  Serial.println(ramBanks);
  Serial.print("RAM BATTERY: ");
  Serial.println(cartridgeBattery);
  Serial.print("CGB: ");
  Serial.println(colorGameBoy);
  Serial.print("SGB: ");
  Serial.println(superGameBoy);
  Serial.print("RTC: ");
  Serial.println(cartridgeRTC);
  Serial.print("RUMBLE: ");
  Serial.println(cartridgeRumble);
}

///////////////////////////////////////////
///////////////////////////////////////////

boolean ValidCheckSum()
{
  int checksum = 0;

  for (int j = 0x134; j < 0x14E; j++)
  {
      checksum += GetByte(j);
  }

  return ((checksum + 25) & 0xFF) == 0;
}

///////////////////////////////////////////
///////////////////////////////////////////

void Die()
{
  Reset();
  while (1) ;
}

///////////////////////////////////////////
///////////////////////////////////////////

void Reset()
{
  DDRA = B11111111; // PORT A for Address bus LSB as output
  DDRK = B11111111; // PORT K for Address bus MSB as output
  DDRF = B00000000; // PORT F for Data bus as input
  DDRL = B00000111; // PORT F for RD, WR and CS as output
  
  PORTL = B00000000; // Set RD, WR and CS to HIGH
  PORTA = B00000000;
  PORTK = B00000000;
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  pinMode(53, OUTPUT); // for SD card
  Reset();
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

void loop()
{
  Serial.println("== START ==");
  
  PORTL = B00000111;
  delayMicroseconds(10);
  
  InitSD();
  
  GatherMetadata();

  if (ValidCheckSum())
  {
    Serial.println("CHECKSUM OK!");
  }
  else
  {
    Serial.println("INVALID ROM!");
    Die();
  }
  
  DumpROM();
  DumpRAM();
  
  Serial.println("==  END  ==");
  
  Die();
}

///////////////////////////////////////////
///////////////////////////////////////////

