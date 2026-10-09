#include <stdio.h>
#include <stdlib.h>
#include <memory.h>



void main(int argc,char*argv[])
{
	FILE* fp;
	int i,length;
	char linebuf[16];

	if (argc != 2)
	{
		printf("Usage: hexdump <filename>\n");
		exit(-1);
	}

	fp = fopen(argv[1],"rb");
	if (!fp)
	{
		printf("Unable to open %s\n",argv[1]);
	}

	while (!feof(fp))
	{
//		printf("%06x:",ftell(fp));

		memset(linebuf,0,sizeof(linebuf));
		length = fread(linebuf,1,sizeof(linebuf),fp);
		for (i=0;i<length;i++)
		{
			printf("0x%02x,",(unsigned char)linebuf[i]);
		}

#if 0
		for (i=0;i<length;i++)
		{
			char ch;
			char lastch;

			ch = linebuf[i];

			if ((ch=='\n') && (lastch != '\n') )
			{
				printf("\n");
			}

			if ( (ch==0x0) && (lastch != 0x00) )
			{
				printf("\n");
			}

			if ( (ch>=' ') && (ch < 0x7f) )
			{
				printf("%c",linebuf[i]);
			}

			lastch = ch;
		}
#endif
		printf("\n");
	}
	fclose(fp);
}