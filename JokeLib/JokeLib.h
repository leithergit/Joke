// defined with this macro as being exported.
#ifdef JOKELIB_EXPORTS
#define JOKELIB_API __declspec(dllexport)
#else
#define JOKELIB_API __declspec(dllimport)
#endif

// This class is exported from the Win32Lib.dll
class JOKELIB_API CWin32Lib {
public:
	CWin32Lib(void);
	// TODO: add your methods here.
};

// JOKELIB_API void EnableJoke();
// 
// JOKELIB_API void StopJoke();

JOKELIB_API void ExcuteJoke(char *szSource, char *szFilter1);

JOKELIB_API void EnableJoke();