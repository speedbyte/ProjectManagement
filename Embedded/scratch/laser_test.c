#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void)
{
char buf[7];
int distance = 0;
short swapped=0;
int i=0;
FILE *fp;

for (i=0; i<10; i++)
{
system("i2cset -y 0 0x62 0x00 0x04 ");
usleep(100);
FILE* file = popen("i2cget -y 0 0x62 0x8f w","r");
fgets(buf,sizeof(buf), file);

//puts(buf);
pclose(file);

//printf("%s\n",buf);


distance = strtol(buf, NULL, 0);
//printf("before swapped hex: %lx\n",distance);
swapped = (distance>>8)| (distance<<8);
//printf("Distance: %lx cm\n",swapped);

fp = fopen("mess.txt", "a");
fprintf(fp, "%d\n", swapped);






}
fprintf(fp, "\n");
fclose(fp);
}
