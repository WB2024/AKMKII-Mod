
#include "pinyin_util.h"

int DDE_WCSLEN(const unsigned short *s)
{
	const unsigned short *save;

	if (s == 0)
		return 0;
	for (save = s; *save; ++save);
	return save-s;
}
void usTochar(char* a, unsigned short* b)
{
	int i=0;
	for(i=0;i<DDE_WCSLEN(b);i++)
		a[i] = b[i];
}
void uscpy(unsigned short* a, unsigned short* b)
{
	int j=0;
	for (j = 0; b[j] != 0x0; j++)
		a[j] = b[j];
}