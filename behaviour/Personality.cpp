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
#include <Personality.h>
#define DEBUG
//------------------------------------------------------------------------------
// Timing constants
/** Refresh interval (in ms) to check Gmail. */
const uint32_t CHECK_GMAIL_INTERVAL = 480000;
/** Refresh interval (in ms) to check Facebook. */
const uint32_t CHECK_FACEBOOK_INTERVAL = 480000;
/** Refresh interval (in ms) to check Twitter. */
const uint32_t CHECK_TWITTER_INTERVAL = 480000;
/** Refresh interval (in ms) to check Rss. */
const uint32_t CHECK_RSS_INTERVAL = 480000;
/** Refresh interval (in ms) to check Foursquare. */
const uint32_t CHECK_FOURSQUARE_INTERVAL = 480000;
/** Refresh interval (in ms) to check SoundCloud. */
const uint32_t CHECK_SOUNDCLOUD_INTERVAL = 3600000;
const uint8_t MAX_LEVEL = 1;
//------------------------------------------------------------------------------
/** Construct an instance of the Personality FSM */
Personality::Personality(Api &api, StatusLed &led, Inbox &inbox,
    ServoControl &control, PusherTrajectory &realtime) :
    api_(&api),
    led_(&led),
    inbox_(&inbox),
    control_(&control),
    realtime_(&realtime)
{
    internalTransition(Personality::wakingUp);
}
//------------------------------------------------------------------------------
void Personality::initialize() {
    resetDeadlines();
    transition(Personality::enteringPushMode);
}
//------------------------------------------------------------------------------
/** Action */
void Personality::action(const Event* e) {
    switch (e->signal) {
        case ENTRY :
#ifdef DEBUG
            Serial.println(F("Personality::action"));
#endif
            led_->colorOrange();
            inbox_->leavePushMode();
            break;
        case STOP :
            transition(Personality::enteringPushMode);
            break;
        case SOUNDCLOUD :
            transition(Personality::playingSoundCloud);
            break;
    }
}
//------------------------------------------------------------------------------
/** Asleep */
void Personality::asleep(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::asleep"));
            break;
#endif
        case SUPERLONG_CLICK_ARMED :
            transition(Personality::wakingUp);
            emit(WAKE_UP);
            break;
    }
}
//------------------------------------------------------------------------------
/** Regular active mode */
void Personality::awake(const Event* e) {
    switch (e->signal) {
        case ENTRY :
#ifdef DEBUG
            Serial.println(F("Personality::awake"));
#endif
            led_->colorGreen();
            break;
        case SHORT_CLICK_RELEASED :
            transition(Personality::action);
            emit(ACTION);
            break;
        case SUPERLONG_CLICK_ARMED :
            transition(Personality::fallingAsleep);
            emit(FALL_ASLEEP);
            break;
        case TICK :
            if (millis() >= checkingFacebookDeadline_
            || millis() >= checkingGmailDeadline_
            || millis() >= checkingTwitterDeadline_
            || millis() >= checkingRssDeadline_
            || millis() >= checkingFoursquareDeadline_
            || millis() >= checkingSoundCloudDeadline_) {
                transition(Personality::checkingServices);
            }
            else {
                int messageType = inbox_->getMessage();
                if (messageType == INBOX_START_REMOTE) {
                    control_->begin(realtime_);
                    transition(Personality::remoteControl);
                }
#ifdef DEBUG
                else if (messageType == INBOX_PING) {
                    Serial.println(F("Ping event"));
                }
#endif
            }
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking Facebook */
void Personality::checkingFacebook(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::checkingFacebook"));
            break;
#endif
        case STOP :
            transition(Personality::checkingServices);
            checkingFacebookDeadline_ = millis() + CHECK_FACEBOOK_INTERVAL;
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking Foursquare*/
void Personality::checkingFoursquare(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::checkingFoursquare"));
            break;
#endif
        case STOP :
            transition(Personality::checkingServices);
            checkingFoursquareDeadline_ = millis() + CHECK_FOURSQUARE_INTERVAL;
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking Gmail */
void Personality::checkingGmail(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::checkingGmail"));
            break;
#endif
        case STOP :
            transition(Personality::checkingServices);
            checkingGmailDeadline_ = millis() + CHECK_GMAIL_INTERVAL;
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking Rss*/
void Personality::checkingRss(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::checkingRss"));
            break;
#endif
        case STOP :
            transition(Personality::checkingServices);
            checkingRssDeadline_ = millis() + CHECK_RSS_INTERVAL;
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking services */
void Personality::checkingServices(const Event* e) {
    switch (e->signal) {
        case ENTRY :
#ifdef DEBUG
            Serial.println(F("Personality::checkingServices"));
#endif
            led_->colorOrange();
            inbox_->leavePushMode();
            break;
        case TICK :
            if (millis() >= checkingFacebookDeadline_) {
                emit(FACEBOOK);
                transition(Personality::checkingFacebook);
            }
            else if (millis() >= checkingGmailDeadline_) {
                emit(GMAIL);
                transition(Personality::checkingGmail);
            }
            else if (millis() >= checkingTwitterDeadline_) {
                emit(TWITTER);
                transition(Personality::checkingTwitter);
            }
            else if (millis() >= checkingRssDeadline_) {
                emit(RSS);
                transition(Personality::checkingRss);
            }
            else if (millis() >= checkingFoursquareDeadline_) {
                emit(FOURSQUARE);
                transition(Personality::checkingFoursquare);
            }
            else if (millis() >= checkingSoundCloudDeadline_) {
                emit(SOUNDCLOUD);
                transition(Personality::checkingSoundCloud);
            }
            else {
                transition(Personality::enteringPushMode);
            }
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking SoundCloud*/
void Personality::checkingSoundCloud(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::checkingSoundcloud"));
            break;
#endif
        case STOP :
            transition(Personality::checkingServices);
            checkingSoundCloudDeadline_ = millis() + CHECK_SOUNDCLOUD_INTERVAL;
            break;
        case SOUNDCLOUD :
            transition(Personality::playingSoundCloud);
            break;
    }
}
//------------------------------------------------------------------------------
/** Checking Twitter */
void Personality::checkingTwitter(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::checkingTwitter"));
            break;
#endif
        case STOP :
            transition(Personality::checkingServices);
            checkingTwitterDeadline_ = millis() + CHECK_TWITTER_INTERVAL;
            break;
    }
}
//------------------------------------------------------------------------------
void Personality::enteringPushMode(const Event* e) {
    switch (e->signal) {
        case ENTRY :
#ifdef DEBUG
            Serial.println(F("Personality::enteringPushMode"));
#endif
            led_->colorOrange();
            break;
        case TICK :
            if (inbox_->enterPushMode()) {
                transition(Personality::awake);
            }
            else {
                inbox_->leavePushMode();
                led_->colorRed();
                internalTransition(Personality::enteringPushMode);
            }
            break;
    }
}
//------------------------------------------------------------------------------
/** Substate when performing an action before going back to sleep */
void Personality::fallingAsleep(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::fallingAsleep"));
            break;
#endif
        case STOP :
            transition(Personality::asleep);
            led_->colorNothing();
            inbox_->leavePushMode();
            break;
    }
}
//------------------------------------------------------------------------------
/** Playing a sound from SoundCloud*/
void Personality::playingSoundCloud(const Event* e) {
    switch (e->signal) {
#ifdef DEBUG
        case ENTRY :
            Serial.println(F("Personality::playingSoundcloud"));
            break;
#endif
        case STOP :
            transition(Personality::enteringPushMode);
            break;
        case EXIT :
            resetDeadlines();
            break;
    }
}
//------------------------------------------------------------------------------
/** Remote control mode */
void Personality::remoteControl(const Event* e) {
    switch (e->signal) {
        case ENTRY :
#ifdef DEBUG
            Serial.println(F("Personality::remoteControl"));
            led_->colorOrange();
#endif
            break;
        case TICK :
            control_->startNextMotion(millis());
            if (control_->finishedTrajectory()) {
                transition(Personality::awake);
            }
            else if (control_->finishedStep()) {
                control_->startNextStep();
            }
            break;
        case SHORT_CLICK_RELEASED :
            transition(Personality::awake);
            break;
        case SUPERLONG_CLICK_ARMED :
            transition(Personality::fallingAsleep);
            emit(FALL_ASLEEP);
            break;
        case EXIT :
            resetDeadlines();
            break;
    }
}
//------------------------------------------------------------------------------
/** Substate when performing a user-triggered action */
void Personality::wakingUp(const Event* e) {
    switch (e->signal) {
        case ENTRY :
#ifdef DEBUG
            Serial.println(F("Personality::wakingUp"));
#endif
            led_->colorOrange();
            break;
        case STOP :
            transition(Personality::enteringPushMode);
            break;
        case SHORT_CLICK_RELEASED :
            transition(Personality::enteringPushMode);
            emit(STOP);
            break;
        case EXIT :
            inbox_->updatePusherChannel();
            break;
    }
}
//------------------------------------------------------------------------------
/** Reset the timers for all recurring actions */
void Personality::resetDeadlines() {
    checkingGmailDeadline_ = millis() + CHECK_GMAIL_INTERVAL;
    checkingFacebookDeadline_ = millis() + CHECK_FACEBOOK_INTERVAL;
    checkingTwitterDeadline_ = millis() + CHECK_TWITTER_INTERVAL;
    checkingRssDeadline_ = millis() + CHECK_RSS_INTERVAL;
    checkingFoursquareDeadline_ = millis() + CHECK_FOURSQUARE_INTERVAL;
    checkingSoundCloudDeadline_ = millis() + CHECK_SOUNDCLOUD_INTERVAL;
}
