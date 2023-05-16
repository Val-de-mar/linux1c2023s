
#ifndef CHARDEV_USERSPACE_API_H
#define CHARDEV_USERSPACE_API_H

#include "UserPersonalData.h"
#include <asm/errno.h>

long get_user(const char* surname, unsigned int len, struct UserPersonalData* output_data);


long add_user(struct UserPersonalData* input_data);

/*do not copy if output_data == NULL*/
long del_user(const char* surname, unsigned int len, struct UserPersonalData* output_data);


#endif //CHARDEV_USERSPACE_API_H
