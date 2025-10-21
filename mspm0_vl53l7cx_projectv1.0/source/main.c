#include "ti_msp_dl_config.h"
#include "vl53l7cx_api.h"
#include "vl53l7cx_i2c_if.h"
#include "vl53l7cx_uart_if.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* ---------------- SysTick millis() ---------------- */
static volatile uint32_t g_ms = 0;
void SysTick_Handler(void) { g_ms++; }
static inline uint32_t millis(void) { return g_ms; }

/* ---------------- Parameters ---------------- */
#define RES                       VL53L7CX_RESOLUTION_4X4   /* 16 zones */
#define HUMAN_MIN_DIST            200   /* mm */
#define HUMAN_MAX_DIST            3000  /* mm */
#define CLUSTER_TOLERANCE_NEAR    200   /* mm */
#define CLUSTER_TOLERANCE_FAR     100   /* mm */

#define Z1_MAX                    1000  /* mm */
#define Z2_MAX                    2000
#define Z3_MAX                    3000
#define HYST_MM                   100

#define MISSING_TIMEOUT_MS        3000
#define PRESENCE_GRACE_MS         1000

#define BASELINE_FRAMES           20
#define MOTION_THRESH_MM          60
#define BASELINE_ALPHA            0.02f

/* ---------------- Baseline buffers ---------------- */
static uint16_t baseline[64];
static bool     base_valid[64];
static uint8_t  base_frames = 0;
static bool     baseline_ready = false;

/* ---------------- Helpers ---------------- */
struct Cluster { long sumDist; int count; };
static inline uint8_t zone_from_dist(int d){
    if (d < Z1_MAX) return 0;
    else if (d < Z2_MAX) return 1;
    else if (d < Z3_MAX) return 2;
    else return 255;
}
static inline uint8_t zone_with_hyst(uint8_t prev, int d){
    uint8_t base = zone_from_dist(d);
    if (prev == 255 || base == 255) return base;
    if (prev != base){
        if ( (d > (Z1_MAX - HYST_MM)) && (d < (Z1_MAX + HYST_MM)) ) return prev;
        if ( (d > (Z2_MAX - HYST_MM)) && (d < (Z2_MAX + HYST_MM)) ) return prev;
    }
    return base;
}

/* ---------------- Session state ---------------- */
static bool     sessionActive    = false;
static uint32_t sessionStartMs   = 0;
static uint32_t lastSeenMs       = 0;      /* last time presence==true */
static uint32_t lastFrameMs      = 0;      /* last processed frame time */

static uint8_t  curZone          = 255;    /* 0..2 valid */
static uint32_t curZoneEnterMs   = 0;

static bool     zoneEntered[3]       = {false,false,false};
static uint32_t zoneFirstEntryMs[3]  = {0,0,0};  /* offset from session start */
static uint32_t zoneDwellMs[3]       = {0,0,0};

static void reset_session(void){
    sessionActive   = false;
    sessionStartMs  = 0;
    lastSeenMs      = 0;
    lastFrameMs     = 0;
    curZone         = 255;
    curZoneEnterMs  = 0;
    for (int z=0; z<3; z++){
        zoneEntered[z]      = false;
        zoneFirstEntryMs[z] = 0;
        zoneDwellMs[z]      = 0;
    }
}

static void send_session_summary(uint8_t finalZone1Based){
    /* Sum of exclusive zone dwells */
    uint32_t total_ms = zoneDwellMs[0] + zoneDwellMs[1] + zoneDwellMs[2];

    char json[256];
    /* Build JSON similar to Arduino version */
    int n = snprintf(json, sizeof(json),
        "{\"event\":\"session_end\",\"total_ms\":%lu,\"final_zone\":%u,\"zones\":[",
        (unsigned long)total_ms, (unsigned)finalZone1Based);

    for (int i=0; i<3; i++){
        int k = snprintf(json + n, sizeof(json) - n,
            "%s{\"idx\":%d,\"entered\":%d,\"first_entry_ms\":%lu,\"dwell_ms\":%lu}",
            (i ? "," : ""), i+1, zoneEntered[i] ? 1 : 0,
            (unsigned long)(zoneEntered[i] ? zoneFirstEntryMs[i] : 0),
            (unsigned long)zoneDwellMs[i]);
        if (k < 0) k = 0;
        n += k;
        if (n >= (int)sizeof(json)) break;
    }
    if (n < (int)sizeof(json)) {
        int k = snprintf(json + n, sizeof(json) - n, "],\"conversionEvent\":0}");
        if (k > 0) n += k;
    }

    /* Send to UART1 (ESP32) and UART0 (debug) */
    Uart1_Write((uint8_t*)json, (uint32_t)strlen(json));
    Uart1_Write((uint8_t*)"\r\n", 2);
    Uart0_Print(json);
    Uart0_Print("\r\n");
}

static void close_session_and_send(uint8_t finalZone1Based){
    uint32_t now = millis();

    /* Final slice within grace window */
    if (curZone < 3 && lastFrameMs > 0 && (now - lastSeenMs) <= PRESENCE_GRACE_MS){
        zoneDwellMs[curZone] += (now - lastFrameMs);
    }

    send_session_summary(finalZone1Based);
    reset_session();
}

/* ---------------- Main ---------------- */
int main(void)
{
    /* Generated init (clocks, pins, I2C0, UART0/1) */
    SYSCFG_DL_init();

    /* 1ms SysTick @ 32 MHz */
    SysTick->LOAD  = (CPUCLK_FREQ / 1000U) - 1U;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    /* Power up sensor & bring out of LP */
    VL_I2C_Init();
    VL_PowerEnable(true);
    VL_LowPower(false);
    VL_I2C_ResetPulse();

    /* ULD sensor object/config */
    VL53L7CX_Configuration sensor;
    memset(&sensor, 0, sizeof(sensor));

    /* Init sensor FW and basic config */
    (void)vl53l7cx_init(&sensor);
    (void)vl53l7cx_set_ranging_frequency_hz(&sensor, 5);
    (void)vl53l7cx_set_resolution(&sensor, RES);
    (void)vl53l7cx_start_ranging(&sensor);

    /* Baseline init */
    for (int i=0;i<64;i++){ baseline[i]=0; base_valid[i]=false; }
    base_frames=0; baseline_ready=false;
    reset_session();

    Uart0_Print("MSPM0 basic v7 port: continuous dwell + grace window (exclusive zone, no double-count)\r\n");

    /* --------- Loop --------- */
    for (;;)
    {
        VL53L7CX_ResultsData R;
        uint8_t ready = 0;

        /* Poll data-ready (blocking) */
        while (vl53l7cx_check_data_ready(&sensor, &ready) || !ready) { /* spin */ }

        if (!vl53l7cx_get_ranging_data(&sensor, &R)) {
            uint32_t now = millis();

            /* ---- Baseline learning (no sessions during learning) ---- */
            if (!baseline_ready){
                for (int i=0; i<RES; i++){
                    if (R.nb_target_detected[i] <= 0) continue;
                    int d = R.distance_mm[i];
                    if (d < HUMAN_MIN_DIST || d > HUMAN_MAX_DIST) continue;
                    if (!base_valid[i]){
                        baseline[i] = (uint16_t)d;
                        base_valid[i] = true;
                    } else {
                        baseline[i] = (uint16_t)((((uint32_t)baseline[i])*base_frames + (uint32_t)d)
                                                  / (uint32_t)(base_frames+1));
                    }
                }
                base_frames++;
                if (base_frames >= BASELINE_FRAMES){
                    baseline_ready = true;
                    Uart0_Print("Baseline ready.\r\n");
                }
                continue;
            }

            /* ---- Build dynamic clusters ---- */
            struct Cluster clusters[64];
            int cc = 0;
            for (int i=0; i<RES; i++){
                if (R.nb_target_detected[i] <= 0) continue;
                int d = R.distance_mm[i];
                if (d < HUMAN_MIN_DIST || d > HUMAN_MAX_DIST) continue;

                bool isDyn = true;
                if (base_valid[i]){
                    int diff = d - (int)baseline[i];
                    if (diff < 0) diff = -diff;
                    if (diff < MOTION_THRESH_MM){
                        /* low-pass baseline */
                        baseline[i] = (uint16_t)((1.0f - BASELINE_ALPHA) * baseline[i] + BASELINE_ALPHA * d);
                        isDyn = false;
                    }
                }
                if (!isDyn) continue;

                int tol = (d >= Z2_MAX ? CLUSTER_TOLERANCE_FAR : CLUSTER_TOLERANCE_NEAR);
                bool added = false;
                for (int c=0; c<cc; c++){
                    int avg = (clusters[c].count > 0) ? (int)(clusters[c].sumDist / clusters[c].count) : d;
                    int adiff = d - avg; if (adiff < 0) adiff = -adiff;
                    if (adiff <= tol){
                        clusters[c].sumDist += d;
                        clusters[c].count++;
                        added = true;
                        break;
                    }
                }
                if (!added && cc < 64){
                    clusters[cc].sumDist = d;
                    clusters[cc].count   = 1;
                    cc++;
                }
            }

            /* ---- Presence & exclusive nearest selection ---- */
            int bestDist = 0x3FFFFFFF;
            bool haveRep = false;
            for (int c=0; c<cc; c++){
                if (clusters[c].count < 2) continue; /* ignore tiny blobs */
                int avg = (int)(clusters[c].sumDist / clusters[c].count);
                if (avg < bestDist){ bestDist = avg; haveRep = true; }
            }
            bool presence = haveRep;

            /* ---- Session lifecycle ---- */
            if (presence){
                lastSeenMs = now;

                if (!sessionActive){
                    sessionActive  = true;
                    sessionStartMs = now;
                    curZone        = 255;
                    curZoneEnterMs = 0;
                    lastFrameMs    = now; /* start dwell from next frame delta */
                }

                uint8_t decidedZone = 255;
                if (haveRep) decidedZone = zone_with_hyst(curZone, bestDist);

                if (decidedZone < 3){
                    /* Add dwell to current zone for this frame delta */
                    if (curZone < 3 && lastFrameMs > 0){
                        zoneDwellMs[curZone] += (now - lastFrameMs);
                    }

                    /* Handle transitions */
                    if (curZone != decidedZone){
                        curZone        = decidedZone;
                        curZoneEnterMs = now;
                        if (!zoneEntered[curZone]){
                            zoneEntered[curZone]      = true;
                            zoneFirstEntryMs[curZone] = now - sessionStartMs;
                        }
                    }
                }
            } else {
                /* No presence this frame */
                if (sessionActive){
                    if (curZone < 3 && lastFrameMs > 0 && (now - lastSeenMs) <= PRESENCE_GRACE_MS){
                        zoneDwellMs[curZone] += (now - lastFrameMs);
                    }
                    if ((now - lastSeenMs) > MISSING_TIMEOUT_MS){
                        uint8_t finalZone1Based = (curZone < 3) ? (curZone+1) : 0;
                        close_session_and_send(finalZone1Based);
                        lastFrameMs = now;
                        continue;
                    }
                }
            }

            /* frame timestamp update */
            lastFrameMs = now;
        }
    }
}
