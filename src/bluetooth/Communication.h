//
// Created by Михайло Грошевий on 21/01/2025.
//

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <functional>

#include <btstack.h>

class Communication {
public:
    static void init();
    static void setCallback(const std::function<int(const char*)> &newCallback);
    static void sendNotification(const char* value);

private:
    static int attWriteCallback(hci_con_handle_t, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t);

    static inline hci_con_handle_t notificationHandle;
    static inline std::function<int(const char*)> callback;
};

#endif //COMMUNICATION_H
