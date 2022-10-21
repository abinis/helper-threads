#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define BITMASK_ARR_SZ 8

void set_bitmask (unsigned int *buf, int value)
{
   memset(buf, value, BITMASK_ARR_SZ*sizeof(unsigned int));
}

void parse_hexbuf (char *hex, unsigned int *nums)
{
   char buf[32];
   memset(buf, 0, 32);

   char padded_hex[64];
   memset(padded_hex, 0, 64);
   int offset = 8 - strlen(hex)%8;
   strncpy(padded_hex+offset, hex, strlen(hex));
   int i = 0;
   while ( i < offset )
      padded_hex[i++] = '0';

   int end = 0;
   while (padded_hex[end] != '\0')
     end++;

   int j = 0;
   for (i = end-8; i >= 0; i-=8) {
     strncpy(buf, padded_hex+i, 8);
     nums[j] = strtol(buf, NULL, 16);
     printf("buf: %s, parsed: %u\n", buf, nums[j]);
     j++;
     memset(buf, 0, 32);
   }

   /*
   const unsigned int msb_set = 0x80000000;
   for (i = 0; i < BITMASK_ARR_SZ; i++) {
      unsigned int num = nums[i];
	  printf("%10u:", num);
      int j = 0;
      while ( j++ < 32 ) {
         if ( (j-1) % 4 == 0 )
            printf(" ");
         printf("%c", ( num & msb_set ) ? '1' : '0');
         num <<= 1;
      }
      printf("\n");
   }
   */
}

void check_set_bits (unsigned int *nums)
{
   int i;
   const unsigned int msb_set = 0x80000000;
   for (i = 0; i < BITMASK_ARR_SZ; i++) {
      unsigned int num = nums[i];
	  printf("%10u:", num);
      int j = 0;
      while ( j++ < 32 ) {
         if ( (j-1) % 4 == 0 )
            printf(" ");
         printf("%c", ( num & msb_set ) ? '1' : '0');
         num <<= 1;
      }
      printf("\n");
   }
}

int main(int argc, char *argv[])
{
   char buf[32];

   printf("INT_MAX = %d\n", INT_MAX);
   //memset(buf, -1, 32);
   snprintf(buf, 32, "%d", -1);

   printf("buf atoi: %d\n", atoi(buf));
   puts(buf);

   char hex[] = "0caa126790000ff00e00bb0da0fffffc00000fffffc00000";
   char hexbuf[1024];
   memset(hexbuf, 0, 1024);
   strncpy(hexbuf, hex, strlen(hex));
   
   printf("  hexbuf: %s\n", hexbuf);

   strncpy(buf, hexbuf, sizeof(int));
   printf("     buf: %s\n", buf);

   int num = strtol(buf, NULL, 16);
   printf(" num(16): %x\n", num);

   //This should fail (return -1)
   int foo = strtol(hexbuf, NULL, 16);
   perror("what happened to strtol ?");
   printf(" foo(10): %d\n", foo);

   //Ok, int array it is
   int nums[BITMASK_ARR_SZ], i;
   memset(nums, 0, 16*sizeof(int));
   unsigned int hexbufsz = strlen(hexbuf);
   unsigned int numcount = hexbufsz/sizeof(int);
   printf("hexbufsz = %u\nnumcount = %u\n", hexbufsz, numcount);
   for (i = 0; i < numcount; i++) {
     int offset = 4*i;
     //printf("offset = %d\n", offset);
     strncpy(buf, hexbuf+offset, sizeof(int));
     nums[i] = strtol(buf, NULL, 16);
     printf("nums[%d] = %x\n", i, nums[i]);
     memset(buf, 0, 32);
   }

   for (i = 0; i < 16; i++)
     printf("%d ", nums[i]);
      
   printf("\n");

   //Trim starting zeros
   i = 0;
   char *p = hexbuf;
   while (hexbuf[i++] == '0')
     p++;

   unsigned int trimmedsz = strlen(p);
   printf("length of trimmed hexbuf, p = %u\n", trimmedsz);
   printf(" p: %s\n", p);
   
   //Parse hexbuf
   /*
   int end = 0;
   while (hexbuf[end] != '\0')
     end++;
   */
   unsigned int parsed[BITMASK_ARR_SZ];
   memset(parsed, 0, BITMASK_ARR_SZ*sizeof(long int));

   printf("checking set_bitmask function...\n");
   set_bitmask(parsed, 42);
   printf("seeing stars? ");
   char *cp = parsed;
   int cur_byte = 0;
   while (cur_byte < BITMASK_ARR_SZ*sizeof(unsigned int)) {
      printf("%c", *cp);
      cp++;
	  cur_byte++;
   }
   printf("\n");
   set_bitmask(parsed, 0);
   printf("checking parse_hexbuf function...\n");
   parse_hexbuf(hexbuf, parsed);
   printf("printing parsed numbers bit-by-bit...\n");
   check_set_bits(parsed);

   //Another hex string (shorter)
   char hex_short[] = "03f03f";
   memset(hexbuf, 0, 1024);
   strncpy(hexbuf, hex_short, strlen(hex_short));
   printf("parsing short hex string, checking...\n");
   set_bitmask(parsed, 0);
   parse_hexbuf(hexbuf, parsed);
   check_set_bits(parsed);
   printf("...seems ok\n");
   
   /*
   int j = 0;
   for (i = end-sizeof(int); i >= 0; i-=sizeof(int)) {
     strncpy(buf, hexbuf+i, sizeof(int));
     parsed[j] = strtol(buf, NULL, 16);
     printf("buf: %s, parsed: %d\n", buf, parsed[j]);
     j++;
     memset(buf, 0, 32);
   }
   */


   //print char as individual bits
   char hexdigit = 'f';
   while (hexdigit != 0 ) {
     printf("%c ", (hexdigit & 1) ? '1' : '0');
     hexdigit >>= 1;
   }
   printf("\n");

   //expand hexes to bits
   
   //check memset NULL pointer
   //memset(NULL, 0, 8);
   buf[0] = '\0';
   printf("      checking zero-setting: %c\n", buf[0]);
   memset(buf, 0, 32);
   printf("checking memsetting to zero: %s\n", buf);

   unsigned int test_msb = 0;
   test_msb = 1<<31;
   
   printf("msb set: %u\n", test_msb);

   return 0;
}
