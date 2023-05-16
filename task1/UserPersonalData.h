 
#ifndef PERSONAL_DATA_H
#define PERSONAL_DATA_H
 


#include <linux/types.h> 


struct UserPersonalData {
    __u32 age;
    char name[64];
    char surname[64];
    char ph_number[16];
    char email[321];
};

struct StoreFormat {
    struct UserPersonalData data;
    __u8 tombstone;
};

enum RequestType {
    Read = 0,
    Add = 1,
    Delete = 2,
    FindBySurname = 3,
};

struct DataRequest {
    char surname[64];
    struct UserPersonalData data;
};
//{
//    UserPersonalData data;
//    __u32 id;
//};

typedef struct DataRequest RemoveRequest;
typedef struct DataRequest ReadRequest;
typedef struct UserPersonalData AddRequest;


#endif