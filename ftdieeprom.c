/*

    BACKUP and FLASH the eeprom of an ftdi device via libftdi

    Copyright Ricardo Ribalda - 2014 - ricardo.ribalda@gmail.com
    Copyright Altynbek Isabekov - 2017 - aisabekov@ku.edu.tr

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, FLASH to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <ftdi.h>
#include <libusb.h>
#include <inttypes.h>
#define EEPROM_SIZE 256

enum {FLASH,BACKUP};

void use(char *name){
	fprintf(stdout, "To backup EEPROM into OutFile\n");
	fprintf(stdout,"                  %s -r -o OutFile\n",name);
	fprintf(stdout, "To flash EEPROM using InFile\n");
	fprintf(stdout,"                  %s -w -i InFile\n",name);
	return;
}

static unsigned long unsigned_val (const char *arg, unsigned long max)
{
	unsigned long val;

	errno = 0;
	val = strtoul(arg, NULL, 0);
	if (errno || val > max) {
		fprintf(stderr, "%s: bad value (max=0x%lx)\n", arg, max);
		exit(EINVAL);
	}
	return val;
}

static int fix_csum(unsigned char *eeprom,int len){
	int i;
	uint16_t csum=0xaaaa;
	uint16_t old_csum=eeprom[len-1]<<8 | eeprom[len-2];

	for (i=0;i<(len/2)-1;i++){
		uint16_t value;
		value=eeprom[i*2]+((eeprom[i*2+1])<<8);

		csum=value^csum;
		csum= (csum<<1)|(csum>>15);
	}

	if (csum!=old_csum){
		fprintf(stderr,"Old csum 0x%.4x, new csum 0x%4.x\n",old_csum,csum);
		eeprom[len-2]=csum;
		eeprom[len-1]=csum>>8;
	}

	return 0;
}

int main(int argc, char *argv[]){
	struct ftdi_context ftdi;
	unsigned char eeprom_buf[EEPROM_SIZE];
	int ret;
	int iflag=0, oflag = 0;
	FILE *file;
	int mode=BACKUP;
	char c;
	const char *iFileName = malloc(100);
    const char *oFileName = malloc(100);
	uint16_t  vid=0;
	uint16_t  pid=0;

	while ((c = getopt (argc, argv, "rwi:o:v:p:")) != -1)
		switch (c)
		  {
		  case 'r':
			mode = BACKUP;
		    break;
		  case 'w':
		    mode = FLASH;
		    break;
		  case 'i':
			iflag = 1;
		    iFileName = optarg;
		    break;
		  case 'o':
			oflag = 1;
		    oFileName = optarg;
		    break;
		  case 'v':			
			vid = unsigned_val(optarg, 0xffff);
		    break;
		  case 'p':
		    pid = unsigned_val(optarg, 0xffff);
		    break;
		  case '?':
		    use(argv[0]);
		    abort ();
		  default:
		    abort ();
		  }

	ftdi_init(&ftdi);
	ftdi.eeprom_size=EEPROM_SIZE;
	ret=ftdi_usb_open(&ftdi, vid, pid);
	if (ret){
		printf("Error: %s\n", ftdi.error_str);
		fclose(file);
		return -1;
	}

	if (mode==BACKUP && oflag == 1){
		if (oFileName != NULL && strlen(oFileName) > 0){
			file = fopen(oFileName, "wb");
			ret=ftdi_read_eeprom(&ftdi, eeprom_buf);
			fwrite(eeprom_buf,sizeof(eeprom_buf),1,file);
		}
	}
	else if(mode==FLASH && iflag == 1){
		if (iFileName != NULL && strlen(iFileName) > 0){
			file = fopen(iFileName, "rb");
			fread(eeprom_buf,sizeof(eeprom_buf),1,file);
			fix_csum(eeprom_buf,sizeof(eeprom_buf));
			ftdi_erase_eeprom(&ftdi);
			ftdi_write_eeprom(&ftdi, eeprom_buf);
			usb_reset(ftdi.usb_dev);
		}
	}

	fclose(file);
	ftdi_deinit (&ftdi);

	return 0;
}
