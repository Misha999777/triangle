//
// Created by Михайло Грошевий on 19/01/2025.
//

#pragma once

class Storage {
public:
    static float readAtIndex(int index);
    static void writeAtIndex(float value, int index);

private:
    static void call_flash_range_erase(void *param);
    static void call_flash_range_program(void *param);
};
