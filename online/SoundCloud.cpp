/* reaDIYmate AVR library
 * Written by Pierre Bouchet
 * Copyright (C) 2011-2012 reaDIYmate
 *
 * This file is part of the reaDIYmate library.
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <SoundCloud.h>
//------------------------------------------------------------------------------
/** Size of the temporary buffer used to read file host */
uint8_t const DOWNLOAD_HOST_BUFFER_SIZE = 128;
const char PROGMEM SOUNDCLOUD_API_METHOD[] = "soundcloud/update";
const char PROGMEM SOUNDCLOUD_HOST_KEY[] = "host";
const char PROGMEM SOUNDCLOUD_NAME_KEY[] = "name";
const char PROGMEM SOUNDCLOUD_OWNER_KEY[] = "owner";
const char PROGMEM SOUNDCLOUD_URL_KEY[] = "url";
//------------------------------------------------------------------------------
/**
 * Construct an instance of SoundCloud.
 *
 * \param[in] wifly The Wifly object to use for communications
 * \param[in] buffer The Buffer to use for downloading.
 * \param[in] bufferSize The size of the buffer.
 * \param[in] host The remote host where the file is located.
 * \param[in] sd A SdFat object
 * \param[in] sdChipSelectPin The AVR pin connected to CS on the SD card.
 */
SoundCloud::SoundCloud(Api &api, Wifly &wifly, char* buffer, size_t bufferSize,
    SdFat &sd, uint8_t sdChipSelectPin, Settings &settings, PGM_P owner) :
    HttpClient(wifly),
    api_(&api),
    buffer_(buffer),
    bufferSize_(bufferSize),
    sd_(&sd),
    sdChipSelectPin_(sdChipSelectPin),
    settings_(&settings),
    owner_(owner)
{
}
//------------------------------------------------------------------------------
bool SoundCloud::download(PGM_P folder) {
    // fetch file information from API
    if (!api_->connect())
        return false;
    api_->call(SOUNDCLOUD_API_METHOD, SOUNDCLOUD_OWNER_KEY,
        settings_->getByName(owner_));
    char host[DOWNLOAD_HOST_BUFFER_SIZE] = {0};
    if (api_->getStringByName_P(SOUNDCLOUD_HOST_KEY, host,
        DOWNLOAD_HOST_BUFFER_SIZE) <= 0)
        return false;
    char filename[16] = {0};
    if (api_->getStringByName_P(SOUNDCLOUD_NAME_KEY, filename, 16) <= 0)
        return false;
    char filepath[32] = {0};
    snprintf(filepath, 32, "%S/%s", folder, filename);
    char path[256] = {0};
    if (api_->getStringByName_P(SOUNDCLOUD_URL_KEY, path, 256) <= 0)
        return false;

    Serial.println(host);
    Serial.println(filepath);
    Serial.println(path);

    // connect to file host
    if (!connect(host))
        return false;

    // fetch content length and compute the number of pieces
    uint32_t fileSize = getContentLength(buffer_, bufferSize_, host, path);
    if (fileSize == 0)
        return false;

    uint16_t nbPieces = (fileSize - 1)/bufferSize_ + 1;

    // create the local file
    if (!sd_->init(SPI_EIGHTH_SPEED, sdChipSelectPin_)) {
        sd_->initErrorHalt();
        return false;
    }
    if (exists(filepath))
        remove(this, filepath);
    if (!open(filepath, O_CREAT | O_WRITE)) {
        return false;
    }

    // download the file piece by piece
    uint32_t firstByte, lastByte;
    for (uint32_t i = 0; i < nbPieces; i++) {
        firstByte = i * bufferSize_;
        if (i < nbPieces - 1)
            lastByte = firstByte + bufferSize_ - 1;
        else if (i == nbPieces - 1)
            lastByte = fileSize - 1;
        int nAttempts = 0;
        while (!getRange(buffer_, bufferSize_, host, path, firstByte,
            lastByte)) {
            // soundcloud link probably expired
            if (++nAttempts > 3) {
                // clear path buffer
                memset(path, 0x00, 256);
                // fetch a new link to the file
                if (!api_->connect())
                    return false;
                api_->call(SOUNDCLOUD_API_METHOD, SOUNDCLOUD_OWNER_KEY,
                    settings_->getByName(owner_));
                api_->getStringByName_P(SOUNDCLOUD_URL_KEY, path, 256);
                Serial.println(path);
                // connect to file host
                if (!connect(host))
                    return false;
                nAttempts = 0;
            }
        }
        write(buffer_, lastByte - firstByte + 1);
        sync();
        Serial.println(fileSize - lastByte - 1);
    }

    // close the file
    close();

    return true;
}