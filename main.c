/**
 * @file main.c
 * @author Locha Mesh Developers (contact@locha.io)
 * @brief Main firmware file
 * @version 0.1
 * @date 2020-02-02
 * 
 * @copyright Copyright (c) 2020 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 * 
 */

#include <stdio.h>

#include "banner.h"

int main(void)
{
    printf("%s", banner);
    puts("Welcome to Turpial CC1312 Radio!");

    return 0;
}
