//
// Created by valdemar on 11.05.23.
//
#include "userspace_api.h"

#include <stdio.h>
#include <memory.h>
#include <errno.h>

#define EMAIL "vova@melnikov.name"

int main() {
    struct UserPersonalData data;
    memcpy(data.name, "Vova", 5);
    memcpy(data.surname, "Vova", 5);
    memcpy(data.ph_number, "+79154824464", 13);
    memcpy(data.email, EMAIL, sizeof(EMAIL));
    data.age=20;

    add_user(&data);
    struct UserPersonalData another;

    memcpy(data.name, "Biba", 5);
    memcpy(data.surname, "Biba", 5);
    memcpy(data.ph_number, "+79154824464", 13);
    memcpy(data.email, "anon", sizeof("anon"));
    int ret = add_user(&data);
    if (ret) {
        return ret;
    }

    ret = get_user("Biba", 5, &another);

    if (ret) {
        return ret;
    }


    another.email[63] = 0;

    printf("%s\n",
           another.email
           );
    return 0;
}