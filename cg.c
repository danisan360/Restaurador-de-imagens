/*
                           
 Universidade Federal da Bahia             
 Computação Grafica        
                                               
 Daniel Lopes                    
 Daniel Fernandes                   

*/
//This program loads 3-channel BMP
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// bitmap header structure, has to be packed to avoid compiler padding
#pragma pack(1)
typedef struct BITMAPFILEHEADER {
  char magic[2];       // "BM" 0x424d - char[2] to avoid endian problems
  uint32_t filesize;   // size of the bitmap file (data + headers)
  uint16_t reserved1;  // must be 0
  uint16_t reserved2;  // must be 0
  uint32_t dataoffset; // when does the data start
} BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER {
  uint32_t headersize;      // size of this header
  int32_t width;            // width of the bmp
  int32_t height;           // height of the bmp
  uint16_t colorplanes;     // must be 1
  uint16_t bitdepth;        // bits per pixel
  uint32_t compression;     // 0 = uncompressed
  uint32_t imagesize;       // can be 0 if bmp is uncompressed
  int32_t hresolution;      // print resolution
  int32_t vresolution;      // print resolution
  uint32_t palettecolors;   // can be 0 if uncompressed
  uint32_t importantcolors; // number of important colors, 0 = all are important
} BITMAPINFOHEADER;

typedef struct BITMAPFULLHEADER {
  BITMAPFILEHEADER fileinfo;
  BITMAPINFOHEADER bmpinfo;
} BITMAPFULLHEADER;
#pragma pack(0)

#define M_PI 3.14159265358979323846

// BMP HEADER
BITMAPFULLHEADER header;

// image data
unsigned char *data;               // loaded image
unsigned char *data_filtro;        // loaded image
unsigned char *data_equaliza;      // loaded image

float Gx(float ro, float x){
  return (1/(2*M_PI*ro*ro))*exp((-(x*x))/(2*ro*ro));
}


// dynamic programming to improve performance
int loadBMP(const char *imagepath) {
  // Open the file
  FILE *file = fopen(imagepath, "rb");
  if (!file) {
    printf("Image could not be opened\n");
    return 0;
  }

  // Read header
  if (fread(&header, 1, sizeof(BITMAPFULLHEADER), file) !=
      54) { // If not 54 bytes read : problem
    printf("Not a correct BMP file - Wrong header size\n");
    return 1;
  }

  if (header.fileinfo.magic[0] != 'B' || header.fileinfo.magic[1] != 'M') {
    printf("Not a correct BMP file - Wrong magic number\n");
    return 1;
  }

  // Read ints from the header
  unsigned long dataPos = header.fileinfo.dataoffset;
  unsigned long imageSize = header.bmpinfo.imagesize;
  long width = header.bmpinfo.width;
  long height = header.bmpinfo.height;

  // Some BMP files are misformatted, guess missing information
  if (imageSize == 0) {
    imageSize = width * height * 3;
  }
  if (dataPos == 0) {
    dataPos = 54; // The BMP header is usually done that way
  }

  // Allocate buffer
  data = (unsigned char *)malloc(imageSize);
  data_filtro = (unsigned char *)malloc(imageSize);
  data_equaliza = (unsigned char *)malloc(imageSize);

  // Read the actual data from the file into the buffer
  fseek(file, dataPos, SEEK_SET);
  if (!fread(data, 1, imageSize, file)) {
    printf("Couldn't read BMP Data. Giving up.\n");
    return 1;
  }

  // copying components BGR to RGB
  unsigned char B,R;
  for(int i = 0; i < width * height ; i++){
   int index = i*3;
   B = data[index];
   R = data[index+2];
   data[index] = R;
   data[index+2] = B;
  }

  
  FILE* saida;
  
  //filtro bilateral
  file = fopen(imagepath, "rb");
  fseek(file, dataPos, SEEK_SET);
  fread(data_filtro, 1, imageSize, file);

  for(int i = 0; i < width * height ; i++){
   int index = i*3;
   B = data_filtro[index];
   R = data_filtro[index+2];
   data_filtro[index] = R;
   data_filtro[index+2] = B;
  }

  

  saida = fopen("saida_bilateral.bmp","wb");
  fwrite(&header, sizeof(BITMAPFULLHEADER), sizeof(header), saida);
  fseek(saida, dataPos, SEEK_SET);
  fwrite(data_filtro, imageSize, 1,saida);

  //Equalização
  file = fopen("saida_bilateral.bmp","rb");
  fseek(file, dataPos, SEEK_SET);
  fread(data_equaliza, 1, imageSize, file);

  for(int i = 0; i < width * height ; i++){
   int index = i*3;
   B = data_equaliza[index];
   R = data_equaliza[index+2];
   data_equaliza[index] = R;
   data_equaliza[index+2] = B;
  }

  int hist[256] = { 0 }; 
  int new_hist[256] = { 0 }; 
  int heightImage = header.bmpinfo.height;
  int widthImage = header.bmpinfo.width;
  int total_pixels = height * width;


  for(int i = 0; i < width * height; i++){
   int index = i*3;
   int media = ((int)data_equaliza[index] + (int)data_equaliza[index+1] + (int)data_equaliza[index+2])/3;
   hist[media] += 1;
  }

  int curr = 0;

  for(int i = 0; i < 256; i++){
    curr += hist[i];
    new_hist[i] = round((((float)curr) * 255) / total_pixels);
  }

  for(int i = 0; i < width * height * 3; i++){
   data_equaliza[i] = ((unsigned char)new_hist[data_equaliza[i]]);
  }

  saida = fopen("saida_bilateral_e_equalizada.bmp","wb");
  fwrite(&header, sizeof(BITMAPFULLHEADER), sizeof(header), saida);
  fseek(saida, dataPos, SEEK_SET);
  fwrite(data_equaliza, imageSize, 1,saida);


  fclose(saida);
  fclose(file);

  return 0;
}

void init(void) { 
  glClearColor(0.0, 0.0, 0.0, 0.0); 
}

void display(void) {
  long width = header.bmpinfo.width;
  long height = header.bmpinfo.height;

  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0, 0);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
  glRasterPos2i(width, 0);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data_filtro);
  glRasterPos2i(width * 2, 0);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, data_equaliza);
  glFlush();
}

void reshape(int w, int h) {
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, 0, h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
  case 27:
    exit(0);
  }
}

int main(int argc, char **argv) {
  loadBMP(argv[1]);
  long width = header.bmpinfo.width;
  long height = header.bmpinfo.height;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(width * 3, height);
  glutInitWindowPosition(100, 100);
  glutCreateWindow(argv[0]);
  init();
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutDisplayFunc(display);
  glutMainLoop();
  return 0;
}
