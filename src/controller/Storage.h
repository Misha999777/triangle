//
// Created by Михайло Грошевий on 19/01/2025.
//

#ifndef STORAGE_H
#define STORAGE_H

class Storage {
public:
    static float readAtIndex(int index);
    static void writeAtIndex(float value, int index);

private:
    static void call_flash_range_erase(void *param);
    static void call_flash_range_program(void *param);
};

#endif //STORAGE_H
