//
// Created by Михайло Грошевий on 21/01/2025.
//

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <string>
#include <functional>

#include <btstack.h>

class Communication {
public:
    static void init();
    static void setCallback(const std::function<void(int, float)> &newCallback);
    static void setCurrentValue(const std::string& newValue);
    static void sendNotification(const std::string& value);

private:
    static uint16_t attReadCallback(hci_con_handle_t, uint16_t, uint16_t, uint8_t*, uint16_t);
    static int attWriteCallback(hci_con_handle_t, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t);

    static inline hci_con_handle_t notificationHandle;
    static inline std::function<void(int, float)> callback;
    static inline std::string currentValue;
};

#endif //COMMUNICATION_H
