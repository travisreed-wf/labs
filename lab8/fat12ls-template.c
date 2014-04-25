/* fat12ls.c
*
*  Displays the files in the root sector of an MSDOS floppy disk
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#define SIZE 32      /* size of the read buffer */
#define ROOTSIZE 256 /* max size of the root directory */
//define PRINT_HEX   // un-comment this to print the values in hex for debugging

struct BootSector
{
    unsigned char  sName[9];            // The name of the volume
    unsigned short iBytesSector;        // Bytes per Sector

    unsigned char  iSectorsCluster;     // Sectors per Cluster
    unsigned short iReservedSectors;    // Reserved sectors
    unsigned char  iNumberFATs;         // Number of FATs

    unsigned short iRootEntries;        // Number of Root Directory entries
    unsigned short iLogicalSectors;     // Number of logical sectors
    unsigned char  xMediumDescriptor;   // Medium descriptor

    unsigned short iSectorsFAT;         // Sectors per FAT
    unsigned short iSectorsTrack;       // Sectors per Track
    unsigned short iHeads;              // Number of heads

    unsigned short iHiddenSectors;      // Number of hidden sectors
};

void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[]);
//  Pre: Calculated directory offset and number of directory entries
// Post: Prints the filename, time, date, attributes and size of each entry

unsigned short endianSwap(unsigned char one, unsigned char two);
//  Pre: Two initialized characters
// Post: The characters are swapped (two, one) and returned as a short

void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[]);
//  Pre: An initialized BootSector struct and a pointer to an array
//       of characters read from a BootSector
// Post: The BootSector struct is filled out from the buffer data

char * toDOSName(char string[], unsigned char buffer[], int offset);
//  Pre: String is initialized with at least 12 characters, buffer contains
//       the directory array, offset points to the location of the filename
// Post: fills and returns a string containing the filename in 8.3 format

char * parseAttributes(char string[], unsigned char key);
//  Pre: String is initialized with at least five characters, key contains
//       the byte containing the attribue from the directory buffer
// Post: fills the string with the attributes

char * parseTime(char string[], unsigned short usTime);
//  Pre: string is initialzied for at least 9 characters, usTime contains
//       the 16 bits used to store time
// Post: string contains the formatted time

char * parseDate(char string[], unsigned short usDate);
//  Pre: string is initialized for at least 13 characters, usDate contains
//       the 16 bits used to store the date
// Post: string contains the formatted date

int roundup512(int number);
// Pre: initialized integer
// Post: number rounded up to next increment of 512


// reads the boot sector and root directory
int main(int argc, char * argv[])
{
    int pBootSector = 0;
    unsigned char buffer[SIZE];
    unsigned char rootBuffer[ROOTSIZE * 32];
    struct BootSector sector;
    int iRDOffset = 0;

    // Check for argument
    if (argc < 2) {
    	printf("Specify boot sector\n");
    	exit(1);
    }

    // Open the file and read the boot sector
    pBootSector = open(argv[1], O_RDONLY);
    read(pBootSector, buffer, SIZE);

    // Decode the boot Sector
    decodeBootSector(&sector, buffer);

    // Calculate the location of the root directory
    iRDOffset = (1 + (sector.iSectorsFAT * sector.iNumberFATs) )
                 * sector.iBytesSector;

    // Read the root directory into buffer
    lseek(pBootSector, iRDOffset, SEEK_SET);
    read(pBootSector, rootBuffer, ROOTSIZE);
    close(pBootSector);

    // Parse the root directory
    parseDirectory(iRDOffset, sector.iRootEntries, rootBuffer);

} // end main


// Converts two characters to an unsigned short with two, one
unsigned short endianSwap(unsigned char one, unsigned char two)
{
 	unsigned short s_one = (unsigned short) one;
	unsigned short s_two = (unsigned short) two;
	s_two = s_two << 8;
	return s_one | s_two;
}


// Fills out the BootSector Struct from the buffer
void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[])
{
	int i = 3;  // Skip the first 3 bytes
	// Pull the name and put it in the struct (remember to null-terminate)
	pBootS->sName[0] = buffer[3];
	pBootS->sName[1] = buffer[4];
	pBootS->sName[2] = buffer[5];
	pBootS->sName[3] = buffer[6];
	pBootS->sName[4] = buffer[7];
	pBootS->sName[5] = buffer[8];
	pBootS->sName[6] = buffer[9];
	pBootS->sName[7] = buffer[10];

	// Read bytes/sector and convert to big endian
        pBootS->iBytesSector = endianSwap(buffer[11], buffer[12]);

	// Read sectors/cluster, Reserved sectors and Number of Fats
        pBootS->iSectorsCluster = buffer[13];
	pBootS->iReservedSectors = endianSwap(buffer[14], buffer[15]);
	pBootS->iNumberFATs = buffer[16];

	// Read root entries, logicical sectors and medium descriptor
    	pBootS->iRootEntries = endianSwap(buffer[17], buffer[18]);
		pBootS->iLogicalSectors = endianSwap(buffer[19], buffer[20]);
        pBootS->xMediumDescriptor = buffer[21];
	// Read and covert sectors/fat, sectors/track, and number of heads
    	pBootS->iSectorsFAT = endianSwap(buffer[22], buffer[23]);
	pBootS->iSectorsTrack = endianSwap(buffer[24], buffer[25]);
	pBootS->iHeads = endianSwap(buffer[26], buffer[27]);

	// Read hidden sectors
	pBootS->iHiddenSectors = endianSwap(buffer[28], buffer[29]);

	return;
}


// iterates through the directory to display filename, time, date,
// attributes and size of each directory entry to the console
void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[])
{
    int i = 0;
    char string[13];

    // Display table header with labels
    printf("Filename\tAttrib\tTime\t\tDate\t\tSize\n");

    // loop through directory entries to print information for each
    for(i = 0; i < (iEntries); i = i + 32)   {
    	if ( !(buffer[i] == '0' && buffer[i+1] == '0')) {
    		// Display filename
    		printf("%s\t", toDOSName(string, buffer, i)  );
    		// Display Attributes
    		printf("%s\t", parseAttributes(string, buffer[i+11])  );
    		// Display Time
    		printf("%s\t", parseTime(string, endianSwap(buffer[i+22], buffer[i+23])));
    		// Display Date
    		printf("%s\t", parseDate(string, endianSwap(buffer[i+24], buffer[i+25])));
    		// Display Size
    		printf("%d\n", (buffer[i+28] | buffer[i+29] << 8 | buffer[i+30] << 16 | buffer[i+31] << 24));
    	}
    }

    // Display key
    printf("(R)ead Only (H)idden (S)ystem (A)rchive\n");
} // end parseDirectory()


// Parses the attributes bits of a file
char * parseAttributes(char string[], unsigned char key)
{
    int i = 0;
	int j=0;
	for (i=0;i<13;i++){
		string[i] = '\0';
	}
	if (key & 0x01){
		string[j] = 'R';
		j++;
	}
	if (key & 0x02){
		string[j] = 'H';
		j++;
	}
	if (key & 0x04){
		string[j] = 'S';
		j++;
	}
	if (key & 0x20){
		string[j] = 'A';
		j++;
	}
	return string;

} // end parseAttributes()


// Decodes the bits assigned to the time of each file
char * parseTime(char string[], unsigned short usTime)
{
    unsigned char hour = 0x00, min = 0x00, sec = 0x00;

    // printf("time: %x||", usTime);

	hour = (usTime >> 11) & 0x1F;
	min = (usTime >> 5) & 0x3F;
    sec = usTime & 0x1F;
	sec = sec * 2;

    // This is stub code!

    sprintf(string, "%02i:%02i:%02i", hour, min, sec);

    return string;


} // end parseTime()


// Decodes the bits assigned to the date of each file
char * parseDate(char string[], unsigned short usDate)
{
    unsigned char month = 0x00, day = 0x00;
    unsigned short year = 0x0000;

    // printf("date: %x|", usDate);

    year = ((usDate >> 9) & 0x7F) + 1980;
    month = (usDate >> 5) & 0xF;
    day = usDate & 0x1F;
    // This is stub code!

    sprintf(string, "%d/%d/%d", year, month, day);

    return string;

} // end parseDate()


// Formats a filename string as DOS (adds the dot to 8-dot-3)
char * toDOSName(char string[], unsigned char buffer[], int offset)
{
    int i;
	for (i = 0; i<8;i++){
		string[i] = buffer[i+offset];
	}
	string[8] = '.';
	for (i=9;i<12;i++){
		string[i] = buffer[i+offset-1];
	}
	string[12] = '\0';
    return string;
} // end toDosNameRead-Only Bit

