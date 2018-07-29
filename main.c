#include "bmp.h"

int main(int argc, char** argv)
{
    if (argc != 2) { printf( "Give me a file\n" ); return -1; }
    struct BMP_Data img;
    BMP_Load( &img, argv[1] );
    printf( "Average color of picture is 0x%X\n", BMP_GetColor( &img ) );
    BMP_Destroy( &img );
    return 0;
}
