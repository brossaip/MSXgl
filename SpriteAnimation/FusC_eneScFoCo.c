/* Vull afegir ara el control de col·lisions entre el protagonista i els esquirols */

#include "../fusion-c/include/msx_fusion.h"
#include "../fusion-c/include/vdp_graph2.h"
#include "../fusion-c/include/vdp_sprites.h"
#include <stdio.h>
#include <string.h>

static FCB file; // Initialisatio de la structure pour le systeme de fichiers

#define BUFFER_SIZE_SC5 2560

unsigned char LDbuffer[BUFFER_SIZE_SC5];
char mypalette[16 * 4];
unsigned char map_tile_x; // Per indicar la rajola de dalt a l'esquerra. Potser es pot fusionar amb tile_esq. No es pot fusionar, ja que tile_esq és la posició actual del personatge
unsigned char map_tile_y;
unsigned char x;
unsigned char y;

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

__at 0xc020 struct_esquirol esquirols[NUM_ESQUIROLS];

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

__at 0xc000 Stack pila_plans_sprite; // He d'investigar perquè s'utilitza el pla1 i es queda quiet sense que jo el pugui col·lisionar.

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
  num_llarg_aleatori = (_Time << 15) | (unsigned long)_Time;
}

int rand_xor() {
  num_llarg_aleatori = num_llarg_aleatori ^ (num_llarg_aleatori << 13);
  num_llarg_aleatori = num_llarg_aleatori ^ (num_llarg_aleatori >> 17);
  num_llarg_aleatori = num_llarg_aleatori ^ (num_llarg_aleatori << 5);
  return (num_llarg_aleatori & 0x7fff);
}

void FT_SetName(FCB *p_fcb, const char *p_name) // Routine servant à vérifier le
                                                // format du nom de fichier
{
  char i, j;
  memset(p_fcb, 0, sizeof(FCB));
  for (i = 0; i < 11; i++) {
    p_fcb->name[i] = ' ';
  }
  for (i = 0; (i < 8) && (p_name[i] != 0) && (p_name[i] != '.'); i++) {
    p_fcb->name[i] = p_name[i];
  }
  if (p_name[i] == '.') {
    i++;
    for (j = 0; (j < 3) && (p_name[i + j] != 0) && (p_name[i + j] != '.');
         j++) {
      p_fcb->ext[j] = p_name[i + j];
    }
  }
}

void FT_errorHandler(char n, char *name) // Gère les erreurs
{
  Screen(0);
  SetColors(15,6,6);
  switch (n)
  {
      case 1:
          Print("\n\rFAILED: fcb_open(): ");
          Print(name);
      break;
 
      case 2:
          Print("\n\rFAILED: fcb_close():");
          Print(name);
      break;  
 
      case 3:
          Print("\n\rStop Kidding, run me on MSX2 !");
      break; 
  }
Exit(0);
}

int FT_LoadSc5Image(char *file_name, unsigned int start_Y, char *buffer, unsigned int amplada_linia, unsigned int tamany_buffer)        // Charge les données d'un fichiers
    {
      // El tamany de tamany_buffer és el nombre de bytes que fan les 20 linies. En screen 5 és 2560, però en screen 7 és 2560*2
        int rd=2560;

        FT_SetName( &file, file_name );
        if(fcb_open( &file ) != FCB_SUCCESS) 
        {
              FT_errorHandler(1, file_name);
              return (0);
        }

        fcb_read( &file, buffer, 7 );  // Skip 7 first bytes of the file
        // La imatge creada també amb el Viewer té els 7 primers bytes com
        // s'indica a la secció de binary files de
        // https://www.msx.org/wiki/MSX-BASIC_file_formats
        while (rd!=0)
        {
             rd = fcb_read( &file, buffer, tamany_buffer );  // Read 20 lines of image data (128bytes per line in screen5)
             HMMC(buffer, 0,start_Y,amplada_linia,20 ); // Move the buffer to VRAM. 
             start_Y=start_Y+20;
         }

return(1);
}
int FT_LoadPalette(char *file_name, char *buffer, char *mypalette) 
{

  // El vector és de 24 bytes, cadascú té doble valor que fan 48=16*3 ordenats com RG BR GB RG BR GB
  unsigned char paleta_flatten[48];

  FT_SetName(&file, file_name);
  if (fcb_open(&file) != FCB_SUCCESS) {
    FT_errorHandler(1, file_name);
    return (0);
  }

  fcb_read(&file, buffer, 7); // Skip 7 first bytes of the file
  fcb_read(&file, buffer, 24); 
  for(int k=0; k<24; k++){
    paleta_flatten[2*k] = buffer[k]>>4 & 0x07;
    paleta_flatten[2*k+1] = buffer[k] & 0x07;
  }
  for(int k=0; k<16; k++) {
    mypalette[4 * k] = k;
    mypalette[4 * k + 1] = paleta_flatten[3 * k]; //red
    mypalette[4 * k + 2] = paleta_flatten[3 * k + 1]; // green
    mypalette[4 * k + 3] = paleta_flatten[3 * k + 2]; // blue
  }
  SetPalette((Palette *)mypalette);

  return (1);
}

int FT_LoadPalette_MSXViewer(char *file_name, char *buffer, char *mypalette) {
  // Tal i com s'indica a l'exemple del manual
  // http://marmsx.msxall.com/msxvw/msxvw5/download/msxvw5_7_man.pdf la paleta és un binari. Si desensamblem el codi
  FT_SetName(&file, file_name);
  if (fcb_open(&file) != FCB_SUCCESS) {
    FT_errorHandler(1, file_name);
    return (0);
  }

  fcb_read(&file, buffer, 0x29); 
  for (int k=0;k<16;k++){
    mypalette[k*4] = k;
    fcb_read(&file, &mypalette[k*4+1], 3); // Els podem llegir directament. Cada byte és el color
  }
  SetPalette((Palette *)mypalette);

  return (1);
}

/***** INTERRUPCIONS *****/
unsigned char OldHook[5];
unsigned char MyHook[5];
unsigned char IntFunc[5];
unsigned char TypeOfInt;
__at 0xFD9F unsigned char VdpIntHook[];
__at 0xFD9A unsigned char AllIntHook[];
__at 0xF344 unsigned char RAMAD3;

void InterruptHandlerHelper (void) __naked
{
__asm
    push af
    call _IntFunc
    pop af
    jp _OldHook
__endasm;
}

void InitializeMyInterruptHandler (int myInterruptHandlerFunction, unsigned char isVdpInterrupt)
{
    unsigned char ui;
    MyHook[0]=0xF7; //RST 30 is interslot call both with bios or dos
    MyHook[1]=RAMAD3; //Page 3 generally is not paged out and is the slot of the ram, so this should be good
    MyHook[2]=(unsigned char)((int)InterruptHandlerHelper&0xff);
    MyHook[3]=(unsigned char)(((int)InterruptHandlerHelper>>8)&0xff);
    MyHook[4]=0xC9;
    IntFunc[0]=0xCD; //CALL
    IntFunc[1]=(unsigned char)((int)myInterruptHandlerFunction&0xff);
    IntFunc[2]=(unsigned char)(((int)myInterruptHandlerFunction>>8)&0xff);
    IntFunc[3]=0xC9;
    TypeOfInt = isVdpInterrupt;
    //Interrupts must be disabled so no one messes with what we are doing
    DisableInterrupt();
    if (isVdpInterrupt)
    {
        for(ui=0;ui<5;ui++)
            OldHook[ui]=VdpIntHook[ui];
        for(ui=0;ui<5;ui++)
            VdpIntHook[ui]=MyHook[ui];
    }
    else
    {
        for(ui=0;ui<5;ui++)
            OldHook[ui]=AllIntHook[ui];
        for(ui=0;ui<5;ui++)
            AllIntHook[ui]=MyHook[ui];
    }

    //Re-enable Interrupts
    EnableInterrupt();
}

void EndMyInterruptHandler (void)
{
    unsigned char ui;
    //Interrupts must be disabled so no one messes with what we are doing
    DisableInterrupt();

    if (TypeOfInt)
        for(ui=0;ui<5;ui++)
            VdpIntHook[ui]=OldHook[ui];
    else
        for(ui=0;ui<5;ui++)
            AllIntHook[ui]=OldHook[ui];

    //Re-enable Interrupts
    EnableInterrupt();

}
char copsVsync;
char processar_moviment;
char processar_esquirols;
char collision_count;

void main_loop(void) {
  processar_moviment = 1;
  processar_esquirols=1;

  copsVsync++;
}


int stamp_x; // Les coordenades a on està la imatge del tile a transformar. Hauré de fer una funció i un lookup table
int stamp_y; // Mantindran les coordenades a on està el tipus de tile

void obtenir_coordenades_rajola(char map_x, char map_y) {
  // Li passem les coordenades del mapa i retorna la posició que s'ha de retallar per fer el HMMC (stamp_x, stamp_y)
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
  FT_LoadSc5Image("PatCit.sc5", 256, LDbuffer, 512,
                  BUFFER_SIZE_SC5); // Carreguem la imatge
  FT_LoadPalette("PatCit.pl5", LDbuffer, mypalette);
  SetPalette((Palette *)mypalette);

  // Fem un escombrat del mapa per pintar cada patró
  // Ara el mapa del joc és més gran que la pantalla. He d'escombrar diferent
  for (int m = 0; m < NOMBRE_RAJOLES_PANTALLA_VER; m++) {
    for (int n = 0; n < NOMBRE_RAJOLES_HOR_ORIGEN_PATRONS; n++) {
      // És fins al 31, ja que els últims 8 pixels queden amagats en fer
      // l'scroll. Són els últims 6 pixels
      obtenir_coordenades_rajola(n + map_tile_x, m + map_tile_y);
      HMMM(stamp_x, stamp_y, n * 8,
           OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + m * 8, 8, 8);
    }
  }

  // SpriteDouble();
  SpriteSmall();
  SetSpritePattern(0, sprite, 8);
  char colorSprites[] = {1, 9, 10, 1, 9, 10, 7, 13};
  SetSpriteColors(0, colorSprites);
  SetSpritePattern(1, esquirol, 8);
  // Resetegem tots els sprites
  // AQuí estic creant l'sprite 2 a la posició 255, hauria de posar els sprites a la part baixa i anar controlant cada vegada tots els superiors perquè vagin desapareixent. Per això es veuen alguns que no es mouen
  for(int k=0; k<32; k++){
    PutSprite(k,k,255,255,0);
  }

  // Hide unused sprites
  VDP_DisableSpritesFrom(18);
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
        HMMM(stamp_x, stamp_y, desti_x,
             OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + desti_y, 8, 8);
      }
      pos_scroll_y -= 1; // Amb aquesta comanda i la de sota ja ho fa tot bé el de pintar. Abans hi havia un error quan era en aquest llindar
    }
    SetScrollV(pos_scroll_y);
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
        HMMM(stamp_x, stamp_y, desti_x, OFFSET_COORDENADAY_PAGINA_ACTIVA_2 + desti_y, 8, 8);
      }
      pos_scroll_y += 1;
    }
    SetScrollV(pos_scroll_y);
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
        HMMM(stamp_x, stamp_y, desti_x,
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
    SetScrollH(pos_scroll_x);
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
        HMMM(stamp_x, stamp_y, desti_x,
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
    SetScrollH(pos_scroll_x);
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
    NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y);
  int mov_casella_2 =
      (tile_esq) + map_tile_x + NOMBRE_RAJOLES_HOR * (tile_amunt +1 + map_tile_y);
  char valor = (es_casellaParet(mov_casella_1) || es_casellaParet(mov_casella_2) );
  // return(es_casellaParet(mov_casella_1));
  return (valor);
}
char esParet_esquerra(char tile_esq, char tile_amunt){
  int mov_casella_1 = (tile_esq - 1) + map_tile_x +
    NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y);
  int mov_casella_2 = (tile_esq - 1) + map_tile_x +
    NOMBRE_RAJOLES_HOR * (tile_amunt + map_tile_y + 1);
  return (es_casellaParet(mov_casella_1) || es_casellaParet(mov_casella_2) );
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

char check_sprite_collision(char sprite1_x, char sprite1_y, char sprite2_x, char sprite2_y, char sprite_size) {
  // Simple bounding box collision detection
  // Returns 1 if collision detected, 0 otherwise
  if (sprite1_x < sprite2_x + sprite_size &&
      sprite1_x + sprite_size > sprite2_x &&
      sprite1_y < sprite2_y + sprite_size &&
      sprite1_y + sprite_size > sprite2_y) {
    return 1;
  }
  return 0;
}

void update_main_character_color() {
  // Change main character color based on collision count
  char colorSprites[] = {1, 9, 10, 1, 9, 10, 7, 13};
  
  switch (collision_count % 4) {
    case 0:
      // Original colors
      colorSprites[0] = 1; colorSprites[1] = 9; colorSprites[2] = 10; colorSprites[3] = 1;
      colorSprites[4] = 9; colorSprites[5] = 10; colorSprites[6] = 7; colorSprites[7] = 13;
      break;
    case 1:
      // Red colors
      colorSprites[0] = 6; colorSprites[1] = 8; colorSprites[2] = 9; colorSprites[3] = 6;
      colorSprites[4] = 8; colorSprites[5] = 9; colorSprites[6] = 6; colorSprites[7] = 8;
      break;
    case 2:
      // Green colors
      colorSprites[0] = 3; colorSprites[1] = 11; colorSprites[2] = 12; colorSprites[3] = 3;
      colorSprites[4] = 11; colorSprites[5] = 12; colorSprites[6] = 3; colorSprites[7] = 11;
      break;
    case 3:
      // Blue colors
      colorSprites[0] = 4; colorSprites[1] = 5; colorSprites[2] = 13; colorSprites[3] = 4;
      colorSprites[4] = 5; colorSprites[5] = 13; colorSprites[6] = 4; colorSprites[7] = 5;
      break;
  }
  
  SetSpriteColors(0, colorSprites);
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
      casella_esquirol =
          tile_esq_esquirol + map_tile_x +
          NOMBRE_RAJOLES_HOR * (tile_amunt_esquirol + map_tile_y);

      if (es_casellaParet(casella_esquirol)) {
        // Si està en una paret, el marquem per recrear-lo
        esquirols[k].eliminar = 1;
      } else {
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
    
    // Check collision with main character
    if (esquirols[k].eliminar == 0 && esquirols[k].pintar == 1) {
      if (check_sprite_collision(x, y + pos_scroll_y, esquirols[k].x, esquirols[k].y, 8)) {
        collision_count++;
        update_main_character_color();
        
        // Mark squirrel for elimination after collision
        esquirols[k].eliminar = 1;
        esquirols[k].pintar = 0;
        push(&pila_plans_sprite, esquirols[k].num_pla_sprite);
      }
    }

    PutSprite(esquirols[k].num_pla_sprite, 1, esquirols[k].x, esquirols[k].y,
              0);

  }
}

void moviment_sprite(){
  while (Inkey()!=27)
  {
    if (processar_moviment == 1) {
      stick = JoystickRead(0);
      space = TriggerRead(0);

      if (stick == 1) {
        // Amunt
        moviment_amunt();
      }
      else if (stick == 2) {
        // Amunt dreta
        moviment_dreta();
        moviment_amunt();
      } else if (stick == 3) {
        // Dreta
        moviment_dreta();
      } else if (stick == 4) {
        // Avall Dreta
        moviment_dreta();
        moviment_avall();
      } else if (stick == 5) {
        // Avall
        moviment_avall();
      } else if (stick == 6) {
        // Avall Esquerra
        moviment_esquerra();
        moviment_avall();
      } else if (stick == 7) {
        // Esquerra
        moviment_esquerra();
      } else if (stick == 8) {
        // Amunt Esquerra
        moviment_esquerra();
        moviment_amunt();
      }
      PutSprite(0, 0, x, y+pos_scroll_y, 4); // Sumem el pos_scroll_y, ja que hi ha un offset cada cop que fem scroll

      processar_moviment = 0;

      if (processar_esquirols == 1) {
        actualitza_pos_esquirols();
        processar_esquirols = 0;
      }
    }
  }
}


void main(){
  Screen(5);
  SetDisplayPage(2);
  map_tile_x = 12;
  map_tile_y = 14;
  init_pantalla_joc();
  // Canviem el color 15 perquè sigui blanc i el text
  SetColorPalette(15 , 7, 7, 7);
  copsVsync = 0;
  processar_moviment=0;
  processar_esquirols=0;
  pos_scroll_x = 1;
  pos_scroll_y = 1; // L'inicialitzem amb 1 perquè així si vaig amunt també el detecta com a 0. 
  fer_scroll_lateral = 1;
  fer_scroll_vertical = 1;

  SetScrollMask(1);
  SetScrollH(1); // Comencem a la mateixa posició que el que hem après de scrpro.c

  InitializeMyInterruptHandler((int)main_loop, 1);
  // Posem sprite al 50,50
  x=20; y=19;
  PutSprite(0, 0, x, y, 4);
  moviment_sprite();
  EndMyInterruptHandler();
  Screen(0);
  RestorePalette();
  Locate(0, 0);
  printf("Pos sprite: %d, %d",x,y);
  Locate(0,1);

  printf("Tiles: %d, %d; %d-%d; %d-%d", tile_esq, tile_amunt,
         casella_1, casella_2,
         es_casellaParet(casella_1), es_casellaParet(casella_2));
  Locate(0,2);
  printf("scrl_x:%d;scrl_y;%d;", pos_scroll_x, pos_scroll_y);
  Locate(0, 3);
  printf("mptle_x:%d;frscrlat:%d;", map_tile_x, fer_scroll_lateral);
  Locate(0, 4);
  printf("mptle_y:%d;frscrver:%d;", map_tile_y, fer_scroll_vertical);
  Locate(0, 5);
  printf("scroll_x + x: %d; >>3: %d\n\r", pos_scroll_x + x,
         ((x + pos_scroll_x) & 255) >> 3);
  printf("desti: %d, %d\n\r", desti_x, desti_y);
  Locate(0, 6);
  printf("Collisions: %d\n\r", collision_count);
  Exit(0);
}

