#ifndef VIDEO_H
#define VIDEO_H

#include "datapacker.h"

#include <cstdint>

/* What the renderer is up to, counted as it goes and written to the log
   every few seconds - see Video::noteFrame. Ordinary builds do not take
   the counts at all:

       make VIDEO_STATS=1

   A macro rather than a constexpr flag because the core builds at C++14,
   where nothing but the preprocessor gets the counting statements out of
   the compiled code altogether. What it counts lives in the class either
   way - see the note over Stats - so that the shape of a Video never
   depends on how the file looking at it was configured.
*/
#ifndef VIDEO_STATS
#define VIDEO_STATS 0
#endif

class Video
{
public:
    static constexpr uint32_t FRAMEBUFFER_WIDTH = 320;
    static constexpr uint32_t FRAMEBUFFER_HEIGHT = 224;
    static constexpr float ASPECT_RATIO = 4.0f / 3.0f;
    static constexpr uint16_t MAX_SPRITES_PER_SCREEN = 381;
    static constexpr uint16_t MAX_SPRITES_PER_LINE = 96;

    // Used to clip sprites
    static constexpr uint32_t LEFT_BORDER = 160 - (FRAMEBUFFER_WIDTH / 2);
    static constexpr uint32_t RIGHT_BORDER = (FRAMEBUFFER_WIDTH / 2) + 159;

    enum HirqControl
    {
        HIRQ_CTRL_DISABLE     = 0x00,
        HIRQ_CTRL_ENABLE      = 0x10,
        HIRQ_CTRL_RELATIVE    = 0x20,
        HIRQ_CTRL_VBLANK_LOAD = 0x40,
        HIRQ_CTRL_AUTOREPEAT  = 0x80
    };

    Video();
    ~Video();
    
    // Non copyable
    Video(const Video&) = delete;

    // Non copyable
    Video& operator=(const Video&) = delete;

    void reset();

    void convertColor(uint32_t index);
    void convertPalette();

    void updateFixUsageMap();

    void drawFix(uint32_t scanline);

    uint16_t renderScanlineSprites(uint32_t scanline, uint16_t *spriteList);
    void rebuildSpriteIndex();

    /* Which sprites can touch which line, worked out once and reused
       until the sprite attributes change. Everything in the resolved
       arrays is line-invariant - the walk over the bank computes the
       same positions and sizes for every line - so it is done once and
       each line takes its own list, in bank order, capped at the
       chip's per line limit. Derived from video RAM, rebuilt on load,
       never saved.
    */
    bool     spriteIndexDirty = true;
    uint16_t resolvedX[MAX_SPRITES_PER_SCREEN + 1];
    uint16_t resolvedY[MAX_SPRITES_PER_SCREEN + 1];
    uint8_t  resolvedZoomX[MAX_SPRITES_PER_SCREEN + 1];
    uint8_t  resolvedZoomY[MAX_SPRITES_PER_SCREEN + 1];
    uint8_t  resolvedClipping[MAX_SPRITES_PER_SCREEN + 1];
    uint16_t lineSprites[224][MAX_SPRITES_PER_LINE];
    uint8_t  lineSpriteCount[224];
    void drawSprite(uint32_t spriteNumber,
                    uint32_t x,
                    uint32_t y,
                    uint32_t zoomX,
                    uint32_t zoomY,
                    uint32_t scanline,
                    uint32_t clipping);
    void drawBlackLine(uint32_t scanline);
    void drawEmptyLine(uint32_t scanline);

    /* One frame done: keeps the worst-of-the-run figures and reports the
       counts to the log every so often. Called from the frame loop of
       NeoGeoCD in a statistics build, and doing nothing at all otherwise.
    */
    void noteFrame();

    friend DataPacker& operator<<(DataPacker& out, const Video& video);
    friend DataPacker& operator>>(DataPacker& in, Video& video);

    uint16_t* paletteRamPc;
    uint8_t* fixUsageMap;
    uint16_t* frameBuffer;

    // Variables to save in savestate
    uint32_t activePaletteBank;
    uint32_t autoAnimationCounter;
    uint32_t autoAnimationSpeed;
    uint32_t autoAnimationFrameCounter;
    bool     autoAnimationDisabled;
    bool     sprDisable;
    bool     fixDisable;
    bool     videoEnable;

    /* The whole screen darkening latch behind REG_SHADOW. Deliberately
       not part of the saved state: adding it would change the state
       size and turn away every state saved before it existed, for one
       bit a game sets again the next time it wants the screen dark.
       A state loaded mid shadow shows full brightness until then.
    */
    bool     shadow = false;
    uint32_t hirqControl;
    uint32_t hirqRegister;
    uint32_t videoramOffset;
    uint32_t videoramModulo;
    uint32_t videoramData;
    uint32_t sprite_x;
    uint32_t sprite_y;
    uint32_t sprite_zoomX;
    uint32_t sprite_zoomY;
    uint32_t sprite_clipping;
    // End variables to save in savestate

    /* Counts of the work the renderer does, taken at the point the work
       happens, so that each costs one increment in a statistics build and
       nothing outside one. They say what this session drew rather than
       what the console holds, so they stay out of the saved state.

       There whether the counting is compiled in or not, deliberately: the
       flag changes what the code does, never what a Video is, so that a
       core half built with one setting and half with the other cannot
       disagree about where anything lives.
    */
    struct Stats
    {
        uint64_t frames;              // frames since the last report
        uint64_t lines;               // scanlines whose object list was collected
        uint64_t rebuilds;            // walks of the whole sprite bank
        uint64_t rebuildsThisFrame;   // walks since the frame began
        uint64_t worstRebuilds;       // most walks in any one frame since the report
        uint64_t bucketEntries;       // (object, line) pairs the line lists hold
        uint64_t objectLines;         // calls that put object pixels on the screen
        uint64_t objectLinesOff;      // calls dismissed as off screen before drawing
        uint64_t objectPixels;        // object pixels put down, opaque or not
        uint64_t fixCells;            // text characters drawn, once per line of a character
        uint64_t fixCellsSkipped;     // ... and those left out, being all of one colour
    } stats = {};
};

DataPacker& operator<<(DataPacker& out, const Video& video);
DataPacker& operator>>(DataPacker& in, Video& video);

#endif // VIDEO_H
