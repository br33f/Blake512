#include <iostream>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <iomanip>
#include <math.h>

using namespace std;

//zmienne globalne
vector <uint64_t> msgBlock; //message block
uint64_t h[8]; //hash
uint64_t s[4]; //salt
uint64_t t[2]; //counter
uint64_t vBlock[16]; //v
int runda = 0;

//constrants - skopiowane z oficjalnej dokumentacji https://131002.net/blake/blake.pdf
static const uint64_t constrants[16] = {
	0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL,
	0xA4093822299F31D0ULL, 0x082EFA98EC4E6C89ULL,
	0x452821E638D01377ULL, 0xBE5466CF34E90C6CULL,
	0xC0AC29B7C97C50DDULL, 0x3F84D5B5B5470917ULL,
	0x9216D5D98979FB1BULL, 0xD1310BA698DFB5ACULL,
	0x2FFD72DBD01ADFB7ULL, 0xB8E1AFED6A267E96ULL,
	0xBA7C9045F12C7F99ULL, 0x24A19947B3916CF7ULL,
	0x0801F2E2858EFC16ULL, 0x636920D871574E69ULL
};

//sigma - skopiowane z oficjalnej dokumentacji https://131002.net/blake/blake.pdf
static const unsigned char sigma[10][16] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
		{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
		{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
		{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
		{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
};

//deklaracje funkcji
void padding(int, int);
void finalizuj(void);
void wypiszBinarnie(uint64_t);
uint64_t rotacja(uint64_t, uint64_t);
void G(int, int, int, int, int, int);
void inicjalizacja(void);
void wykonaj(string, int);

//implementacje funkcji
void wypiszBinarnie(uint64_t p)
{
	int pomocnicza[64];
	for (int z = 0; z < 64; z++)
	{
		if (p % 2 == 1) pomocnicza[z] = 1;
		else pomocnicza[z] = 0;
		p = p >> 1;
	}
	for (int z = 63; z >= 0; z--)
		cout << pomocnicza[z];
}
uint64_t rotacja(uint64_t x, uint64_t i)
{
	uint64_t p = ((x)<<(64-i)) | ((x)>>i);
	return p;
}

void G(int a, int b, int c, int d, int i, int offset)
{
	vBlock[a] = vBlock[a] + vBlock[b] + (msgBlock[offset + sigma[runda % 10][i]] ^ constrants[sigma[runda % 10][i + 1]]);
	vBlock[d] = rotacja((vBlock[d] ^ vBlock[a]), 32);
	vBlock[c] = vBlock[c] + vBlock[d];
	vBlock[b] = rotacja((vBlock[b] ^ vBlock[c]), 25);
	vBlock[a] = vBlock[a] + vBlock[b] + (msgBlock[offset + sigma[runda % 10][i + 1]] ^ constrants[sigma[runda % 10][i]]);
	vBlock[d] = rotacja((vBlock[d] ^ vBlock[a]), 16);
	vBlock[c] = vBlock[c] + vBlock[d];
	vBlock[b] = rotacja((vBlock[b] ^ vBlock[c]), 11);
}

void inicjalizacja(void)
{
	//hash ustawiam na wartosc poczatkowa
	h[0] = 0x6A09E667F3BCC908ULL;
	h[1] = 0xBB67AE8584CAA73BULL;
	h[2] = 0x3C6EF372FE94F82BULL,
	h[3] = 0xA54FF53A5F1D36F1ULL;
	h[4] = 0x510E527FADE682D1ULL;
	h[5] = 0x9B05688C2B3E6C1FULL;
	h[6] = 0x1F83D9ABFB41BD6BULL;
	h[7] = 0x5BE0CD19137E2179ULL;

	//salt ustawiam na 0
	for (int i = 0; i < 4; i++)
		s[i] = 0;

	//counter ustawiam na 0
	t[0] = 0;
	t[1] = 0;
}

void wykonaj(string msg, int msgBitLength)
{
	//rozdzielenie wiadomosci na bloki
	int ilosc = (int)ceil(msg.length() / (double)16.0);
	int blockCount = 0;
	int p = msgBitLength;
	while (p > 0)
	{
		blockCount += 16;
		for (int i = 0; i < 16; i++)
			msgBlock.push_back(0);
		p -= 1024;
	}
	if ((msgBitLength % 1024) >(1024 - 128 - 2) || msgBitLength % 1024 == 0)
	{
		blockCount += 16;
		for (int i = 0; i < 16; i++)
			msgBlock.push_back(0);
	}
	for (int i = 0; i < ilosc; i++)
	{
		string w = msg.substr(i * 16, 16);
		msgBlock[i] = strtoull(w.c_str(), NULL, 16);
		msgBlock[i] = msgBlock[i] << (64 - w.length()*4);
	}
	for (int i = ilosc; i < blockCount; i++)
		msgBlock[i] = 0x0;

	padding(msgBitLength, blockCount);

	int pom_dlugosc = msgBitLength;
	for (int its = 0; its < blockCount / 16; its++)
	{
		if (pom_dlugosc > 1024)
		{
			t[0] += 1024;
			pom_dlugosc -= 1024;
		}
		else if (pom_dlugosc > 894)
		{
			t[0] += pom_dlugosc;
			pom_dlugosc = 0;
		}
		else if (pom_dlugosc == 0)
			t[0] = 0;
		else t[0] += pom_dlugosc;

		//ustawienie V
		for (int i = 0; i < 8; i++)
			vBlock[i] = h[i];

		vBlock[8] = s[0] ^ constrants[0];
		vBlock[9] = s[1] ^ constrants[1];
		vBlock[10] = s[2] ^ constrants[2];
		vBlock[11] = s[3] ^ constrants[3];
		vBlock[12] = t[0] ^ constrants[4];
		vBlock[13] = t[0] ^ constrants[5];
		vBlock[14] = t[1] ^ constrants[6];
		vBlock[15] = t[1] ^ constrants[7];

		for (runda = 0; runda < 16; ++runda)
		{
			//rundy
			/* column step */
			G(0, 4, 8, 12, 0, 16*its);
			G(1, 5, 9, 13, 2, 16 * its);
			G(2, 6, 10, 14, 4, 16 * its);
			G(3, 7, 11, 15, 6, 16 * its);

			/* diagonal step */
			G(0, 5, 10, 15, 8, 16 * its);
			G(1, 6, 11, 12, 10, 16 * its);
			G(2, 7, 8, 13, 12, 16 * its);
			G(3, 4, 9, 14, 14, 16 * its);
		}

		finalizuj();
	}
	for (int i = 0; i < 8; i++)
	{
		cout << setfill('0') << setw(16) << hex << h[i];
	}
}
void padding(int msgBitLength, int blockCount)
{
	int p_blok = (int)floor(msgBitLength / 64);
	int pozycja = msgBitLength % 64;
	uint64_t maska = 0x8000000000000000 >> pozycja;
	if (msgBitLength <= (1024 - 128 - 2))
	{
		msgBlock[p_blok] |= maska;
		msgBlock[13] |= 0x1;
		msgBlock[14] = 0;
		msgBlock[15] = msgBitLength;
	}
	else if(msgBitLength > (1024 - 128 - 2))
	{
		msgBlock[p_blok] |= maska;
		msgBlock[blockCount - 3] |= 0x1;
		msgBlock[blockCount - 2] = 0;
		msgBlock[blockCount - 1] = msgBitLength;
	}
}

void finalizuj(void)
{
	for (int i = 0; i < 16; i++)
		h[i%8] ^= vBlock[i];
	for (int i = 0; i < 8; i++)
		h[i] ^= s[i % 4];
}
int main(int argc, const char* argv[])
{
	inicjalizacja();

	string msg = "";
	if (argc == 2)
		msg = string(argv[1]);

	int msgBitLength = 0;
	msgBitLength = msg.length() * 4;

	wykonaj(msg, msgBitLength);

	return 0;
}
