//
// Created by thomas on 26.06.24.
//

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
