#if !defined(LIBRETRO_COMMON_H)
#define LIBRETRO_COMMON_H

#include <string>
#include <vector>

#include "bios.h"
#include "libretro.h"

class NeoGeoCD;

struct LibretroCallbacks
{
    retro_log_printf_t log{ nullptr };
    retro_video_refresh_t video{ nullptr };
    retro_input_poll_t inputPoll{ nullptr };
    retro_input_state_t inputState{ nullptr };
    retro_environment_t environment{ nullptr };
    retro_audio_sample_batch_t audioBatch{ nullptr };
    retro_perf_callback perf{ nullptr };
};

struct BiosListEntry
{
    std::string filename;
    std::string description;
    Bios::Type type;
};

namespace AspectRatio
{
    static constexpr uint32_t PAR_1_1 = 0;
    static constexpr uint32_t PAR_45_44 = 1;
    static constexpr uint32_t DAR_4_3 = 2;
};

struct Globals
{
    // Version number of the notification interface
    unsigned messageInterfaceVersion{ 0 };

    // Retroarch's system directory
    const char* systemDirectory{ nullptr };

    // Retroarch's save directory
    const char* saveDirectory{ nullptr };

    // Path to the srm file
    std::string srmFilename;

    // List of all BIOSes found
    std::vector<BiosListEntry> biosList;

    // Index of currently selected BIOS
    size_t biosIndex{ 0 };

    // Description of all BIOSes separated by |
    std::string biosChoices;

    // Should we skip CD loading?
    bool skipCDLoading{ true };

    // Should we patch the BIOS to lower CPU usage during loading
    bool cdSpeedHack{ false };

    // Pixels of horizontal overscan hidden on each side of the picture
    uint32_t overscanH{ 8 };

    // 68000 overclock, in percent of the stock clock. 100 is stock.
    uint32_t cpuOverclock{ 100 };

    // If true will create a memory card file for each game
    bool perContentSaves{ false };

    // Aspect ratio setting (0 = 1:1 PAR, 1 = 45:44 PAR, 2 = 4:3 DAR)
    uint32_t aspectRatio{ AspectRatio::PAR_1_1 };
};

extern LibretroCallbacks libretro;

extern NeoGeoCD* neocd;

extern Globals globals;

#endif // LIBRETRO_COMMON_H
