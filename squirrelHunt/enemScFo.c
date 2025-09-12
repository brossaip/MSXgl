/* Vull provar de fer el joc squirrel eat squirrel del pygame desenvolupat en el
llibre "Making Games with Python & Pygame"
En aquesta primera part només vull controlar que apareguin enemics a la zona
fora del joc i que hi vagin entrant i després que es vagin movent amb l'scroll.
El programa original té un mapa infinit, però nosaltres utilitzarem de moment el
mateix que hem fet les proves de 64x64. A l'original hi ha una zona
d'aproximitat que és on es creen els personatges. Nosaltres també la tindrem,
però haurà de ser adaptativa per quan arribi als extrems. Tal i com comenten
aquí
https://stackoverflow.com/questions/17354905/sdcc-and-malloc-allocating-much-less-memory-than-is-available
nosaltres no tindrem memòria dinàmica perquè no tenim un sistema operatiu
multiprocés que comparteix la memòria. Ho executem en exclusivitat, per tant fem
un nombre de concret de conills que vagin apareixent. Puc emmegatzemar 256
patrons però visualitzar 32 sprites. Si els grossos  es formen per 4 sprites, ho
he de tenir en compte. Però de moment comencem només visualitzant-os i movent un
senzill 2024-04-15 Faré més senzill que els esquirols corrent pel mapa. També
per començar a debugar aniria bé tenir-ne pocs, dos o quatre. Quan desapareguin
de la càmera ja els borro de l'array. Quan els genero hauré de tenir en compte
l'scroll per posar-los. Podran aparèixer als marges. Trauré un número del 0 al 3
per si va a dalt, baix, dreta, esquerra i després un altre per la posició x,y
segons el que hagi sortit
 */
/* Ho he passat a MSXgl */

#include "dos.h"
#include "memory.h"
#include "msxgl.h"
#include "string.h"
#include "tool/reg.h"

// File handling will be done differently in MSXgl

u8 g_StrBuffer[128];
unsigned char map_tile_x; // Per indicar la rajola de dalt a l'esquerra. Potser es pot fusionar amb tile_esq. No es pot fusionar, ja que tile_esq és la posició actual del personatge
unsigned char map_tile_y;
unsigned char x;
unsigned char y;

DOS_FCB g_File;

typedef struct{
  char x; // Les posicions dins la càmera. S'inicialitzaran el primer cop que apareguin
  char y;
  signed char speed_x;
  signed char speed_y;
  unsigned char tamany;
  unsigned char eliminar; // Si s'ha d'eliminar el creem de nou
  unsigned char pintar; // Si està en el camp de visió de la càmera.
  unsigned char num_pla_sprite;
} struct_esquirol;

#define NUM_ESQUIROLS 5

struct_esquirol esquirols[NUM_ESQUIROLS];

#define ACT_KEY_A 64 // Línia 2
#define ACT_KEY_D 2  // Línia 3
#define ACT_KEY_W 16 // Línia 5
#define ACT_KEY_S 1 // Línia 5
#define OFFSET_COORDENADAY_PAGINA_ACTIVA_1 256
#define OFFSET_COORDENADAY_PAGINA_ACTIVA_2 256 * 2
// A on comença la pàgina 2 de l'screen 5. Comtpador començant per 0
#define OFFSET_COORDENADAY_PAGINA_ACTIVA_3 256 * 3
// A on comença la pàgina 3 de l'screen 5. Comtpador començant per 0
#define NOMBRE_RAJOLES_HOR 64
// Nombre de rajoles que conté el mapa
#define NOMBRE_RAJOLES_VER 64
#define NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS 32 // Rajoles a pintar en una linia horitzontal
#define NOMBRE_RAJOLES_PANTALLA_HOR 31
#define NOMBRE_RAJOLES_PANTALLA_VER 28 // Aquestes són les que es veuen en pantalla
#define NOMBRE_RAJOLES_PANTALLA_VER_SCROLL 32 // Aquesta són les de tota la pàgina que es fa l'scroll
// En total només visualitzo 31, ja que la 32 queda amagada quan es fa l'scroll. Però el faré més curt, de 26. Controlar les altres 5ho trobava complicat quan es barrejava amb scroll vertical
#define AMPLADA_CAMERA 256 // Per calcular l'offset quan desapareixen els conills de la càmera
#define ALCADA_CAMERA 212

// L'he fet de 50 columnes però per tema de simplificar càlculs hauria de ser potència de 2
const char map1[] = {
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,0,1,2,36,35,36,35,36,35,36,35,36,35,0,2,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,5,4,3,4,3,4,3,4,3,4,3,4,32,33,34,4,3,4,3,4,3,4,3,4,3,32,34,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,5,4,3,4,3,4,3,6,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,5,35,36,35,36,35,36,35,36,35,36,64,65,66,36,35,36,35,36,35,36,35,36,35,64,66,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,96,97,98,4,3,4,3,4,3,4,3,4,3,96,98,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,0,1,2,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,32,33,34,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,6,3,4,3,4,3,4,3,4,3,0,1,2,3,4,3,4,3,4,3,4,3,4,3,4,3,64,65,66,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,32,33,34,35,36,35,36,35,36,35,36,35,36,35,36,35,96,97,98,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,0,1,2,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,6,3,4,
35,36,35,36,35,36,32,33,34,36,35,36,35,36,35,36,35,5,35,36,35,36,35,36,35,36,35,36,35,36,35,0,1,2,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,64,65,66,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,32,33,34,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,5,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,96,97,98,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,96,97,98,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,5,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,0,1,2,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,32,33,34,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,5,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,96,97,98,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,5,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,0,1,2,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,6,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,32,33,34,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,96,97,98,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,0,1,2,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,5,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,32,33,34,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,64,65,66,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,96,97,98,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,5,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,0,1,2,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,0,1,2,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,32,33,34,36,35,36,35,36,35,36,35,5,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,32,33,34,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,96,97,98,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,64,65,66,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,96,97,98,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,6,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,0,2,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,0,1,2,36,35,36,35,36,35,36,35,36,35,36,35,36,35,32,34,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,32,33,34,4,3,4,3,4,3,4,3,4,3,4,3,4,3,64,66,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,96,97,98,36,35,36,35,36,35,36,35,36,35,36,35,36,35,96,98,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,6,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,6,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,6,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,6,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,0,1,2,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,32,33,34,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,64,65,66,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,96,97,98,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,5,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,5,4,3,4,3,6,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,0,1,2,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,32,33,34,3,4,3,4,3,4,3,4,5,4,3,4,3,6,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,0,1,2,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,64,65,66,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,32,33,34,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,96,97,98,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,96,97,98,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,0,2,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,
35,36,32,34,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,
3,4,64,66,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,0,1,2,3,4,3,4,3,4,3,4,3,4,
35,36,96,98,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,32,33,34,35,36,35,36,35,36,35,36,35,36,
3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,4,3,96,97,98,3,4,3,4,3,4,3,4,3,4,
35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36,35,36
};

static const unsigned char sprite[] = {
  0b00111000,
  0b01000100,
  0b00101000,
  0b00010000,
  0b00111000,
  0b01111100,
  0b01111110,
  0b11111111
};

static const unsigned char esquirol[] = {
  0b11000110,
  0b11100111,
  0b11111111,
  0b01110111,
  0b01110010,
  0b01110000,
  0b00000000,
  0b00000000  
};

#define MAX_PLANS_SPRITE 30

typedef struct {
  char data[MAX_PLANS_SPRITE]; // Array to store elements
  char top;            // Index of the top element
} Stack;

Stack pila_plans_sprite;

void push(Stack *stack, char value) {
  stack->top++;
  stack->data[stack->top] = value;
}

char pop(Stack *stack) {
  char val_ret = stack->data[stack->top];
  stack->top --;
  return val_ret;
}


unsigned long num_llarg_aleatori = 2463534242;

void rand_xor_init() {
  // Use BIOS jiffy counter for random seed
  num_llarg_aleatori = (g_JIFFY << 15) | (unsigned long)g_JIFFY;
}

int rand_xor() {
  num_llarg_aleatori = num_llarg_aleatori ^ (num_llarg_aleatori << 13);
  num_llarg_aleatori = num_llarg_aleatori ^ (num_llarg_aleatori >> 17);
  num_llarg_aleatori = num_llarg_aleatori ^ (num_llarg_aleatori << 5);
  return (num_llarg_aleatori & 0x7fff);
}

// File name handling removed - MSXgl uses different file I/O

void FT_errorHandler(char n, char *name) // Gère les erreurs
{
  VDP_SetMode(VDP_MODE_SCREEN0);
  VDP_SetColor(0x16);
  switch (n)
  {
      case 1:
          DOS_StringOutput("\n\rFAILED: file open: ");
          DOS_StringOutput(name);
      break;
 
      case 2:
          DOS_StringOutput("\n\rFAILED: file close: ");
          DOS_StringOutput(name);
      break;  
 
      case 3:
          DOS_StringOutput("\n\rStop Kidding, run me on MSX2 !");
      break; 
  }
  DOS_Exit0();
}

void FT_SetName(DOS_FCB *p_fcb, const char *p_name) // Escrivim el nom del fitxer tal i com l'espera el DOS a l'estructura FCB
{
  char i, j;
  Mem_Set(0, p_fcb, sizeof(DOS_FCB));
  for (i = 0; i < 11; i++) {
    p_fcb->Name[i] = ' ';
  }
  for (i = 0; (i < 8) && (p_name[i] != 0) && (p_name[i] != '.'); i++) {
    p_fcb->Name[i] = p_name[i];
  }
  if (p_name[i] == '.') {
    i++;
    for (j = 0; (j < 3) && (p_name[i + j] != 0) && (p_name[i + j] != '.');
         j++) {
      p_fcb->Name[8 + j] = p_name[i + j];
    }
  }
}

int FT_LoadSc5Image(char *file_name) {
  FT_SetName(&g_File, file_name);
  DOS_OpenFCB(&g_File);

  // Read file and copy content into VRAM
  u16 dst = 0x8000;
  for (u16 i = 0; i < g_File.Size; i += 128) {
    DOS_SetTransferAddr(g_StrBuffer);
    DOS_SequentialReadFCB(&g_File);

    const u8 *src;
    u8 size;
    if (i == 0) {
      src = g_StrBuffer + 7;  // Saltem els primers 7 bytes de la imatge
      size = 128 - 7;
    } else {
      src = g_StrBuffer;
      size = 128;
    }

    VDP_WriteVRAM(src, dst, 0, size);
    dst += size;
  }
  DOS_CloseFCB(&g_File);
}

int FT_LoadPalette(char *file_name, char *buffer) {
  u16 paleta[16];
  u8 paleta_flatten[48];

  FT_SetName(&g_File, file_name);
  DOS_OpenFCB(&g_File);

  // Read file and copy content into paleta
  // Llegeix 128 bytes perquè són els que llegeix cada vegada que fa ReadFCB epl
  // propi MSX-DOS, un "record" són 128 bytes
  DOS_SetTransferAddr(g_StrBuffer);
  DOS_SequentialReadFCB(&g_File);

  for (u8 k = 0; k < 24; k++) {
    paleta_flatten[2 * k] = buffer[k+7] >> 4 & 0x07;
    paleta_flatten[2 * k + 1] = buffer[k+7] & 0x07;
  }
  for (int k = 0; k < 16; k++) {
    paleta[k] = RGB16( paleta_flatten[3*k], paleta_flatten[3*k+1], paleta_flatten[3*k+2]);    
  }
  DOS_CloseFCB(&g_File);

  VDP_SetPalette(paleta)  ;
  return 1;
}

/***** INTERRUPCIONS *****/
__at 0xc000 char copsVsync;
__at 0xc001 u8 debugar;
char processar_moviment;
char processar_esquirols;

// V-blank interruption handler (called from the ISR)
void VDP_InterruptHandler()
{
    processar_moviment = 1;
    processar_esquirols = 1;
    copsVsync++;
}

int stamp_x; // Les coordenades a on està la imatge del tile a transformar. Hauré de fer una funció i un lookup table
int stamp_y; // Mantindran les coordenades a on està el tipus de tile

void obtenir_coordenades_rajola(char map_x, char map_y) {
  // Li passem les coordenades del mapa i retorna la posició que s'ha de retallar per fer el VDP_CommandHMMM (stamp_x, stamp_y)
  char tipus = map1[map_x + (map_y * NOMBRE_RAJOLES_HOR)];
  stamp_x = (tipus % NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS) * 8;
  stamp_y = OFFSET_COORDENADAY_PAGINA_ACTIVA_1 + (tipus / NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS)*8;
}

void crea_velocitat_esquirols(char num_esquirol) {
  // La velocitat que porten
  esquirols[num_esquirol].speed_x = (rand_xor() & 3) - 1;
  esquirols[num_esquirol].speed_y = (rand_xor() & 3) - 1;

  // Comprovem que no siguin 0 i 0
  if (esquirols[num_esquirol].speed_x == 0 &&
      esquirols[num_esquirol].speed_y == 0) {
    esquirols[num_esquirol].speed_x = -1;
    esquirols[num_esquirol].speed_y = -1;
  }

  // Fem també el tamany, que haurà d'anar canviant la distribució segons avanci el joc per adequar-lo a la dificultat
  esquirols[num_esquirol].tamany = 8;
}

void init_pantalla_joc() {
  // Carreguem la imatge dels patrons
  FT_LoadSc5Image("PatCit.sc5"); // Carreguem la imatge
  FT_LoadPalette("PatCit.pl5", g_StrBuffer);

  // Fem un escombrat del mapa per pintar cada patró
  // Ara el mapa del joc és més gran que la pantalla. He d'escombrar diferent
  for (int m = 0; m < NOMBRE_RAJOLES_PANTALLA_VER; m++) {
    for (int n = 0; n < NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS; n++) {
      // És fins al 31, ja que els últims 8 pixels queden amagats en fer
      // l'scroll. Són els últims 6 pixels
      obtenir_coordenades_rajola(n + map_tile_x, m + map_tile_y);
      VDP_CommandHMMM(stamp_x, stamp_y, n * 8,
           OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + m * 8, 8, 8);
    }
  }

  VDP_SetSpriteFlag(VDP_SPRITE_SIZE_8 | VDP_SPRITE_SCALE_1);
  VDP_LoadSpritePattern(sprite, 0, 1);
  VDP_LoadSpritePattern(esquirol, 1, 1);
  // Resetegem la resta d'sprites, estaven tots a la línia 217 quan s'inicialitzava l'aplicació
  for(int k=2; k<32; k++){
    VDP_SetSpritePosition(k, 255, 255);
  }

  // Posem tots els sprites menys al 0 a la pila ja que el 0 és el del caràcter principal
  for (char k = 0; k < MAX_PLANS_SPRITE; k++) {
    pila_plans_sprite.data[k] = k + 1;
  }
  pila_plans_sprite.top = MAX_PLANS_SPRITE-1;

  // Creem tota l'estructura d'esquirols
  // Si hi ha massa esquirols al principi, puc fer aquest bucle fins a un número i la resta posar-los com a eliminar
  for (char k=0; k < NUM_ESQUIROLS; k++) {
    // Aquí no cal que calculi res perquè l'scroll és 0
    // Determinem si apareix dalt,baix o dreta, dreat
    char pos_apareix = rand_xor() & 3;
    if (pos_apareix == 0) {
      esquirols[k].y = 0;
      esquirols[k].x = rand_xor() & 255;
    } else if (pos_apareix == 1) {
      esquirols[k].y = 211;
      esquirols[k].x = rand_xor() & 255;
    } else if (pos_apareix == 2) {
      esquirols[k].y = rand_xor() & 211;
      esquirols[k].x = 0;
    } else if (pos_apareix == 1) {
      esquirols[k].y = rand_xor() & 211;
      esquirols[k].x = 255;
    }

    esquirols[k].eliminar = 0;

    crea_velocitat_esquirols(k);
   
    esquirols[k].pintar = 1;

    esquirols[k].num_pla_sprite = pop(&pila_plans_sprite);
  }

}

char pos_scroll_x;
char pos_scroll_y;
char fer_scroll_lateral; // Per si ja hem arribat als límits
char fer_scroll_vertical;
char desti_x;
char desti_y;
void scroll_amunt() {
  if(map_tile_y >= 1){
    fer_scroll_vertical = 1;
  }

  if(fer_scroll_vertical==1){
    pos_scroll_y -=1;

    if((pos_scroll_y & 7)==0) {
      map_tile_y -=1;
      desti_y = pos_scroll_y - 8;
      for (int n = 0; n < NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS; n++) {
        obtenir_coordenades_rajola(n + map_tile_x, map_tile_y);
        desti_x = (n + (pos_scroll_x>>3)) * 8; 
        VDP_CommandHMMM(stamp_x, stamp_y, desti_x,
             OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + desti_y, 8, 8);
      }
      pos_scroll_y -= 1; // Amb aquesta comanda i la de sota ja ho fa tot bé el de pintar. Abans hi havia un error quan era en aquest llindar
    }
    // MSXgl doesn't have vertical scroll functions for Screen 5, need to implement manually
    VDP_SetVerticalOffset(pos_scroll_y);
  }
  if(map_tile_y == 0 && fer_scroll_vertical==1){
    fer_scroll_vertical = 0;
    pos_scroll_y -= 1;
  }
}

void scroll_avall() {
  if (map_tile_y < NOMBRE_RAJOLES_VER - NOMBRE_RAJOLES_PANTALLA_VER - 1){
    fer_scroll_vertical = 1;
  }

  if(fer_scroll_vertical==1){
    pos_scroll_y += 1;

    if ((pos_scroll_y & 7) == 0) {
      map_tile_y += 1;
      desti_y = pos_scroll_y + 216;  // Per què 216, d'on surt???
      for (int n = 0; n < NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS; n++) {
        obtenir_coordenades_rajola(n + map_tile_x,
                                   map_tile_y + NOMBRE_RAJOLES_PANTALLA_VER - 1);
        desti_x = (n + (pos_scroll_x>>3)) * 8;
        VDP_CommandHMMM(stamp_x, stamp_y, desti_x, OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + desti_y, 8, 8);
      }
      pos_scroll_y += 1;
    }
    VDP_SetVerticalOffset(pos_scroll_y);
  }
  if ((map_tile_y == NOMBRE_RAJOLES_VER - NOMBRE_RAJOLES_PANTALLA_VER + 1) && fer_scroll_vertical==1) {
    // He de sumar a NOMBRE_RAJOLES_PANTALLA_VER perquè. També hem de tenir en compte que queda a mitges. S'haurà de pintar el bordó
    fer_scroll_vertical = 0;
    pos_scroll_y += 1;
  }
}


void scroll_esq() {
  if (map_tile_x>3) {
    fer_scroll_lateral = 1;
  }
  if (fer_scroll_lateral==1) {
    // Ajustem la posició dels esquirols abans de canviar l'scroll
    for (char k=0; k < NUM_ESQUIROLS; k++) {
      if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
        esquirols[k].x += 1; // Compensem el moviment de l'scroll
      }
    }
    
    pos_scroll_x -= 1; // És un char quan arribi a 0 i se li resti 1 torna a 256

    if ((pos_scroll_x & 7) == 0) {
      map_tile_x -= 1; // És un char, no arriba mai a ser més petit que 0
      // Falta la segona part que queda tallada entre les dues pàgines
      desti_x = pos_scroll_x - 8; // D'aquesta forma fa l'operació com a char, en mòdul 256
      for (int m = 0; m < NOMBRE_RAJOLES_PANTALLA_VER_SCROLL ; m++) {
        obtenir_coordenades_rajola(map_tile_x, m + map_tile_y);
        desti_y = (m + (pos_scroll_y >> 3)) * 8;
        VDP_CommandHMMM(stamp_x, stamp_y, desti_x,
             OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + desti_y, 8, 8);
      }

      // Ajustem la posició dels esquirols per l'scroll addicional
      for (char k=0; k < NUM_ESQUIROLS; k++) {
        if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
          esquirols[k].x += 1; // Compensem el moviment extra de l'scroll
        }
      }
      
      pos_scroll_x -=1; // Sembla que amb aquest extra de desplaçament les col·lisions cap a l'esquerra després del primer xoc funcionen. Però si faig la seqüència de pintar, encara la fa malament
    }
    VDP_SetHorizontalOffset(pos_scroll_x);
  }
  if (map_tile_x == 3 && fer_scroll_lateral == 1) {
    // Per què ha de ser 3? Pot ser menor
    // Ha fet la volta del char
    fer_scroll_lateral = 0;
    
    // Ajustem la posició dels esquirols per l'scroll addicional
    for (char k=0; k < NUM_ESQUIROLS; k++) {
      if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
        esquirols[k].x += 1; // Compensem el moviment extra de l'scroll
      }
    }
    
    pos_scroll_x -=1; // Faltava aquest scroll per poder mantenir la lògica de les caselles
  }
}

void scroll_dreta() {
  if (map_tile_x < NOMBRE_RAJOLES_HOR - NOMBRE_RAJOLES_PANTALLA_HOR - 1) {
    fer_scroll_lateral = 1;
  }
  if (fer_scroll_lateral==1){
    // Ajustem la posició dels esquirols abans de canviar l'scroll
    for (char k=0; k < NUM_ESQUIROLS; k++) {
      if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
        esquirols[k].x -= 1; // Compensem el moviment de l'scroll
      }
    }
    
    pos_scroll_x += 1;
    if ((pos_scroll_x & 7) == 0) {
      map_tile_x += 1; 
      desti_x = pos_scroll_x - 8; // D'aquesta forma fa l'operació com a char, en mòdul 256

      for (int m = 0; m < NOMBRE_RAJOLES_PANTALLA_VER_SCROLL; m++) {
        obtenir_coordenades_rajola(map_tile_x + 31, m + map_tile_y);
        desti_y = (m + (pos_scroll_y >> 3)) * 8;
        VDP_CommandHMMM(stamp_x, stamp_y, desti_x,
             OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + desti_y, 8, 8);
      }

      // Ajustem la posició dels esquirols per l'scroll addicional
      for (char k=0; k < NUM_ESQUIROLS; k++) {
        if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
          esquirols[k].x -= 1; // Compensem el moviment extra de l'scroll
        }
      }
      
      pos_scroll_x +=1;
    }
    VDP_SetHorizontalOffset(pos_scroll_x);
  }
  if (map_tile_x == (NOMBRE_RAJOLES_HOR - NOMBRE_RAJOLES_PANTALLA_HOR -1) && fer_scroll_lateral == 1) {
    // Ha fet la volta del char
    fer_scroll_lateral = 0;
    
    // Ajustem la posició dels esquirols per l'scroll addicional
    for (char k=0; k < NUM_ESQUIROLS; k++) {
      if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
        esquirols[k].x -= 1; // Compensem el moviment extra de l'scroll
      }
    }
    
    pos_scroll_x +=1 ; 
  }
}

char stick;
char space;
char es_casellaParet(int nombre_casella){
  if (nombre_casella<0) {
    return(0);
  } else if (nombre_casella>=sizeof(map1)) {
    return(0);
  }
  if (map1[nombre_casella]<3) {
      return(1);
  }  else if (map1[nombre_casella]>=32 && map1[nombre_casella]<=34 ) {
    return(1);
  } else if (map1[nombre_casella] >= 64 && map1[nombre_casella] <= 66) {
    return (1);
  } else if (map1[nombre_casella] >= 96 && map1[nombre_casella] <= 98) {
    return (1);
  } else {
    return(0);
  }
}

char tile_esq;
char tile_amunt;
int casella_1;
int casella_2;
void calculem_tiles(){
  // Investiguem caselles a on estem de la pantalla. El scroll i el mapa ja s'ajusten amb el map_tile_x
  tile_esq = (x + (pos_scroll_x & 7)) >> 3;
  tile_amunt = (y + (pos_scroll_y&7))>>3;
}
// Funció per comprovar si un sprite col·lisiona amb una paret
// map_tile_x, map_tile_y són variables globals de la posició del mapa no cal passar-les com a paràmetre
char esParet_amunt(char tile_esq, char tile_amunt){
  int mov_casella_1 =
    tile_esq + map_tile_x + NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y);
  int mov_casella_2 = mov_casella_1 - 1; // Aquest fa que diagonal dreta amunt vagi un abans
  return( es_casellaParet(mov_casella_1) || es_casellaParet(mov_casella_2));
}
char esParet_avall(char tile_esq, char tile_amunt){
  int mov_casella_1 = tile_esq -1 + map_tile_x +
    NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y + 1); 
  int mov_casella_2 = mov_casella_1 + 1;
  char valor = (es_casellaParet(mov_casella_1) || es_casellaParet(mov_casella_2) );
  //return(es_casellaParet(mov_casella_1));
  return (valor);
}
char esParet_dreta(char tile_esq, char tile_amunt){
  // Ajustem la posició per comprovar exactament on col·lisionarà
  int mov_casella_1 = (tile_esq) + map_tile_x +
    NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y) +1;
  // int mov_casella_2 =
  //    (tile_esq) + map_tile_x + NOMBRE_RAJOLES_HOR * (tile_amunt +1 + map_tile_y);
  // char valor = (es_casellaParet(mov_casella_1) || es_casellaParet(mov_casella_2) );
  return(es_casellaParet(mov_casella_1));
  // return (valor);
}
char esParet_esquerra(char tile_esq, char tile_amunt){
  int mov_casella_1 = (tile_esq - 1) + map_tile_x +
    NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y)+1;
  //int mov_casella_2 = (tile_esq - 1) + map_tile_x +
  //  NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y + 1);
  //return (es_casellaParet(mov_casella_1) || es_casellaParet(mov_casella_2) );
  return(es_casellaParet(mov_casella_1));
}

void moviment_amunt(){
  y = (y - 1);
  calculem_tiles();
  if( esParet_amunt(tile_esq, tile_amunt) == 1) {
    y = y+1;
  }
  if (y < 14) {
    y = y + 1;
    scroll_amunt();
  }
}
void moviment_avall(){
  y = (y + 1);
  calculem_tiles();
  if ( esParet_avall(tile_esq, tile_amunt) == 1) {
    y = y - 1;
  }
  if (y > 206 - 8) {
    y = y - 1;
    scroll_avall();
  }
}
void moviment_dreta(){
  x = (x + 1);
  calculem_tiles();
  if (esParet_dreta(tile_esq, tile_amunt) == 1) {
    x = x - 1;
  }
  if (x > 241) {
    // Els primers estan amagats
    x = x - 1;
    scroll_dreta();
  }
}
void moviment_esquerra(){
  x = (x - 1);
  calculem_tiles();
  if ( esParet_esquerra(tile_esq, tile_amunt) == 1) {
    x = x + 1;
  }
  if (x < 14) {
    // Els primers estan amagats
    x = x + 1;
    scroll_esq();
  }
}


char tile_esq_esquirol;
char tile_amunt_esquirol;
int casella_esquirol;

char esquirol_col_lisiona_amb_paret(int nova_x, int nova_y, char mov_num_esquirol) {
  // Calculem les coordenades de tile per l'esquirol
  // Utilitzem la mateixa lògica que a calculem_tiles() per consistència
  unsigned char tile_esq_esquirol_treb = (nova_x) + (pos_scroll_x & 7);
  tile_esq_esquirol = tile_esq_esquirol_treb >> 3;
  unsigned char tile_amunt_esquirol_treb = (nova_y - pos_scroll_y) + (pos_scroll_y & 7) ;
  tile_amunt_esquirol = tile_amunt_esquirol_treb >> 3;

  // Calculem les caselles que ocupa l'esquirol segons la direcció del moviment
  char esParet = 0;
  
  // Comprovem col·lisió segons la direcció del moviment
  // Només comprovem la direcció en què s'està movent l'esquirol
  if (esquirols[mov_num_esquirol].speed_x > 0) {
    // Moviment cap a la dreta
    esParet = esParet_dreta(tile_esq_esquirol, tile_amunt_esquirol);
  } else if (esquirols[mov_num_esquirol].speed_x < 0) {
    // Moviment cap a l'esquerra
    esParet = esParet_esquerra(tile_esq_esquirol, tile_amunt_esquirol);
  }
  
  if (esquirols[mov_num_esquirol].speed_y > 0) {
    // Moviment cap avall
    esParet = esParet || esParet_avall(tile_esq_esquirol, tile_amunt_esquirol);
  } else if (esquirols[mov_num_esquirol].speed_y < 0) {
    // Moviment cap amunt
    esParet = esParet || esParet_amunt(tile_esq_esquirol, tile_amunt_esquirol);
  }

  return esParet;
}

void actualitza_pos_esquirols() {
  for (char k=0; k < NUM_ESQUIROLS; k++) {
    // AQuí també he d'actualitzar el x i y de la càmera
    if (esquirols[k].eliminar == 1) {
      // S'han de crear com els que apareguin a la pantalla, per tant ara sí que s'ha de tenir en compte l'scroll
      char pos_apareix = rand_xor() & 3;
      int num_aleatori_deb = rand_xor();
      if (pos_apareix == 0) {
        // a dalt
        esquirols[k].y = pos_scroll_y;
        esquirols[k].x = (num_aleatori_deb & 255) + pos_scroll_x;
      }
      else if (pos_apareix == 1) {
        // a baix
        esquirols[k].y = 211 + pos_scroll_y;
        esquirols[k].x = (num_aleatori_deb & 255) + pos_scroll_x;
      }
      else if (pos_apareix == 2) {
        // a l'esquerra
        esquirols[k].y = (num_aleatori_deb & 211) + pos_scroll_y; 
        esquirols[k].x = pos_scroll_x;
      }
      else if (pos_apareix == 3) {
        // a la dreta
        // és aquest que ha donat error. Potser també alguna altra conversió estranya de signes?
        char temp = (num_aleatori_deb & 211);
        esquirols[k].y = temp+pos_scroll_y;
        esquirols[k].x = 255 + pos_scroll_x; // No té sentit aquest +255, tant és a on apareix
      }

      // Comprovem si l'esquirol s'ha creat en una paret
      tile_esq_esquirol = ((esquirols[k].x) + (pos_scroll_x & 7)) >> 3;
      unsigned char esqu_y = esquirols[k].y - pos_scroll_y;
      tile_amunt_esquirol = ((esqu_y) + (pos_scroll_y & 7)) >> 3;
      
      // Comprovem si està en una paret
      casella_esquirol = tile_esq_esquirol + map_tile_x + 
        NOMBRE_RAJOLES_HOR * (tile_amunt_esquirol + map_tile_y);
      
      if (es_casellaParet(casella_esquirol)) {
        // Si està en una paret, el marquem per recrear-lo
        esquirols[k].eliminar = 1;
      }
      else {
        esquirols[k].eliminar = 0;
        crea_velocitat_esquirols(k);
        esquirols[k].pintar = 1;
        esquirols[k].num_pla_sprite = pop(&pila_plans_sprite);
      }
    }
    else {
      // Ara hem de fer el desplaçament i veure si han sortit de la pantalla
      // Com que faig que la posició variï amb l'scroll amb mòdul 256 no puc saber a quin lloc estan
      // Per tant miraré la posició origen i la final si creua un dels llindars de la càmera (scroll_x, scroll_y)
      // He d'afegir el pos_scroll a la posició X dels esquirols, tant i com s'estant creant, si no sempre desapareixen sense creuar els límits
      int calcul_x = esquirols[k].x + esquirols[k].speed_x;
      int calcul_y = esquirols[k].y + esquirols[k].speed_y;
      
      // Comprovem si l'esquirol surt de la pantalla
      if (((esquirols[k].x > pos_scroll_x) &&
           (((esquirols[k].x + esquirols[k].speed_x) & 0xFF) < pos_scroll_x)) ||
          ((esquirols[k].x < pos_scroll_x) &&
           (((esquirols[k].x + esquirols[k].speed_x) & 0xFF) > pos_scroll_x)) ||
          ((esquirols[k].y > pos_scroll_y) &&
           (calcul_y >= pos_scroll_y + 255)) ||
          ((esquirols[k].y < pos_scroll_y) && (calcul_y + esquirols[k].tamany >= pos_scroll_y))) {
          
          esquirols[k].pintar = 0;
          esquirols[k].eliminar = 1;

          push(&pila_plans_sprite, esquirols[k].num_pla_sprite);
      } else {
        if(esquirol_col_lisiona_amb_paret(calcul_x, calcul_y, k)) {
          esquirols[k].speed_x = -esquirols[k].speed_x;
          esquirols[k].speed_y = -esquirols[k].speed_y;
        }

        esquirols[k].x += esquirols[k].speed_x;
        esquirols[k].y += esquirols[k].speed_y;
      }
    }
    VDP_SetSprite(esquirols[k].num_pla_sprite, esquirols[k].x, esquirols[k].y, 1);
  }
}

void moviment_sprite(){
  while (!Bios_IsKeyPressed(KEY_ESC))
  {
    if (processar_moviment == 1) {
      // Joystick input
      stick = Joystick_Read(JOY_PORT_1);
      space = Joystick_IsButtonPressed(JOY_PORT_1, JOY_INPUT_TRIGGER_A);

      u8 dir = Joystick_GetDirection(JOY_PORT_1);
      if (dir & JOY_INPUT_DIR_UP) {
        moviment_amunt();
      }
      if (dir & JOY_INPUT_DIR_DOWN) {
        moviment_avall();
      }
      if (dir & JOY_INPUT_DIR_LEFT) {
        moviment_esquerra();
      }
      if (dir & JOY_INPUT_DIR_RIGHT) {
        moviment_dreta();
      }

      // Keyboard input
      u8 row8 = Keyboard_Read(8);
      debugar = row8;      
      if (IS_KEY_PRESSED(row8, KEY_UP)) {
        moviment_amunt();
        debugar = 0xaa;
      }
      if (IS_KEY_PRESSED(row8, KEY_DOWN)) {
        moviment_avall();
      }
      if (IS_KEY_PRESSED(row8, KEY_LEFT)) {
        moviment_esquerra();
      }
      if (IS_KEY_PRESSED(row8, KEY_RIGHT)) {
        moviment_dreta();
      }

      VDP_SetSpritePosition(0, x, y+pos_scroll_y); // Sumem el pos_scroll_y, ja que hi ha un offset cada cop que fem scroll

      processar_moviment = 0;

      if (processar_esquirols == 1) {
        actualitza_pos_esquirols();
        processar_esquirols = 0;
      }
    }
  }
}


void main() {
  u8 g_StrBuffer[128];
  VDP_SetMode(VDP_MODE_GRAPHIC4);
  VDP_SetPage(2);
  map_tile_x = 12;
  map_tile_y = 14;
  // Imatge a la pàgina 1 i visualització de la 2  
  init_pantalla_joc();
  // Canviem el color 15 perquè sigui blanc i el text
  VDP_SetPaletteEntry(15, RGB16(7, 7, 7));
  copsVsync = 0;
  debugar = 0;
  processar_moviment=0;
  processar_esquirols=0;
  pos_scroll_x = 1;
  pos_scroll_y = 1; // L'inicialitzem amb 1 perquè així si vaig amunt també el detecta com a 0. 
  fer_scroll_lateral = 1;
  fer_scroll_vertical = 1;
  VDP_EnableMask(TRUE);  // SetScrollMask(1)
  VDP_SetHorizontalOffset(1); // Comencem a la mateixa posició que el que hem après de scrpro.c

  VDP_EnableVBlank(TRUE);
  Bios_SetHookCallback(H_TIMI, VDP_InterruptHandler);
  // Posem sprite al 50,50
  x=20; y=19;
  VDP_SetSprite(0, x, y, 0);
  moviment_sprite();
  Bios_ClearHook(H_TIMI);
  VDP_EnableVBlank(FALSE);
  VDP_SetMode(VDP_MODE_SCREEN0);
  VDP_SetColor(0x16);

  DOS_StringOutput("Pos sprite: ");
  // Note: DOS_StringOutput doesn't support formatted output like printf
  // You'll need to convert numbers to strings manually or use a different approach
  String_Format(g_StrBuffer, "Cops Vsync %d\n\r", copsVsync);
  DOS_StringOutput(g_StrBuffer);
  DOS_StringOutput("Debug info printed\n");
  DOS_Exit0();
}

/* 2024-04-22 Els sprites van molt ràpids, hauré de fer les animacions més lentes d'ells, no a cada pantalla com amb el protagonista.
   A cops també agafen el color del protagonista, he de mirar la gestió del número de plans. Però només quan es mou, és estrany. 
   Una altra cosa que he vist és que amb aquest plantejament, no els puc anar perseguint, ja que si creuen el límit de la pantalla tornen a aparèixer. Hauré de tenir les dimensions de la pantalla i un altre amb l'offset que serà la pantalla del joc a on apareixeran els enemics
   No hi ha cap enemic que vagi de dreta a esquerra
   Els enemics desapareguts també es queden a la pantalla
*/
/* 2024-04-28 Faltava fer el push. Ja l'he afegit. Ara ja no borra l'sprite del protagonista.
   Falla l'scroll. A vegades el torna a posar diferent.
   Els enemeics salten els obstacles. A la primera versió els farem senzills
   Amb l'scroll avall, hi ha sprites que no arriben a final de tot
*/
/* 2024-05-08 Scroll arreglat, era problema de l'offset del càlcul. He agafat la correcció de scroll_fons.
 Tant el primer com el segon comencen en el mateix pla, el 0x1e, per això només es dibuixen 3. Arreglat, el top de la pila no era el correcte.
 L'scroll afecta la posició dels sprites. No està fet de forma correcte */
/* 2024-05-20 He fet el programa sprscr_provant per veure si els sprites en scroll vertical desapareixen i no he vist cap comportament estrany. Van desapareixent en el límit de la pantalla. No sé perquè desapareixen en el enemScFo */
/* 2024-08-10 continuant la nota del dia 2024-05-08, l'scroll afecta els sprites dels conills, quan l'usuari desplaça l'scroll, els sprites dels conills s'haurien de mantenir al tile que estaven abans de l'scroll, no a la pantalla com fa l'usuari. Ha de ser una lògica diferent. També es continua observant que quan tiro l'scroll avall, hi ha un punt a on desapareixen. Quan faig l'scroll també hauré de desplaçar els esquirols que estiguin visibles perquè mantinguin la mateixa casella. Primer investigar perquè desapareixen quan tiro avall l'scroll */
/* 2024-08-13 Arreglat que l'esquirol desapareixi en l'eix vertical. Quan tombo cap a la dreta, no detecta que s'hagi d'eliminar el conill, sempre està funcionant. He de continuar debugant */
/* 2025-04-19 després de molt de temps treballant amb la interrupció de línia, torno amb el tema dels sprites. Li he dit al aider-sonnet que arreglés el moviment lateral tal i com havia fet jo amb el vertical. I funciona. */
/* 2025-04-23 He modificat perquè els esquirols detectin si és paret. Però a cops ho detecta i a cops no. He definit la variable debugar per començar a esbrinar què està passant */
/* 2025-04-28 Crec que el que està passant és que no calcula bé la posició de les Y. Mirar en un mateix punt el valor de les coordenades de l'esquirol 0x7031 i 0x7030 i les del personatge principal 0x7014 i 0x7015
 !!! Quan els creo els esquirols també hauré de mirar que no estiguin a casellaParet !!!!*/
/* He provat d'eliminar l'operació de restar scroll per tile_amunt_esquirol = nova_y >>3 i no ha funcionat */
/* 2025-04-29 era problema de l'scroll. He posat més esquirols i queda xulo. Hi ha algunes col·lisions dels esquirols que es solapen i genera quan està en casella prohibida. També hi ha alguns rebots al límit de la pantalla que rebota en lloc de desaparèixer */
/* 2025-05-05 He estat millorant les col·lisions, però a la part dreta no ho fa bé, ho fa un tile massa aviat. L'aider no ho ha pogut corregir */
/* 2025-05-11 Per debugar aniré fent cada posició creant només un esquirol i controlant les seves velocitats */
/* 2025-05-12 Em pensava que havia debut bé les col·lisions dels esquirols, però ara veig que amunt dreta la fa abans. Crec que la velocitat és 2.
   També quan està amunt i rebota dreta primer obstacle dalt de tot esquerra pantalla inicial, quan retorna per creuar la pantalla, fa un rebot en lloc de retornar
   Sembla que només sigui la part d'amunt que dongui col·lisió, no fa servir la de la dreta?? S'activa la col·lisió 4 que és la de l'esquerra¿?
   Quan va cap a baix al final de tot, també col·lisiona amb rebot alguns. Són els que se'n va de l'array de tiles, el de <0 i >número_de_tiles
*/
/* 2025-05-19 He provat de posar aixó a es_casellaParet:   if (nombre_casella<0)
  { return(0); } else if (nombre_casella>=sizeof(map1)) { return(0);
  }
  Però ho ha fet igual. A més quan és al principi que rebota, no estic a dalt de tot. Però potser en algun punt hi ha un valor negatiu que no estigui fent bé.
*/
/* 2025-05-22 He fet el càlcul i retorna uns valors del mapa que estan entre 0-3. Però què varia quan canvio l'scroll? tile_amunt i tile_esq es van reduint fins a 0 i després de cop prene el valor 0 esq 20 amunt. Tampoc haurien de ser 0-0 a la posició inicial ja que hi ha més tiles cap amunt. No sé si algun lloc fa la conversió als tiles totals del mapa o el tile_esquirol és el tile dins la pantalla i no dins el mapa
   Pot ser que el tile_amunt esquirol amb el pos_scroll_y negatiu no el faci bé i per això dóna el 0x20
   He comprovat de fer els passos del tile per separat i ho fa bé. He de mirar quina és la diferència en codi compilat
   Aquí hi ha els codis de diferència:
                                      7590 ;learning-fusion-c-programs/enemScFo.c:811: esquirol_y = nova_y - pos_scroll_y;
      0022DD DD 7E 06         [19] 7591 	ld	a, 6 (ix)
      0022E0 21 06 70         [10] 7592 	ld	hl, #_pos_scroll_y
      0022E3 4E               [ 7] 7593 	ld	c, (hl)
      0022E4 21 F9 6F         [10] 7594 	ld	hl, #_esquirol_y
      0022E7 91               [ 4] 7595 	sub	a, c
      0022E8 77               [ 7] 7596 	ld	(hl), a
                                   7597 ;learning-fusion-c-programs/enemScFo.c:813: pos_scr_deb = pos_scroll_x & 7;
      0022E9 3Ar7Cr0A         [13] 7598 	ld	a,(#_pos_scroll_x + 0)
      0022EC E6 07            [ 7] 7599 	and	a, #0x07
      0022EE 32 F0 6F         [13] 7600 	ld	(_pos_scr_deb+0), a
                                   7601 ;learning-fusion-c-programs/enemScFo.c:814: suma_deb = esquirol_y + pos_scr_deb;
      0022F1 21 F0 6F         [10] 7602 	ld	hl, #_pos_scr_deb
      0022F4 3A F9 6F         [13] 7603 	ld	a,(#_esquirol_y + 0)
      0022F7 86               [ 7] 7604 	add	a, (hl)
      0022F8 32 F1 6F         [13] 7605 	ld	(_suma_deb+0), a
                                   7606 ;learning-fusion-c-programs/enemScFo.c:815: desplac3 = suma_deb >> 3;
      0022FB 3A F1 6F         [13] 7607 	ld	a,(#_suma_deb + 0)
      0022FE 0F               [ 4] 7608 	rrca
      0022FF 0F               [ 4] 7609 	rrca
      002300 0F               [ 4] 7610 	rrca
      002301 E6 1F            [ 7] 7611 	and	a, #0x1f
      002303 32 F2 6F         [13] 7612 	ld	(_desplac3+0), a
                                   7613 ;learning-fusion-c-programs/enemScFo.c:816: tile_amunt_esquirol = ((esquirol_y) + (pos_scroll_y & 7)) >> 3;
      002306 3A F9 6F         [13] 7614 	ld	a, (#_esquirol_y + 0)
      002309 4F               [ 4] 7615 	ld	c, a
      00230A 06 00            [ 7] 7616 	ld	b, #0x00
      00230C 3A 06 70         [13] 7617 	ld	a,(#_pos_scroll_y + 0)
      00230F E6 07            [ 7] 7618 	and	a, #0x07
      002311 6F               [ 4] 7619 	ld	l, a
      002312 26 00            [ 7] 7620 	ld	h, #0x00
      002314 09               [11] 7621 	add	hl, bc
      002315 06 03            [ 7] 7622 	ld	b, #0x03
      002317                       7623 00167$:
      002317 CB 2C            [ 8] 7624 	sra	h
      002319 CB 1D            [ 8] 7625 	rr	l
      00231B 10 FA            [13] 7626 	djnz	00167$
      00231D 7D               [ 4] 7627 	ld	a, l
      00231E 32 02 70         [13] 7628 	ld	(#_tile_amunt_esquirol), a

      Segons l'explicació del Claud:
 *The sra instruction in the second method preserves the sign bit (mostsignificant bit) during shifts, making it a true arithmetic right shift. The rrca in the first method is a circular rotation that doesn't preserve sign.
 Sembla que faci per enters en lloc de chars i per això usa el sra en comptes de rrca
 */
/* 2025-05-27 els rebots els fa bé, però s'hauria de mirar que a cops la generació no surt dels extrems */
/* 2025-05-31 He mirat un cas que apareixia més tard, però estava ben generat, és possible que fos pels altres sprites i la prioritat, ja que estaven visibles a prop d'aquella línia. L'esquirol tenia totes les propietats bé, però només apareixia més tard. He de mirar més casos a veure si això és veritat. No he sapigut trobar més casos per tant entenc que és degut a això */
/* 2025-08-30 He començat el port del fitxer a MSXgl */
/* 2025-09-03 L'sprite principal no es mou. Mirar si executa vSync i entra a la interrupció */
/* 2025-09-11 Sense fer res l'sprite ha començat a moure's, potser no estava executant la versió correcta. Ara quan detecta les col·lisions ho fa abans. He fet un diff i està igual que el Fusion-C. No apareix canvis. Vaig a modificar el detecta col·lisions
   l'scroll horitzontal ensenya tota la part de l'esquerra. S'ha d'inicialitzar el registre de forma correcta. Fet
 Ara quan pinta a la dreta, es veu com està dibuixant. També s'ha de corregir. Potser és perquè MSXgl no té waits i s'ha d'esperar que acabi la comanda*/
