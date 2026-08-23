#include <cstdlib>
#include <vector>

#include "libretro_bios.h"
#include "libretro_common.h"
#include "libretro_variables.h"
#include "libretro.h"
#include "neogeocd.h"
#include "video.h"

// Variable names and descriptions for the settings
static constexpr const char* VARIABLE_REGION = "neocd_region";
static constexpr const char* DESC_REGION = "Console Region";
static constexpr const char* VARIABLE_BIOS = "neocd_bios";
static constexpr const char* DESC_BIOS = "BIOS Select";
static constexpr const char* VARIABLE_SPEEDHACK = "neocd_cdspeedhack";
static constexpr const char* DESC_SPEEDHACK = "CD Speed Hack";
static constexpr const char* VARIABLE_LOADSKIP = "neocd_loadskip";
static constexpr const char* DESC_LOADSKIP = "Skip CD Loading";
static constexpr const char* VARIABLE_PER_CONTENT_SAVES = "neocd_per_content_saves";
static constexpr const char* DESC_PER_CONTENT_SAVES = "Per-Game Saves (Restart)";
static constexpr const char* VARIABLE_ASPECT_RATIO = "neocd_aspect_ratio";
static constexpr const char* DESC_ASPECT_RATIO = "Aspect Ratio";
static constexpr const char* VARIABLE_OVERSCAN_H = "neocd_overscan_h";
static constexpr const char* DESC_OVERSCAN_H = "Horizontal Overscan Mask";
static constexpr const char* VARIABLE_CPU_OVERCLOCK = "neocd_cpu_overclock";
static constexpr const char* DESC_CPU_OVERCLOCK = "CPU Overclock";

// Categories
static constexpr const char* CATEGORY_SYSTEM = "system";
static constexpr const char* CATEGORY_VIDEO = "video";
// static constexpr const char* CATEGORY_AUDIO = "audio";
// static constexpr const char* CATEGORY_INPUT = "input";
static constexpr const char* CATEGORY_ADVANCED = "advanced";

// Values
static constexpr const char* VALUE_ON = "On";
static constexpr const char* VALUE_OFF = "Off";
static constexpr const char* VALUE_JAPAN = "Japan";
static constexpr const char* VALUE_USA = "USA";
static constexpr const char* VALUE_EUROPE = "Europe";
static constexpr const char* VALUE_1_1_PAR = "1:1 PAR";
static constexpr const char* VALUE_45_44_PAR = "45:44 PAR";
static constexpr const char* VALUE_4_3_DAR = "4:3 DAR";

static constexpr std::initializer_list<const char*> VALUES_ONOFF{ VALUE_ON, VALUE_OFF };

// All core variables
static std::vector<retro_variable> variables;
static std::vector<retro_core_option_v2_definition> coreOptionDefinitions;

static retro_core_option_v2_category coreOptionCategories[] = {
    { CATEGORY_SYSTEM, "System", nullptr },
    { CATEGORY_VIDEO, "Video", nullptr },
    //{ CATEGORY_AUDIO, "Audio", nullptr },
    //{ CATEGORY_INPUT, "Input", nullptr },
    { CATEGORY_ADVANCED, "Advanced", nullptr },
    { nullptr, nullptr, nullptr },
};

static retro_core_options_v2 coreOptionsV2 = { coreOptionCategories, nullptr };

// Helper functions
static inline bool string_is(const char* str, const char* value) { return (strcmp(str, value) == 0); }

static inline bool string_to_bool(const char* str) { return string_is(str, VALUE_ON) ? true : false; }

static inline uint32_t string_to_region(const char* str)
{
    if (string_is(str, VALUE_USA))
        return NeoGeoCD::NationalityUSA;

    if (string_is(str, VALUE_EUROPE))
        return NeoGeoCD::NationalityEurope;

    return NeoGeoCD::NationalityJapan;
}

static inline uint32_t string_to_aspect_ratio(const char* str)
{
    if (string_is(str, VALUE_45_44_PAR))
        return AspectRatio::PAR_45_44;

    if (string_is(str, VALUE_4_3_DAR))
        return AspectRatio::DAR_4_3;

    return AspectRatio::PAR_1_1;
}

static void buildBiosChoices()
{
    globals.biosChoices.clear();

    if (globals.biosList.empty())
        return;

    globals.biosChoices = "BIOS Select; ";
    globals.biosChoices.append(globals.biosList[0].description);

    for (size_t i = 1; i < globals.biosList.size(); ++i)
    {
        globals.biosChoices.append("|");
        globals.biosChoices.append(globals.biosList[i].description);
    }
}

static void fillBasicOption(retro_core_option_v2_definition& option,
                            const char* key,
                            const char* desc,
                            const char* categoryKey,
                            const char* defaultValue,
                            const std::initializer_list<const char*>& values)
{
    option = retro_core_option_v2_definition{};
    option.key = key;
    option.desc = desc;
    option.desc_categorized = desc;
    option.category_key = categoryKey;

    const size_t maxValues = RETRO_NUM_CORE_OPTION_VALUES_MAX - 1;
    const size_t valueCount = values.size();
    const size_t count = valueCount > maxValues ? maxValues : valueCount;
    const auto pValues = values.begin();

    for (size_t i = 0; i < count; ++i)
    {
        option.values[i].value = pValues[i];
        option.values[i].label = pValues[i];
    }

    option.values[count].value = NULL;
    option.values[count].label = NULL;
    option.default_value = defaultValue;
}

static void fillBiosOption(retro_core_option_v2_definition& option)
{
    option = retro_core_option_v2_definition{};
    option.key = VARIABLE_BIOS;
    option.desc = DESC_BIOS;
    option.desc_categorized = DESC_BIOS;
    option.category_key = CATEGORY_SYSTEM;

    size_t count = globals.biosList.size();
    const size_t maxValues = RETRO_NUM_CORE_OPTION_VALUES_MAX - 1;
    if (count > maxValues)
        count = maxValues;

    for (size_t i = 0; i < count; ++i)
    {
        const char* desc = globals.biosList[i].description.c_str();
        option.values[i].value = desc;
        option.values[i].label = desc;
    }

    option.values[count].value = NULL;
    option.values[count].label = NULL;
    option.default_value = count ? option.values[0].value : NULL;
}

static void buildLegacyVariables()
{
    variables.clear();

    variables.emplace_back(retro_variable{ VARIABLE_REGION, "Region; Japan|USA|Europe" });

    buildBiosChoices();

    if (globals.biosList.size())
        variables.emplace_back(retro_variable{ VARIABLE_BIOS, globals.biosChoices.c_str() });

    variables.emplace_back(retro_variable{ VARIABLE_OVERSCAN_H, "Horizontal Overscan Mask; 8|4|0|12|16" });
    variables.emplace_back(retro_variable{ VARIABLE_SPEEDHACK, "CD Speed Hack; On|Off" });
    variables.emplace_back(retro_variable{ VARIABLE_CPU_OVERCLOCK, "CPU Overclock; 100%|110%|125%|150%|200%" });
    variables.emplace_back(retro_variable{ VARIABLE_LOADSKIP, "Skip CD Loading; On|Off" });
    variables.emplace_back(retro_variable{ VARIABLE_PER_CONTENT_SAVES, "Per-Game Saves (Restart); Off|On" });
    variables.emplace_back(retro_variable{ VARIABLE_ASPECT_RATIO, "Aspect Ratio; 1:1 PAR|45:44 PAR|4:3 DAR" });

    variables.emplace_back(retro_variable{ nullptr, nullptr });
}

static void buildCoreOptionsV2()
{
    coreOptionDefinitions.clear();
    coreOptionDefinitions.reserve(8);

    retro_core_option_v2_definition option;

    fillBasicOption(option, VARIABLE_REGION, DESC_REGION, CATEGORY_SYSTEM, VALUE_JAPAN, { VALUE_JAPAN, VALUE_USA, VALUE_EUROPE });
    coreOptionDefinitions.emplace_back(option);

    if (!globals.biosList.empty())
    {
        fillBiosOption(option);
        coreOptionDefinitions.emplace_back(option);
    }

    fillBasicOption(option, VARIABLE_OVERSCAN_H, DESC_OVERSCAN_H, CATEGORY_VIDEO, "8", { "8", "4", "0", "12", "16" });
    coreOptionDefinitions.emplace_back(option);

    // OFF by default because I am not certain this has no undesirable side effects
    fillBasicOption(option, VARIABLE_SPEEDHACK, DESC_SPEEDHACK, CATEGORY_ADVANCED, VALUE_OFF, VALUES_ONOFF);
    coreOptionDefinitions.emplace_back(option);

    fillBasicOption(option, VARIABLE_LOADSKIP, DESC_LOADSKIP, CATEGORY_ADVANCED, VALUE_ON, VALUES_ONOFF);
    coreOptionDefinitions.emplace_back(option);

    fillBasicOption(option, VARIABLE_CPU_OVERCLOCK, DESC_CPU_OVERCLOCK, CATEGORY_ADVANCED, "100%", { "100%", "110%", "125%", "150%", "200%" });
    coreOptionDefinitions.emplace_back(option);

    fillBasicOption(option, VARIABLE_PER_CONTENT_SAVES, DESC_PER_CONTENT_SAVES, CATEGORY_SYSTEM, VALUE_OFF, VALUES_ONOFF);
    coreOptionDefinitions.emplace_back(option);

    fillBasicOption(option, VARIABLE_ASPECT_RATIO, DESC_ASPECT_RATIO, CATEGORY_VIDEO, VALUE_1_1_PAR, { VALUE_1_1_PAR, VALUE_45_44_PAR, VALUE_4_3_DAR });
    coreOptionDefinitions.emplace_back(option);

    coreOptionDefinitions.emplace_back(retro_core_option_v2_definition{});
    coreOptionsV2.definitions = coreOptionDefinitions.data();
}

void Libretro::Variables::init()
{
    unsigned optionsVersion = 0;
    bool setOptions = false;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &optionsVersion) && optionsVersion >= 2)
    {
        buildCoreOptionsV2();
        setOptions = libretro.environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &coreOptionsV2);
    }

    if (!setOptions)
    {
        buildLegacyVariables();
        libretro.environment(RETRO_ENVIRONMENT_SET_VARIABLES, reinterpret_cast<void*>(&variables[0]));
    }
}

void Libretro::Variables::update(bool needReset)
{
    struct retro_variable var;

    var.value = NULL;
    var.key = VARIABLE_REGION;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        const auto nationality = string_to_region(var.value);

        if (neocd->machineNationality != nationality)
        {
            neocd->machineNationality = nationality;
            needReset = true;
        }
    }

    var.value = NULL;
    var.key = VARIABLE_BIOS;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        size_t index = Libretro::Bios::descriptionToIndex(var.value);

        if (index != globals.biosIndex)
        {
            globals.biosIndex = index;
            Libretro::Bios::load();
            needReset = true;
        }
    }

    var.value = NULL;
    var.key = VARIABLE_OVERSCAN_H;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        uint32_t newValue = static_cast<uint32_t>(atoi(var.value));

        if (newValue > 16)
            newValue = 16;

        if (globals.overscanH != newValue)
        {
            globals.overscanH = newValue;

            struct retro_system_av_info avinfo;
            retro_get_system_av_info(&avinfo);
            libretro.environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &avinfo);
        }
    }

    var.value = NULL;
    var.key = VARIABLE_SPEEDHACK;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        const auto newValue = string_to_bool(var.value);

        if (globals.cdSpeedHack != newValue)
        {
            globals.cdSpeedHack = newValue;
            Libretro::Bios::load();
            needReset = true;
        }
    }

    var.value = NULL;
    var.key = VARIABLE_LOADSKIP;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        globals.skipCDLoading = string_to_bool(var.value);

    var.value = NULL;
    var.key = VARIABLE_CPU_OVERCLOCK;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        uint32_t newValue = (uint32_t)atoi(var.value);
        if (newValue >= 100 && newValue <= 1000)
            globals.cpuOverclock = newValue;
    }

    var.value = NULL;
    var.key = VARIABLE_PER_CONTENT_SAVES;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        globals.perContentSaves = string_to_bool(var.value);

    var.value = NULL;
    var.key = VARIABLE_ASPECT_RATIO;

    if (libretro.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        const auto aspectRatio = string_to_aspect_ratio(var.value);

        if (globals.aspectRatio != aspectRatio)
        {
            globals.aspectRatio = aspectRatio;

            struct retro_system_av_info avinfo;
            retro_get_system_av_info(&avinfo);
            libretro.environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &avinfo);
        }
    }

    if (needReset)
        neocd->reset();
}
