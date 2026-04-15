/*
* *********************************************************************** *
* Author: Jure Grčar                                                      *
* Created: 6. 10. 2025                                                    *
* Descirption: Programska implementacija Conwayeve igre življenja za UEFI *
* *********************************************************************** *
*/

/*
* DOKUMENTACIJA
https://uefi.org/specs/UEFI/2.10/01_Introduction.html
https://x86asm.net/articles/uefi-programming-first-steps/index.html
https://en.wikipedia.org/wiki/UEFI
https://en.wikipedia.org/wiki/Conway's_Game_of_Life
https://www.youtube.com/watch?v=ZFHnbozz7b4
https://github.com/InkboxSoftware/spacegamex64/
https://github.com/tianocore/tianocore.github.io/wiki/EFI-Toolkit
*/

unsigned char SIZE = 64;

// podatkovni tipi za lažje delo, kot dokunentirani v EFI toolkit II in EFI dokumentaciji
typedef unsigned char UINT8;       // 1 byte
typedef unsigned short UINT16;     // 2 bytes
typedef unsigned int UINT32;       // 4 bytes
typedef unsigned long long UINT64; // 8 bytes
typedef unsigned long long UINTN;  // "Native" width (8 bytes on x64)

typedef void *EFI_HANDLE;
typedef UINTN EFI_STATUS;

typedef EFI_STATUS (*EFI_TEXT_SET_MODE)(
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber);

typedef EFI_STATUS (*EFI_SET_WATCHDOG)(
    UINTN Timeout,
    UINT64 WatchdogCode,
    UINTN DataSize,
    UINT16 *WatchdogData);

typedef EFI_STATUS (*EFI_STALL)(UINTN Microseconds);
typedef enum
{
    TimerDefault,  // 0
    TimerPeriodic, // 1
    TimerRelative  // 2
} EFI_TIMER_DELAY;
typedef EFI_STATUS (*EFI_SET_TIMER)(
    void *Event,
    EFI_TIMER_DELAY Type,
    UINT64 TriggerTime);
typedef EFI_STATUS (*EFI_STALL)(UINTN Microseconds);

// ConIn
typedef struct
{
    UINT16 ScanCode;
    UINT16 UnicodeChar;
} EFI_INPUT_KEY;
typedef EFI_STATUS (*EFI_INPUT_RESET)(
    struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    unsigned char ExtendedVerification // boolean
);
typedef EFI_STATUS (*EFI_INPUT_READ_KEY)(
    struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    EFI_INPUT_KEY *Key);

//! STRUCTS
typedef struct
{
    unsigned long long Signature;
    unsigned int Revision;
    unsigned int HeaderSize;
    unsigned int CRC32;
    unsigned int Reserved;
} EFI_HEADER;

typedef struct
{
    UINT16 year;
    UINT8 month;
    UINT8 day;
    UINT8 hour;
    UINT8 minute;
    UINT8 second;
    UINT8 Pad1;
    UINT32 nanoSecond;
    UINT16 TimeZone;
    UINT8 dayLight;
    UINT8 Pad2;
} EFI_TIME;

//! GRAFIKE
typedef struct
{
    UINT32 Red;
    UINT32 Green;
    UINT32 Blue;
    UINT32 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef struct
{
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

typedef EFI_STATUS (*EFI_LOCATE_PROTOCOL)(
    EFI_GUID *Protocol,
    void *Registration,
    void **Interface);

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL
{
    void *QueryMode;
    void *SetMode;
    void *Blt;
    struct
    {
        UINT32 MaxMode;
        UINT32 Mode;
        struct
        {
            UINT32 Version;
            UINT32 HorizontalResolution;
            UINT32 VerticalResolution;
            UINT32 PixelFormat;
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL PixelInformation; // 8bytov
            UINT32 PixelsPerScanLine;
        } *Info;
        UINTN SizeOfInfo;
        UINT64 FrameBufferBase; // frame buffer kazalc
        UINTN FrameBufferSize;
    } *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

//! SYSTEMTABLE
// conin conout in use ostalo je u systemtable
typedef struct
{
    //24 B
    EFI_HEADER header;

    // 8 B
    unsigned short *FirmwareVendor;

    // 4 B
    unsigned int FirmwareRevision;

    // 4B
    unsigned int _padding_alignment;

    void *ConsoleInHandle;
    struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL
    {
        EFI_INPUT_RESET Reset;            // 0
        EFI_INPUT_READ_KEY ReadKeyStroke; // 8
        void *WaitForKey;                 // 16
    } *ConIn;

    void *ConsoleOutHandle;
    struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
    {
        void *Reset;
        long long (*OutputString)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, unsigned short *String);
        void *TestString;
        void *QueryMode;
        EFI_TEXT_SET_MODE SetMode;
        void *SetAttribute;
        EFI_STATUS (*ClearScreen)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
        EFI_STATUS (*SetCursorPosition)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, UINTN Column, UINTN Row);
    } *ConOut;

    void *StandardErrorHandle;
    void *StdErr;

    struct _EFI_RUNTIME_SERVICES
    {
        EFI_HEADER header;

        EFI_STATUS (*GetTime)(EFI_TIME *Time, void *Capabilities);
    } *RuntimeServices;

    struct _EFI_BOOT_SERVICES
    {
        EFI_HEADER header;

        // fst 24
        void *RaiseTPL;
        void *RestoreTPL;

        // 40
        void *AllocatePages;
        void *FreePages;
        void *GetMemoryMap;
        void *AllocatePool;
        void *FreePool;

        // 80
        void *CreateEvent;
        EFI_SET_TIMER SetTimer;
        void *WaitForEvent;
        void *SignalEvent;
        void *CloseEvent;
        void *CheckEvent;

        // 128
        void *InstallProtocolInterface;
        void *ReinstallProtocolInterface;
        void *UninstallProtocolInterface;
        void *HandleProtocol;
        void *Reserved;
        void *RegisterProtocolNotify;
        void *LocateHandle;
        void *LocateDevicePath;
        void *InstallConfigurationTable;

        // fst 200
        void *LoadImage;
        void *StartImage;
        void *Exit;
        void *UnloadImage;
        void *ExitBootServices;

        // fst 240
        void *GetNextMonotonicCount;
        EFI_STALL Stall;
        EFI_SET_WATCHDOG SetWatchdogTimer;

        // image zadeve
        void *ConnectController;       // 264
        void *DisconnectController;    // 272
        void *OpenProtocol;            // 280
        void *CloseProtocol;           // 288
        void *OpenProtocolInformation; // 296
        void *ProtocolsPerHandle;      // 304
        void *LocateHandleBuffer;      // 312

        EFI_LOCATE_PROTOCOL LocateProtocol; // 320

    } *BootServices;

} EFI_SYSTEM_TABLE;

//! Pomozne funkcije
void PrintUint32(EFI_SYSTEM_TABLE *ST, UINT32 value)
{
    UINT16 buffer[11]; //"4294967295" + null
    int i = 9;

    buffer[10] = 0; // null terminated
    if (value == 0)
        buffer[i--] = L'0';

    while (value > 0 && i >= 0)
    {
        buffer[i--] = (UINT16)(L'0' + (value % 10));
        value /= 10;
    }

    ST->ConOut->OutputString(ST->ConOut, &buffer[i + 1]);
}

void NL(EFI_SYSTEM_TABLE *ST) { ST->ConOut->OutputString(ST->ConOut, L"\r\n"); }

//! game logic
// nastav celico na (x, y) na 1
void SetCell(UINT64 *grid, UINT8 x, UINT8 y)
{
    if (x < 64 && y < 64)
    {
        grid[y] |= (1ULL << x);
    }
}

// nastav celico na (x, y) na 0
void ClearCell(UINT64 *grid, UINT8 x, UINT8 y)
{
    if (x < 64 && y < 64)
    {
        grid[y] &= ~(1ULL << x);
    }
}

// vrne stanje celice (0 ali 1)
UINT8 GetCell(UINT64 *grid, UINT8 x, UINT8 y)
{
    if (x < 64 && y < 64)
    {
        return (grid[y] >> x) & 1;
    }
    return 0;
}

int CountNeighbors(UINT64 *grid, int x, int y)
{
    int count = 0;

    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++)
        {
            // preskoc sredno celico ( ker zanjo gledamo)
            if (i == 0 && j == 0)
                continue;

            int nx = x + j;
            int ny = y + i;

            // meja: ne vn iz 64x64
            if (nx >= 0 && nx < 64 && ny >= 0 && ny < 64)
            {
                if (GetCell(grid, (UINT8)nx, (UINT8)ny))
                {
                    count++;
                }
            }
        }
    }
    return count;
}


void DrawRect(UINT32 *fb, UINT32 PSL, UINT32 x, UINT32 y, UINT32 width, UINT32 height, UINT32 color)
{
    // top left
    UINT32 *dest = fb + (y * PSL) + x;

    for (UINT32 i = 0; i < height; i++)
    {
        for (UINT32 j = 0; j < width; j++)
        {
            // napovn vrstico
            dest[j] = color;
        }
        // skoc na zacetek nasledne vrstice
        dest += PSL;
    }
}

void PrintGrid(EFI_SYSTEM_TABLE *st, EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop, UINT64 *grid, UINT8 Sx, UINT8 Sy)
{
    if (!Gop || !Gop->Mode || !Gop->Mode->FrameBufferBase)
        return;

    UINT32 *fb = (UINT32 *)Gop->Mode->FrameBufferBase;
    UINT32 ScreenW = Gop->Mode->Info->HorizontalResolution;
    UINT32 ScreenH = Gop->Mode->Info->VerticalResolution;

    UINT32 PSL = Gop->Mode->Info->PixelsPerScanLine;
    if (PSL == 0)
    {
        PSL = ScreenW;
    } // safety zarad mojih trenutnih problemov z PSL

    // square aspect ratio
    UINT32 TileSize = ScreenH / SIZE;

    // Cela sirina grida
    UINT32 GridTotalWidth = TileSize * SIZE;

    // X offset od centra grida
    UINT32 XOffset = (ScreenW - GridTotalWidth) / 2;

    // Left bar
    DrawRect(fb, ScreenW, 0, 0, XOffset, ScreenH, 0x00000000);
    // right bar
    DrawRect(fb, ScreenW, XOffset + GridTotalWidth, 0, ScreenW - (XOffset + GridTotalWidth), ScreenH, 0x00000000);

    for (UINT8 i = 0; i < SIZE; i++)
    {
        for (UINT8 j = 0; j < SIZE; j++)
        {
            UINT8 a = (grid[i] >> j) & 1;

            UINT32 color;
            if (i == Sy && j == Sx)
            {
                color = 0x00FFFFFF; // curser
            }
            else
            {
                color = a ? 0x0000FF00 : 0x00000000; // zeleno crno ozadje
            }

            // kvadrat v center screena (zarad black barov)
            DrawRect(fb, PSL, XOffset + (TileSize * j), TileSize * i, TileSize, TileSize, color);
        }
    }
}

//! --------------------------------------------
//! |             EFI ENTRY POINT              |
//! --------------------------------------------
UINTN efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, (void *)(0)); // da se ne skluka po neki cajt

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    EFI_STATUS s;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = (void *)0;
    EFI_GUID GopGuid = {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};

    s = SystemTable->BootServices->LocateProtocol(&GopGuid, (void *)0, (void **)&Gop);

    if (s != 0)
    {
        // ? DEBUG FrameBuffer -> TEXT console (ZA FINAL: OSTANKI DEBUGA ZBRIS)
        /*?
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"FB addr: ");
        PrintUint32(SystemTable, (UINT32)(Gop->Mode->FrameBufferBase >> 32));
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L" : ");
        PrintUint32(SystemTable, (UINT32)(Gop->Mode->FrameBufferBase & 0xFFFFFFFF));
        NL(SystemTable);
        */

        // TODO TERMINEREJ KER NI GRAFIKE
        SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Program ni našel grafičnega protokola GOP\r\n");
        // TODO termineraj
        while (1)
        {
        }
    }

    //? ORIGINAL DEBUG TODO ZBRIS
    // L"string"
    // SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Conways game of life\r\n");

    // PrintUint32(SystemTable, SystemTable->FirmwareRevision); // naprintej revizijo uefija

    // 64 rows * 64b per row = 4096b = 512B
    UINT64 grid[64] = {0};
    UINT64 TEMPgrid[64] = {0};
    UINT64 BACKUPgrid[64] = {0};

    UINT8 x = 0;
    UINT8 y = 0;

    EFI_TIME time;
    EFI_INPUT_KEY Key;

    int run = 1;
    int sim = 0;

    // game loop
    while (run)
    {
        s = SystemTable->RuntimeServices->GetTime(&time, (void *)0);

        s = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
        if (s == 0)
        {
            //? ZA DEBUG
            /*
            PrintUint32(SystemTable, (UINT32)Key.UnicodeChar);
            NL(SystemTable);
            PrintUint32(SystemTable, (UINT32)Key.ScanCode);
            */

            SystemTable->ConIn->Reset(SystemTable->ConIn, 0); // zbris prejsn keypress

            switch (Key.ScanCode)
            {
            case 1: // gor
                if (y > 0)
                {
                    y--;
                }
                break;
            case 2: // dol
                if (y < 63)
                {
                    y++;
                }
                break;
            case 3: // desno
                if (x < 63)
                {
                    x++;
                }
                break;
            case 4: // levo
                if (x > 0)
                {
                    x--;
                }
                break;

            case 0: // space
                switch (Key.UnicodeChar)
                {
                case 'a': // a - set
                    SetCell(grid, x, y);
                    break;

                case 's': // s - clear
                    ClearCell(grid, x, y);
                    break;

                case 'c': // clear board
                    for (int i = 0; i < 64; i++)
                    {
                        grid[i] = 0;
                    }
                    break;

                case 0x20: // SPACE (Start Simulation)
                    if (sim == 0)
                    {
                        // EDIT v SIM
                        for (int i = 0; i < 64; i++)
                        {
                            BACKUPgrid[i] = grid[i];
                        }
                        sim = 1;
                    }
                    else if (sim == 1)
                    {
                        // SIM v EDIT
                        for (int i = 0; i < 64; i++)
                        {
                            grid[i] = BACKUPgrid[i];
                        }
                        sim = 0;
                        SystemTable->BootServices->Stall(100000); // debounce ker je dual purpouse button
                    }
                    break;
                }
            }
        }

        if (sim == 1)
        {
            for (int i = 0; i < 64; i++)
            {
                TEMPgrid[i] = 0;
            }

            // novo stanje
            for (int i = 0; i < 64; i++)
            {
                for (int j = 0; j < 64; j++)
                {
                    int neighbors = CountNeighbors(grid, j, i);
                    UINT8 alive = GetCell(grid, j, i);

                    if (alive)
                    {
                        // 2 or 3 da prezivi
                        if (neighbors == 2 || neighbors == 3)
                            SetCell(TEMPgrid, j, i);
                    }
                    else
                    {
                        // tocn 3 da se rodi
                        if (neighbors == 3)
                            SetCell(TEMPgrid, j, i);
                    }
                }
            }

            // nazaj pa naris
            for (int i = 0; i < 64; i++)
            {
                grid[i] = TEMPgrid[i];
            }
        }

        PrintGrid(SystemTable, Gop, grid, x, y);

        if (sim == 1)
        {
            // to je da v simulaciji tece mal pocasnej
            SystemTable->BootServices->Stall(100000); // 0.1 s
        }
        else
        {
            SystemTable->BootServices->Stall(16666); // fps recimo
        }
    }

    //ciklej za vedno ce nekak breaka vn iz gameloopa
    while (1)
        ;
    return 0;
}