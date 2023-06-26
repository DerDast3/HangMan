#define RNDNR(max) (((rand() ^ ((int)(GetTickCount()))) & 0x7fffffff) % max)

extern LPWORD Worte[];
extern BYTE palmem[sizeof(LOGPALETTE)+260*sizeof(PALETTEENTRY)];
extern LPLOGPALETTE mypallette;
extern HPALETTE hPal;

int WortCount();
LPWORD GimmeOne();
void BuildPalette();

