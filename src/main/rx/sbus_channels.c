/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "platform.h"

#ifdef USE_SERIAL_RX

#include "common/utils.h"
#include "common/maths.h"

#include "build/debug.h"

#include "rx/sbus_channels.h"


#define SBUS_DIGITAL_CHANNEL_MIN 173
#define SBUS_DIGITAL_CHANNEL_MAX 1812

STATIC_ASSERT(SBUS_FRAME_SIZE == sizeof(sbusFrame_t), SBUS_FRAME_SIZE_doesnt_match_sbusFrame_t);

uint8_t sbus26ChannelsDecode(rxRuntimeConfig_t *rxRuntimeConfig, const sbusChannels_t *channels, bool highChannels)
{
    uint8_t offset = highChannels ? 16 : 0;
    uint16_t *sbusChannelData = rxRuntimeConfig->channelData;
    sbusChannelData[0 + offset] = channels->chan0;
    sbusChannelData[1 + offset] = channels->chan1;
    sbusChannelData[2 + offset] = channels->chan2;
    sbusChannelData[3 + offset] = channels->chan3;
    sbusChannelData[4 + offset] = channels->chan4;
    sbusChannelData[5 + offset] = channels->chan5;
    sbusChannelData[6 + offset] = channels->chan6;
    sbusChannelData[7 + offset] = channels->chan7;
    sbusChannelData[8 + offset] = channels->chan8;
    sbusChannelData[9 + offset] = channels->chan9;
    sbusChannelData[10 + offset] = channels->chan10;
    sbusChannelData[11 + offset] = channels->chan11;
    sbusChannelData[12 + offset] = channels->chan12;
    sbusChannelData[13 + offset] = channels->chan13;
    sbusChannelData[14 + offset] = channels->chan14;
    sbusChannelData[15 + offset] = channels->chan15;

    if (!highChannels) {
        if (channels->flags & SBUS_FLAG_CHANNEL_DG1) {
            sbusChannelData[32] = SBUS_DIGITAL_CHANNEL_MAX;
        } else {
            sbusChannelData[32] = SBUS_DIGITAL_CHANNEL_MIN;
        }

        if (channels->flags & SBUS_FLAG_CHANNEL_DG2) {
            sbusChannelData[33] = SBUS_DIGITAL_CHANNEL_MAX;
        } else {
            sbusChannelData[33] = SBUS_DIGITAL_CHANNEL_MIN;
        }
    }

    if (channels->flags & SBUS_FLAG_FAILSAFE_ACTIVE) {
        // internal failsafe enabled and rx failsafe flag set
        // RX *should* still be sending valid channel data, so use it.
        return RX_FRAME_COMPLETE | RX_FRAME_FAILSAFE;
    }

    if (channels->flags & SBUS_FLAG_SIGNAL_LOSS) {
        // The received data is a repeat of the last valid data so can be considered complete.
        return RX_FRAME_COMPLETE | RX_FRAME_DROPPED;
    }

    return RX_FRAME_COMPLETE;
}

uint8_t sbusChannelsDecode(rxRuntimeConfig_t *rxRuntimeConfig, const sbusChannels_t *channels)
{
    uint16_t *sbusChannelData = rxRuntimeConfig->channelData;
    sbusChannelData[0] = channels->chan0;
    sbusChannelData[1] = channels->chan1;
    sbusChannelData[2] = channels->chan2;
    sbusChannelData[3] = channels->chan3;
    sbusChannelData[4] = channels->chan4;
    sbusChannelData[5] = channels->chan5;
    sbusChannelData[6] = channels->chan6;
    sbusChannelData[7] = channels->chan7;
    sbusChannelData[8] = channels->chan8;
    sbusChannelData[9] = channels->chan9;
    sbusChannelData[10] = channels->chan10;
    sbusChannelData[11] = channels->chan11;
    sbusChannelData[12] = channels->chan12;
    sbusChannelData[13] = channels->chan13;
    sbusChannelData[14] = channels->chan14;
    sbusChannelData[15] = channels->chan15;

    if (channels->flags & SBUS_FLAG_CHANNEL_DG1) {
        sbusChannelData[16] = SBUS_DIGITAL_CHANNEL_MAX;
    } else {
        sbusChannelData[16] = SBUS_DIGITAL_CHANNEL_MIN;
    }

    if (channels->flags & SBUS_FLAG_CHANNEL_DG2) {
        sbusChannelData[17] = SBUS_DIGITAL_CHANNEL_MAX;
    } else {
        sbusChannelData[17] = SBUS_DIGITAL_CHANNEL_MIN;
    }

    if (channels->flags & SBUS_FLAG_FAILSAFE_ACTIVE) {
        // internal failsafe enabled and rx failsafe flag set
        // RX *should* still be sending valid channel data, so use it.
        return RX_FRAME_COMPLETE | RX_FRAME_FAILSAFE;
    }

    if (channels->flags & SBUS_FLAG_SIGNAL_LOSS) {
        // The received data is a repeat of the last valid data so can be considered complete.
        return RX_FRAME_COMPLETE | RX_FRAME_DROPPED;
    }

    return RX_FRAME_COMPLETE;
}

uint16_t sbusDecodeChannelValue(uint16_t sbusValue, bool safeValuesOnly)
{
    // Linear fitting values read from OpenTX-ppmus and comparing with values received by X4R
    // http://www.wolframalpha.com/input/?i=linear+fit+%7B173%2C+988%7D%2C+%7B1812%2C+2012%7D%2C+%7B993%2C+1500%7D
    if (safeValuesOnly) {
        // Clip channel values to more or less safe values (988 .. 2012)
        return (5 * constrain(sbusValue, SBUS_DIGITAL_CHANNEL_MIN, SBUS_DIGITAL_CHANNEL_MAX) / 8) + 880;
    }
    else {
        // Use full range of values (11 bit, channel values in range 880 .. 2159)
        return (5 * constrain(sbusValue, 0, 2047) / 8) + 880;
    }
}

uint16_t sbusEncodeChannelValue(uint16_t rcValue)
{
    return constrain((((int)rcValue - 880) * 8 + 4) / 5, SBUS_DIGITAL_CHANNEL_MIN, SBUS_DIGITAL_CHANNEL_MAX);
}

static uint16_t sbusChannelsReadRawRC(const rxRuntimeConfig_t *rxRuntimeConfig, uint8_t chan)
{
    return sbusDecodeChannelValue(rxRuntimeConfig->channelData[chan], false);
}

void sbusChannelsInit(rxRuntimeConfig_t *rxRuntimeConfig)
{
    rxRuntimeConfig->rcReadRawFn = sbusChannelsReadRawRC;
    for (int b = 0; b < SBUS_MAX_CHANNEL; b++) {
        rxRuntimeConfig->channelData[b] = (16 * PWM_RANGE_MIDDLE) / 10 - 1408;
    }
}
#endif
