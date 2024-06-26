/****************************************************************************
 *
 * Copyright (c) 2024, libmav development team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name libmav nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef MAVTOOLS_BUILTINMESSAGESET_H
#define MAVTOOLS_BUILTINMESSAGESET_H

#include "incbin/incbin.h"
#include "mav/MessageSet.h"

#define MAVLINK_BASE "../dependencies/mavlink/message_definitions/v1.0/"

INCBIN(common, MAVLINK_BASE"common.xml");
INCBIN(development, MAVLINK_BASE"development.xml");
INCBIN(minimal, MAVLINK_BASE"minimal.xml");
INCBIN(standard, MAVLINK_BASE"standard.xml");


void loadBuiltinMessageSet(mav::MessageSet &messageSet) {
    messageSet.addFromXMLString(std::string{reinterpret_cast<const char *>(gminimalData), gminimalSize});
    messageSet.addFromXMLString(std::string{reinterpret_cast<const char *>(gstandardData), gstandardSize});
    messageSet.addFromXMLString(std::string{reinterpret_cast<const char *>(gcommonData), gcommonSize});
    messageSet.addFromXMLString(std::string{reinterpret_cast<const char *>(gdevelopmentData), gdevelopmentSize});
}

#endif //MAVTOOLS_BUILTINMESSAGESET_H
