#include "supermario_task.h"
#include "user_task.h"
#include "cmsis_os.h"
#include "parameters.h"

#define SUPERMARIO_TASK_WAKE_FLAG 0x01U

static osThreadId_t s_taskHandle;
static const osThreadAttr_t s_taskAttributes = {
    .name       = "supermarioTask",
    .stack_size = 256 * 4,
    .priority   = osPriorityNormal,
};

/*
 * TIM1's update ISR (100 kHz) runs:
 *     gParameters.uAngleManual += gParameters.uIncrementManual;
 * uAngleManual is a uint16_t, so it wraps around every 65536 counts and the
 * open-loop voltage vector (angle driven by fDutyD/fDutyQ) spins at:
 *     f_out[Hz] = uIncrementManual * 100000 / 65536
 * Inverting that gives the increment needed to make the inverter "sing" a
 * given note frequency.
 */
#define FREQ_TO_INCREMENT(freqHz) ((uint16_t)(((float)(freqHz) * 65536.0f / 100000.0f) + 0.5f))

#define NOTE_REST 0

/* Standard equal-temperament note frequencies [Hz], A4 = 440 Hz */
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_E6  1319
#define NOTE_G6  1568

/*
 * Winding impedance is dominated by inductive reactance at these audio
 * frequencies (|Z| ~ 2*pi*f*Ld), so at constant voltage the current (and
 * therefore loudness) drops as frequency rises. Scale fDutyD linearly with
 * frequency, referenced to the lowest note in whichever song is playing, so
 * every note in that song draws roughly the same current and sounds equally
 * loud. Each song therefore carries its own base frequency (see Song_t).
 */
#define BASE_DUTY 0.1f

typedef struct {
    uint16_t freqHz;    /* NOTE_REST for silence */
    uint16_t durationMs;
} Note_t;

#define GAP_MS 20U /* short silence between notes so they don't slur together */

/* Opening phrase of the Super Mario Bros. "Overworld" theme (Koji Kondo) */
static const Note_t s_songMario[] = {
    { NOTE_E5,  100 }, { NOTE_E5,  100 }, { NOTE_REST, 100 }, { NOTE_E5,  100 },
    { NOTE_REST, 100 }, { NOTE_C5,  100 }, { NOTE_E5,  100 }, { NOTE_REST, 100 },
    { NOTE_G5,  100 }, { NOTE_REST, 300 }, { NOTE_G4,  100 }, { NOTE_REST, 300 },

    { NOTE_C5,  200 }, { NOTE_REST, 150 }, { NOTE_G4,  200 }, { NOTE_REST, 150 },
    { NOTE_E4,  200 }, { NOTE_REST, 150 }, { NOTE_A4,  100 }, { NOTE_B4,  100 },
    { NOTE_AS4, 100 }, { NOTE_A4,  200 },

    { NOTE_G4,  133 }, { NOTE_E5,  133 }, { NOTE_G5,  133 }, { NOTE_A5,  200 },
    { NOTE_REST, 100 }, { NOTE_F5,  100 }, { NOTE_G5,  100 }, { NOTE_REST, 100 },
    { NOTE_E5,  200 }, { NOTE_REST, 100 }, { NOTE_C5,  100 }, { NOTE_D5,  100 },
    { NOTE_B4,  200 }, { NOTE_REST, 200 },

    { NOTE_C5,  200 }, { NOTE_REST, 150 }, { NOTE_G4,  200 }, { NOTE_REST, 150 },
    { NOTE_E4,  200 }, { NOTE_REST, 150 }, { NOTE_A4,  100 }, { NOTE_B4,  100 },
    { NOTE_AS4, 100 }, { NOTE_A4,  200 },

    { NOTE_G4,  133 }, { NOTE_E5,  133 }, { NOTE_G5,  133 }, { NOTE_A5,  200 },
    { NOTE_REST, 100 }, { NOTE_F5,  100 }, { NOTE_G5,  100 }, { NOTE_REST, 100 },
    { NOTE_E5,  200 }, { NOTE_REST, 100 }, { NOTE_C5,  100 }, { NOTE_D5,  100 },
    { NOTE_B4,  400 },
};

/*
 * "Korobeiniki" (Tetris Theme A) - Russian folk melody, THE 8-bit-era tune.
 * Unlike a fingerpicked chord, this is already a single melodic line, so it
 * plays back on a one-tone-at-a-time inverter exactly as written.
 */
#define DUR_QUARTER 400U
#define DUR_EIGHTH  200U
static const Note_t s_songTetris[] = {
    { NOTE_E5, DUR_QUARTER }, { NOTE_B4, DUR_EIGHTH },  { NOTE_C5, DUR_EIGHTH },  { NOTE_D5, DUR_QUARTER }, { NOTE_C5, DUR_EIGHTH },  { NOTE_B4, DUR_EIGHTH },
    { NOTE_A4, DUR_QUARTER }, { NOTE_A4, DUR_EIGHTH },  { NOTE_C5, DUR_EIGHTH },  { NOTE_E5, DUR_QUARTER }, { NOTE_D5, DUR_EIGHTH },  { NOTE_C5, DUR_EIGHTH },
    { NOTE_B4, DUR_QUARTER }, { NOTE_C5, DUR_EIGHTH },  { NOTE_D5, DUR_QUARTER }, { NOTE_E5, DUR_QUARTER },
    { NOTE_C5, DUR_QUARTER }, { NOTE_A4, DUR_QUARTER }, { NOTE_A4, DUR_QUARTER }, { NOTE_REST, DUR_QUARTER },

    { NOTE_D5, DUR_QUARTER }, { NOTE_F5, DUR_EIGHTH },  { NOTE_A5, DUR_QUARTER }, { NOTE_G5, DUR_EIGHTH },  { NOTE_F5, DUR_EIGHTH },
    { NOTE_E5, DUR_QUARTER }, { NOTE_C5, DUR_EIGHTH },  { NOTE_E5, DUR_EIGHTH },  { NOTE_D5, DUR_EIGHTH },  { NOTE_C5, DUR_EIGHTH },
    { NOTE_B4, DUR_EIGHTH },  { NOTE_B4, DUR_EIGHTH },  { NOTE_C5, DUR_EIGHTH },  { NOTE_D5, DUR_QUARTER }, { NOTE_E5, DUR_QUARTER },
    { NOTE_C5, DUR_QUARTER }, { NOTE_A4, DUR_QUARTER }, { NOTE_A4, DUR_QUARTER },
};

#define DUR_DOTTED_EIGHTH 300U
#define DUR_SIXTEENTH     100U
#define DUR_HALF          800U

/* "The Imperial March" (Star Wars, John Williams) - opening statement */
static const Note_t s_songImperialMarch[] = {
    { NOTE_A4, DUR_QUARTER }, { NOTE_A4, DUR_QUARTER }, { NOTE_A4, DUR_QUARTER }, { NOTE_F4, DUR_DOTTED_EIGHTH }, { NOTE_C5, DUR_SIXTEENTH },
    { NOTE_A4, DUR_QUARTER }, { NOTE_F4, DUR_DOTTED_EIGHTH }, { NOTE_C5, DUR_SIXTEENTH }, { NOTE_A4, DUR_HALF },

    { NOTE_E5, DUR_QUARTER }, { NOTE_E5, DUR_QUARTER }, { NOTE_E5, DUR_QUARTER }, { NOTE_F5, DUR_DOTTED_EIGHTH }, { NOTE_C5, DUR_SIXTEENTH },
    { NOTE_GS4, DUR_QUARTER }, { NOTE_F4, DUR_DOTTED_EIGHTH }, { NOTE_C5, DUR_SIXTEENTH }, { NOTE_A4, DUR_HALF },

    { NOTE_A5, DUR_QUARTER }, { NOTE_A4, DUR_DOTTED_EIGHTH }, { NOTE_A4, DUR_SIXTEENTH }, { NOTE_A5, DUR_QUARTER },
    { NOTE_GS5, DUR_DOTTED_EIGHTH }, { NOTE_G5, DUR_SIXTEENTH }, { NOTE_FS5, DUR_DOTTED_EIGHTH }, { NOTE_F5, DUR_SIXTEENTH }, { NOTE_FS5, DUR_EIGHTH },

    { NOTE_REST, DUR_EIGHTH }, { NOTE_AS4, DUR_QUARTER }, { NOTE_DS5, DUR_DOTTED_EIGHTH }, { NOTE_D5, DUR_SIXTEENTH },
    { NOTE_CS5, DUR_QUARTER }, { NOTE_C5, DUR_DOTTED_EIGHTH }, { NOTE_AS4, DUR_SIXTEENTH }, { NOTE_C5, DUR_HALF },
};

/* Pac-Man intro jingle (Toshio Kai) - rapid staccato arpeggio + rising resolution */
static const Note_t s_songPacman[] = {
    { NOTE_B4, DUR_SIXTEENTH }, { NOTE_B5, DUR_SIXTEENTH }, { NOTE_FS5, DUR_SIXTEENTH }, { NOTE_DS5, DUR_SIXTEENTH },
    { NOTE_B5, DUR_SIXTEENTH }, { NOTE_FS5, DUR_SIXTEENTH }, { NOTE_DS5, DUR_SIXTEENTH },

    { NOTE_C5, DUR_SIXTEENTH }, { NOTE_C6, DUR_SIXTEENTH }, { NOTE_G6, DUR_SIXTEENTH }, { NOTE_E6, DUR_SIXTEENTH },
    { NOTE_C6, DUR_SIXTEENTH }, { NOTE_G6, DUR_SIXTEENTH }, { NOTE_E6, DUR_SIXTEENTH },

    { NOTE_B4, DUR_SIXTEENTH }, { NOTE_B5, DUR_SIXTEENTH }, { NOTE_FS5, DUR_SIXTEENTH }, { NOTE_DS5, DUR_SIXTEENTH },
    { NOTE_B5, DUR_SIXTEENTH }, { NOTE_FS5, DUR_SIXTEENTH }, { NOTE_DS5, DUR_SIXTEENTH },

    { NOTE_C5, DUR_EIGHTH }, { NOTE_D5, DUR_EIGHTH }, { NOTE_DS5, DUR_EIGHTH }, { NOTE_D5, DUR_QUARTER },
};

#define DUR_DOTTED_QUARTER 600U

/* "Raiders March" (Indiana Jones, John Williams) - opening fanfare */
static const Note_t s_songIndianaJones[] = {
    { NOTE_G4, DUR_EIGHTH }, { NOTE_A4, DUR_EIGHTH }, { NOTE_B4, DUR_EIGHTH }, { NOTE_C5, DUR_DOTTED_QUARTER }, { NOTE_REST, DUR_EIGHTH },
    { NOTE_G4, DUR_EIGHTH }, { NOTE_A4, DUR_EIGHTH }, { NOTE_B4, DUR_EIGHTH }, { NOTE_D5, DUR_HALF }, { NOTE_REST, DUR_EIGHTH },

    { NOTE_G4, DUR_EIGHTH }, { NOTE_A4, DUR_EIGHTH }, { NOTE_B4, DUR_EIGHTH }, { NOTE_C5, DUR_EIGHTH },
    { NOTE_D5, DUR_EIGHTH }, { NOTE_E5, DUR_EIGHTH }, { NOTE_FS5, DUR_EIGHTH },
    { NOTE_G5, DUR_QUARTER }, { NOTE_G5, DUR_QUARTER }, { NOTE_G5, DUR_HALF },
};

typedef struct {
    const Note_t *pNotes;
    uint32_t      noteCount;
    uint16_t      baseFreqHz; /* lowest note in this song; anchors DUTY_FOR_FREQ */
} Song_t;

#define SONG(arr, base) { (arr), (sizeof(arr) / sizeof((arr)[0])), (base) }

/* Indexed by uSongSelect: 0 = Super Mario Bros., 1 = Tetris Theme A,
   2 = Imperial March, 3 = Pac-Man, 4 = Indiana Jones */
static const Song_t s_songs[] = {
    SONG(s_songMario,          NOTE_E4),
    SONG(s_songTetris,         NOTE_A4),
    SONG(s_songImperialMarch,  NOTE_F4),
    SONG(s_songPacman,         NOTE_B4),
    SONG(s_songIndianaJones,   NOTE_G4),
};
#define SONG_COUNT (sizeof(s_songs) / sizeof(s_songs[0]))

static uint8_t playNote(const Note_t *pNote, uint16_t baseFreqHz)
{
    if (pNote->freqHz == NOTE_REST) {
        gParameters.fDutyD = 0.0f;
    } else {
        gParameters.uIncrementManual = FREQ_TO_INCREMENT(pNote->freqHz);
        gParameters.fDutyD = BASE_DUTY * (float)pNote->freqHz / (float)baseFreqHz;
    }

    for (uint16_t uElapsedMs = 0; uElapsedMs < pNote->durationMs; uElapsedMs++) {
        if (uSuperMario == 2) {
            return 1U;
        }
        osDelay(1);
    }

    gParameters.fDutyD = 0.0f;

    for (uint16_t uElapsedMs = 0; uElapsedMs < GAP_MS; uElapsedMs++) {
        if (uSuperMario == 2) {
            return 1U;
        }
        osDelay(1);
    }

    return 0U;
}

static void supermarioTask(void *argument)
{
    (void)argument;

    for (;;) {
        osThreadFlagsWait(SUPERMARIO_TASK_WAKE_FLAG, osFlagsWaitAny, osWaitForever);

        while (uSuperMario != 2) {
            uint8_t songIdx = (uSongSelect < SONG_COUNT) ? uSongSelect : 0U;
            const Song_t *pSong = &s_songs[songIdx];

            /* Re-checked so a live uSongSelect change switches songs immediately. */
            for (uint32_t i = 0; (i < pSong->noteCount) && (uSongSelect == songIdx); i++) {
                if (playNote(&pSong->pNotes[i], pSong->baseFreqHz)) {
                    break;
                }
            }
        }

        gParameters.fDutyD = 0.0f;
        gParameters.fDutyQ = 0.0f;
        gParameters.uIncrementManual = 0;

        uSuperMario = 0;
    }
}

void SuperMarioTask_Init(void)
{
    s_taskHandle = osThreadNew(supermarioTask, NULL, &s_taskAttributes);
}

void SuperMarioTask_Wake(void)
{
    osThreadFlagsSet(s_taskHandle, SUPERMARIO_TASK_WAKE_FLAG);
}
