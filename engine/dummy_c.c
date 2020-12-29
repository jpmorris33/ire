//
//  Editor stubs of C functions
//

#ifdef USE_FMOD
#ifdef _WIN32
int Waves=0;
int Songs=0;
#endif
#endif

int T_IsDue()
{
return 1;
}

#ifdef USE_FMOD
#ifdef _WIN32
int S_Init()
{
return 1;
}

void S_Term()
{
}
#endif
#endif
