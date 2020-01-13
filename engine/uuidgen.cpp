extern "C"
{
#ifdef WIN32
#include <Rpc.h>
#else
#include <uuid/uuid.h>
#endif
}

void newUUID(char str[37])
{
#ifdef WIN32
    UUID uuid;
    UuidCreate ( &uuid );
    unsigned char *strout;
    UuidToStringA ( &uuid, &strout );
    strcpy(str,(const char *)strout);
    RpcStringFreeA ( &strout );
#else
    uuid_t uuid;
    uuid_generate_random ( uuid );
    uuid_unparse ( uuid, str );
#endif
}
