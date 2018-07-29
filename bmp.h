#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int HEADER_SIZE = 0x3A;
const int R = 0;
const int G = 1;
const int B = 2;
const int BLUR_GRID_ROW = 7;
const int BLUR_GRID_SIZE = 49;

//structs to hold BMP info/data
struct BMP_Header
{
    unsigned short header;
    unsigned int size;
    unsigned int spec;      //program that created this specific (0x0 here)
    unsigned int offset;    //versace
};
struct DIB_Header
{
    unsigned int header_size;
    int width;
    int height;
    unsigned short one;     //always 0x1
    unsigned short depth;
    unsigned int compression;
    unsigned int data_size;
    int horizontal_resolution;
    int vertical_resolution;
    //8 Bytes unused here   //zeroes here
    unsigned int colorspace;
    //64Bytes unused here   //zeroes here
};
struct BMP_Data
{
    struct BMP_Header bmp;
    struct DIB_Header dib;
    unsigned char* data[3];
};

//Basic Tools
void BMP_Load(struct BMP_Data* bmp, const char* filename);
void BMP_Save(struct BMP_Data* bmp, const char* filename);
void BMP_Print(struct BMP_Data bmp);
void BMP_Destroy(struct BMP_Data* bmp);
//filters
void BMP_ModGrayscale(struct BMP_Data* bmp);
void BMP_ModBW(struct BMP_Data* bmp);
void BMP_ModRed(struct BMP_Data* bmp);
void BMP_ModGreen(struct BMP_Data* bmp);
void BMP_ModBlue(struct BMP_Data* bmp);
void BMP_ModBlur(struct BMP_Data* bmp);
//ktp
int BMP_GetColor(struct BMP_Data* bmp);

//Basic Tools
void BMP_Load(struct BMP_Data* bmp, const char* filename)
{
    //open file for reading binary
    FILE* file = fopen(filename, "rb");
    //error checking for fopen
    if (file == NULL)
    {
        printf("Error: failed to open %s\n", filename);
        return;
    }
    unsigned char header_buffer[HEADER_SIZE]; //to hold all file header contents
    memset(header_buffer, 0, HEADER_SIZE);
    memset(&bmp->bmp, 0, sizeof(bmp->bmp));
    memset(&bmp->dib, 0, sizeof(bmp->dib));
    int i;
    for (i = 0; i < HEADER_SIZE; ++i)
        header_buffer[i] = (unsigned char)fgetc(file);  //get header data

    //load BMP_Header
    bmp->bmp.header = (header_buffer[0] << 8) | header_buffer[1];
    bmp->bmp.size = (header_buffer[2] << 0*8) | (header_buffer[3] << 1*8) | (header_buffer[4] << 2*8) | (header_buffer[5] << 3*8);
    bmp->bmp.spec = (header_buffer[6] << 0*8) | (header_buffer[7] << 1*8) | (header_buffer[8] << 2*8) | (header_buffer[9] << 3*8);
    bmp->bmp.offset = (header_buffer[10] << 0*8) | (header_buffer[11] << 1*8) | (header_buffer[12] << 2*8) | (header_buffer[13] << 3*8);
    //load DIB_Header
    bmp->dib.header_size = (header_buffer[14] << 0*8) | (header_buffer[15] << 1*8) | (header_buffer[16] << 2*8) | (header_buffer[17] << 3*8);
    bmp->dib.width = (header_buffer[18] << 0*8) | (header_buffer[19] << 1*8) | (header_buffer[20] << 2*8) | (header_buffer[21] << 3*8);
    bmp->dib.height = (header_buffer[22] << 0*8) | (header_buffer[23] << 1*8) | (header_buffer[24] << 2*8) | (header_buffer[25] << 3*8);
    bmp->dib.one = (header_buffer[26] << 0*8) | (header_buffer[27] << 1*8);
    bmp->dib.depth = (header_buffer[28] << 0*8) | (header_buffer[29] << 1*8);
    bmp->dib.compression = (header_buffer[30] << 0*8) | (header_buffer[31] << 1*8) | (header_buffer[32] << 2*8) | (header_buffer[33] << 3*8);
    bmp->dib.data_size = (header_buffer[34] << 0*8) | (header_buffer[35] << 1*8) | (header_buffer[36] << 2*8) | (header_buffer[37] << 3*8);
    bmp->dib.horizontal_resolution = (header_buffer[38] << 0*8) | (header_buffer[39] << 1*8) | (header_buffer[40] << 2*8) | (header_buffer[41] << 3*8);
    bmp->dib.vertical_resolution = (header_buffer[42] << 0*8) | (header_buffer[43] << 1*8) | (header_buffer[44] << 2*8) | (header_buffer[45] << 3*8);
    bmp->dib.colorspace = (header_buffer[54] << 0*8) | (header_buffer[55] << 1*8) | (header_buffer[56] << 2*8) | (header_buffer[57] << 3*8);
    //load data
    bmp->data[R] = (unsigned char*)malloc(bmp->dib.data_size/3);    //allocate for reds
    bmp->data[G] = (unsigned char*)malloc(bmp->dib.data_size/3);    //allocate for greens
    bmp->data[B] = (unsigned char*)malloc(bmp->dib.data_size/3);    //allocate for blues
    fseek(file, bmp->bmp.offset, SEEK_SET);     //go to where data starts
    for (i = 0; i < bmp->dib.data_size/3; ++i)    //get pixel data
    {
        //order reversed for little-endianness
        bmp->data[B][i] = (unsigned char)fgetc(file);
        bmp->data[G][i] = (unsigned char)fgetc(file);
        bmp->data[R][i] = (unsigned char)fgetc(file);
    }

    //close file
    fclose(file);
}
void BMP_Save(struct BMP_Data* bmp, const char* filename)
{
    FILE* file = fopen(filename, "wb");
    if (file == NULL)
    {
        printf("Error: failed to open %s\n", filename);
        return;
    }
    unsigned char header_buffer[HEADER_SIZE]; //to hold all file header contents
    memset(header_buffer, 0, HEADER_SIZE);
    int i;

    //load BMP_Header into buffer
    header_buffer[0] = (0x0000FF00 & bmp->bmp.header) >> 1*8;
    header_buffer[1] = (0x000000FF & bmp->bmp.header) >> 0*8;
    header_buffer[2] = (0x000000FF & bmp->bmp.size) >> 0*8;
    header_buffer[3] = (0x0000FF00 & bmp->bmp.size) >> 1*8;
    header_buffer[4] = (0x00FF0000 & bmp->bmp.size) >> 2*8;
    header_buffer[5] = (0xFF000000 & bmp->bmp.size) >> 3*8;
    header_buffer[6] = (0x000000FF & bmp->bmp.spec) >> 0*8;
    header_buffer[7] = (0x0000FF00 & bmp->bmp.spec) >> 1*8;
    header_buffer[8] = (0x00FF0000 & bmp->bmp.spec) >> 2*8;
    header_buffer[9] = (0xFF000000 & bmp->bmp.spec) >> 3*8;
    header_buffer[10] = (0x000000FF & bmp->bmp.offset) >> 0*8;
    header_buffer[11] = (0x0000FF00 & bmp->bmp.offset) >> 1*8;
    header_buffer[12] = (0x00FF0000 & bmp->bmp.offset) >> 2*8;
    header_buffer[13] = (0xFF000000 & bmp->bmp.offset) >> 3*8;
    //load DIB_Header into buffer
    header_buffer[14] = (0x000000FF & bmp->dib.header_size) >> 0*8;
    header_buffer[15] = (0x0000FF00 & bmp->dib.header_size) >> 1*8;
    header_buffer[16] = (0x00FF0000 & bmp->dib.header_size) >> 2*8;
    header_buffer[17] = (0xFF000000 & bmp->dib.header_size) >> 3*8;
    header_buffer[18] = (0x000000FF & bmp->dib.width) >> 0*8;
    header_buffer[19] = (0x0000FF00 & bmp->dib.width) >> 1*8;
    header_buffer[20] = (0x00FF0000 & bmp->dib.width) >> 2*8;
    header_buffer[21] = (0xFF000000 & bmp->dib.width) >> 3*8;
    header_buffer[22] = (0x000000FF & bmp->dib.height) >> 0*8;
    header_buffer[23] = (0x0000FF00 & bmp->dib.height) >> 1*8;
    header_buffer[24] = (0x00FF0000 & bmp->dib.height) >> 2*8;
    header_buffer[25] = (0xFF000000 & bmp->dib.height) >> 3*8;
    header_buffer[26] = (0x000000FF & bmp->dib.one) >> 0*8;
    header_buffer[27] = (0x0000FF00 & bmp->dib.one) >> 1*8;
    header_buffer[28] = (0x000000FF & bmp->dib.depth) >> 0*8;
    header_buffer[29] = (0x0000FF00 & bmp->dib.depth) >> 1*8;
    header_buffer[30] = (0x000000FF & bmp->dib.compression) >> 0*8;
    header_buffer[31] = (0x0000FF00 & bmp->dib.compression) >> 1*8;
    header_buffer[32] = (0x00FF0000 & bmp->dib.compression) >> 2*8;
    header_buffer[33] = (0xFF000000 & bmp->dib.compression) >> 3*8;
    header_buffer[34] = (0x000000FF & bmp->dib.data_size) >> 0*8;
    header_buffer[35] = (0x0000FF00 & bmp->dib.data_size) >> 1*8;
    header_buffer[36] = (0x00FF0000 & bmp->dib.data_size) >> 2*8;
    header_buffer[37] = (0xFF000000 & bmp->dib.data_size) >> 3*8;
    header_buffer[38] = (0x000000FF & bmp->dib.horizontal_resolution) >> 0*8;
    header_buffer[39] = (0x0000FF00 & bmp->dib.horizontal_resolution) >> 1*8;
    header_buffer[40] = (0x00FF0000 & bmp->dib.horizontal_resolution) >> 2*8;
    header_buffer[41] = (0xFF000000 & bmp->dib.horizontal_resolution) >> 3*8;
    header_buffer[42] = (0x000000FF & bmp->dib.vertical_resolution) >> 0*8;
    header_buffer[43] = (0x0000FF00 & bmp->dib.vertical_resolution) >> 1*8;
    header_buffer[44] = (0x00FF0000 & bmp->dib.vertical_resolution) >> 2*8;
    header_buffer[45] = (0xFF000000 & bmp->dib.vertical_resolution) >> 3*8;
    header_buffer[54] = (0x000000FF & bmp->dib.colorspace) >> 0*8;
    header_buffer[55] = (0x0000FF00 & bmp->dib.colorspace) >> 1*8;
    header_buffer[56] = (0x00FF0000 & bmp->dib.colorspace) >> 2*8;
    header_buffer[57] = (0xFF000000 & bmp->dib.colorspace) >> 3*8;
    //write all to file
    for (i = 0; i < HEADER_SIZE; ++i)
        fputc((unsigned char)header_buffer[i], file);
    for (i = 0; i < bmp->bmp.offset - HEADER_SIZE; ++i)       //extra zeroes for stuff irrelevant for sRGB
        fputc(0, file);
    //write pixel data to file
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        //order reversed for little-endianness
        fputc((unsigned char)bmp->data[B][i], file);
        fputc((unsigned char)bmp->data[G][i], file);
        fputc((unsigned char)bmp->data[R][i], file);
    }

    //close file
    fclose(file);
}
void BMP_Print(struct BMP_Data bmp)
{
    //BMP Header
    printf("Header: %c%c\nSize: %X\nSpec: %X\nOffset: %X\n", (0xFF00&bmp.bmp.header)>>8, (0xFF&bmp.bmp.header), bmp.bmp.size, bmp.bmp.spec, bmp.bmp.offset);
    //DIB Header
    printf("Header_Size: %X\nWidth: %X\nHeight: %X\nOne: %X\nDepth: %X\nCompression: %X\nData_Size: %X\nHRes: %X\nVRes: %X\nColorspace: %c%c%c%c\n", bmp.dib.header_size, bmp.dib.width, bmp.dib.height, bmp.dib.one, bmp.dib.depth, bmp.dib.compression, bmp.dib.data_size, bmp.dib.horizontal_resolution, bmp.dib.vertical_resolution, (0xFF000000&bmp.dib.colorspace)>>3*8, (0xFF0000&bmp.dib.colorspace)>>2*8, (0xFF00&bmp.dib.colorspace)>>1*8, (0xFF&bmp.dib.colorspace)>>0*8);
}
void BMP_Destroy(struct BMP_Data* bmp)
{
    free(bmp->data[R]);
    free(bmp->data[G]);
    free(bmp->data[B]);
}
//filters
void BMP_ModGrayscale(struct BMP_Data* bmp)
{
    int i;      //iterator
    int avg;    //to hold grayscale average
    //set each pixels RGB to the same based on average
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        avg = (bmp->data[R][i] + bmp->data[G][i] + bmp->data[B][i])/3;

        bmp->data[R][i] = avg;
        bmp->data[G][i] = avg;
        bmp->data[B][i] = avg;
    }
}
void BMP_ModBW(struct BMP_Data* bmp)
{
    int i;      //iterator
    int avg;    //to hold average
    //set each pixels RGB to max or min based on average
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        
        avg = (bmp->data[R][i] + bmp->data[G][i] + bmp->data[B][i])/3;

        if (avg < 0x80)
        {
            bmp->data[R][i] = 0x0;
            bmp->data[G][i] = 0x0;
            bmp->data[B][i] = 0x0;
        }
        else
        {
            bmp->data[R][i] = 0xFF;
            bmp->data[G][i] = 0xFF;
            bmp->data[B][i] = 0xFF;
        }
    }
}
void BMP_ModRed(struct BMP_Data* bmp)
{
    int i;      //iterator
    //increase red in each pixel
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        if(bmp->data[R][i] >= 0xA0)
            bmp->data[R][i] = 0xFF;
        else
            bmp->data[R][i] += 0x60;
    }
}
void BMP_ModGreen(struct BMP_Data* bmp)
{
    int i;      //iterator
    //increase green in each pixel
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        if(bmp->data[G][i] >= 0xA0)
            bmp->data[G][i] = 0xFF;
        else
            bmp->data[G][i] += 0x60;
    }
}
void BMP_ModBlue(struct BMP_Data* bmp)
{
    int i;      //iterator
    //increase blue in each pixel
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        if(bmp->data[B][i] >= 0xA0)
            bmp->data[B][i] = 0xFF;
        else
            bmp->data[B][i] += 0x60;
    }
}
void BMP_ModBlur(struct BMP_Data* bmp)
{
//For now off image at top/bottom assume missing values are black
// also left/right edges wrap
    int i, j;      //iterators
    //copy data so all operations are based on original unmodified pixels
    unsigned char* orig[3];
    orig[R] = (unsigned char*)malloc(bmp->dib.data_size/3);
    orig[G] = (unsigned char*)malloc(bmp->dib.data_size/3);
    orig[B] = (unsigned char*)malloc(bmp->dib.data_size/3);
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        orig[R][i] = bmp->data[R][i];
        orig[G][i] = bmp->data[G][i];
        orig[B][i] = bmp->data[B][i];
    }

    //Blur each pixel based on itself and its neighbors
    unsigned char adj[3][BLUR_GRID_SIZE];   //holds grid of pixels centered on current
    double w[BLUR_GRID_SIZE];     //holds weights of grid elements
        //outer square
        w[0] = w[6] = w[42] = w[48] = 0.005084; //corners
        w[1] = w[5] = w[7] = w[13] = w[35] = w[41] = w[43] = w[47] = 0.009377; //next to corners
        w[2] = w[4] = w[14] = w[20] = w[28] = w[34] = w[44] = w[46] = 0.013539; //inner
        w[3] = w[21] = w[27] = w[45] = 0.015302; //max edgeness
        //middle square
        w[8] = w[12] = w[36] = w[40] = 0.017296; //corners
        w[9] = w[11] = w[15] = w[19] = w[29] = w[33] = w[37] = w[39] = 0.024972; //next to corners
        w[10] = w[22] = w[26] = w[38] = 0.017296; //max edgeness
        //inner square
        w[16] = w[18] = w[30] = w[32] = 0.036054; //corners
        w[17] = w[23] = w[25] = w[31] = 0.040749; //max edgeness
        //current pixel
        w[24] = 0.046056;
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        //clear all
        bmp->data[R][i] = 0;
        bmp->data[G][i] = 0;
        bmp->data[B][i] = 0;
        //set adjacent pixel matrix
        for (j = 0; j < BLUR_GRID_SIZE; ++j)
        {
            //set modifier based on location relative to current pixel
            int mod = 0;
            mod += ((int)bmp->dib.width * ((BLUR_GRID_ROW/2) - j/BLUR_GRID_ROW)); //for row offset
            mod += (j%BLUR_GRID_ROW - (BLUR_GRID_ROW/2)); //for column offset
            //just a little bounds checking
            // because nobody wants a segfault
            if ((i + mod) >= bmp->dib.data_size/3 || (i + mod) < 0)
            {
                //R
                adj[R][j] = 0xFF;
                //G
                adj[G][j] = 0xFF;
                //B
                adj[B][j] = 0xFF;
            }
            else //all is well then
            {
                //R
                adj[R][j] = orig[R][i + mod];
                //G
                adj[G][j] = orig[G][i + mod];
                //B
                adj[B][j] = orig[B][i + mod];
            }
        }
        //blur the rest
        for (j = 0; j < BLUR_GRID_SIZE; ++j)
        {
            //R
            bmp->data[R][i] += (unsigned char)((double)adj[R][j] * w[j]);
            //G
            bmp->data[G][i] += (unsigned char)((double)adj[G][j] * w[j]);
            //B
            bmp->data[B][i] += (unsigned char)((double)adj[B][j] * w[j]);
        }
    }

    //free extra data
    free(orig[R]);
    free(orig[G]);
    free(orig[B]);
}
//ktp
int BMP_GetColor(struct BMP_Data* bmp)
{
    int i = 0;      //iterator
    int sum[3] = { 0, 0, 0 };    //self-explanatory

    //sum data
    for (i = 0; i < bmp->dib.data_size/3; ++i)
    {
        sum[R] += bmp->data[R][i];
        sum[G] += bmp->data[G][i];
        sum[B] += bmp->data[B][i];
    }

    //make avg from sum
    sum[R] /= (bmp->dib.data_size/3);
    sum[G] /= (bmp->dib.data_size/3);
    sum[B] /= (bmp->dib.data_size/3);

    //return combination of avgs = 0xRRGGBB
    return ((sum[R] << 16) + (sum[G] << 8) + (sum[B] << 0));
}
