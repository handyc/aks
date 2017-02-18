// **************************************
// aks n-gram extractor
// version 1.0
// 17 February 2017
// by Christopher Handy
// **************************************
// To compile: gcc aks.c -o aks
// **************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#include <termios.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <time.h>

struct rusage ruse;

#define MALLOC_MULT 128 // multiplier for memory allocation based
						// on maximum chars per syllable

///////////////////////												
#define RESERVED 0
#define CHINESE 1
#define TIBETAN_ROMAN 2
#define TIBETAN_UCHEN 3
#define SANSKRIT_UNICODE 4
#define SANSKRIT_DEVA 5
///////////////////////
#define FIRSTCHAR 0
///////////////////////
#define CHIN_FIRSTCHAR 0
#define CHIN_STDCHAR 1
#define CHIN_SPECIAL 5
#define CHIN_SPACE 6
///////////////////////
#define SKT_VOWEL 1
#define SKT_CONSONANT 2
#define SKT_FINAL 3
#define SKT_NUMBER 4
#define SKT_SPECIAL 5
///////////////////////
#define TIB_ROMAN_FIRSTCHAR 0
#define TIB_ROMAN_VOWEL 1
#define TIB_ROMAN_CONSONANT 2
#define TIB_ROMAN_FINAL 3
#define TIB_ROMAN_NUMBER 4
#define TIB_ROMAN_SPECIAL 5
#define TIB_ROMAN_SPACE 6
///////////////////////
#define TIB_UCHEN_FIRSTCHAR 0
#define TIB_UCHEN_STDCHAR 1
#define TIB_UCHEN_FINAL 3
#define TIB_UCHEN_FINAL2 4
#define TIB_UCHEN_SPECIAL 5
#define TIB_UCHEN_SPACE 6
///////////////////////
#define LANGUAGE_CHOICE CHINESE
//////////////////////

#define CPU_TIME (getrusage(RUSAGE_SELF, &ruse), ruse.ru_utime.tv_sec + \
  ruse.ru_stime.tv_sec + 1e-6 * \
  (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec))

#ifndef FAIL_SAFE_H
    #define FAIL_SAFE_H
    
     #define INIT_BUF_SIZE     40
    #define BUF_INC_SIZE      20
    
char *failSafeRead(FILE *);

#endif

////////////////////////////////
int mygetch( ) {
  struct termios oldt,
                 newt;
  int            ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;
}
////////////////////////////////

/// DEFINE AKSHARA
struct akshara {
   int val;
   
	wchar_t component[10];
	int position;
	int total_components;

	int initial_r;
	int final_present;
	int strength;
	int tone;

   	struct akshara * prev;
   	struct akshara * next;
};
typedef struct akshara Akshara;

///////////////////////////////////////////
typedef struct List {
    int count;
    Akshara *first;
    Akshara *last;
} List;
///////////////////////////////////////////
List *List_create()
{
    return calloc(1, sizeof(List));
}
///////////////////////////////////////////

struct glyph{
	wchar_t component[10];

	int position;
	int total_components;

	int initial_r;

	int final_present;
	int strength;
	int tone;
}ca;
typedef struct glyph Glyph;
///////////////////////////////////////////

int glyph_add(Glyph *ak, wchar_t newchar)
	{
	ak->component[ak->position]=newchar;
	ak->position++;
	ak->total_components++;
	ak->component[ak->total_components]='\0';
	
	return 0;
	}

int glyph_reset(Glyph *ak)
	{
	ak->position=0;
	ak->total_components=0;

	ak->initial_r=0;

	ak->final_present=0;
	ak->strength=0;
	ak->tone=0;

	return 0;
	}

///////////////////////////
// GLOBALS
//////////////////////////

struct akshara* insert_akshara(wchar_t insert_value[], struct akshara* current, struct akshara* header)//wchar_t newchar)
	{
	struct akshara *new_node;
	new_node = (Akshara *)malloc(sizeof(Akshara));

	if(new_node == NULL)
	printf("nFailed to Allocate Memory");

	wcscpy(new_node->component, insert_value);

 	new_node->next=NULL;

 	if(header==NULL)
 		{
		header=new_node;
		current=new_node;
		return header;
		}
	else
		{
		struct akshara *temp;
		temp = header;
		while(temp->next!=NULL)
			{
			temp = temp->next;
			}
		temp->next = new_node;

		return header; 
		}
 
	}

void reset_list(struct akshara* header, struct akshara* current)
	{
	struct akshara *temp_ptr;
	temp_ptr = (Akshara *)malloc(sizeof(Akshara));

	if(temp_ptr == NULL)
	printf("nFailed to Allocate Memory");
 
 	current=header;
   
 	while(current!=NULL)
 		{
		temp_ptr=current->next;
		free(current);
		}
 
 	free(header);
	free(temp_ptr); 
	}

wchar_t * write_ngram(struct akshara* current, int n, int delimiter_flag)
	{
	Akshara * tempcursor;

	int malloc_value=(n*MALLOC_MULT); // n value times max. possible chars
						 		// per glyph (16?) * max. bytesize per char (4)
// NGRAM_MALLOC
wchar_t *ngram = malloc(malloc_value); // value should be changed to reflect
								// the amount of memory actually needed
								// based on maximum ngram size
								// currently is wasteful
tempcursor = current;
//printf("\n");

	int x=0;
	while(x<n && tempcursor != NULL)
	{

	if(x>0 && delimiter_flag==1)
		{
		wcscat(ngram, L"_");
		} // add delimiter if not first item and delimiter_flag is on

	wcscat(ngram, tempcursor->component);
	tempcursor = tempcursor->next;
	x++;
	}

return ngram;
}

int char_check(wchar_t x, int language_choice);

int line_counter=0;
int akshara_count=0;
int unique_aksharas=0;

int type_check=0;
int prev_type=0;
int prev_used=FIRSTCHAR;

long bytes_read=0;

#define DEFAULT_TEXT_CHINESE "../texts/chinese/taisho/T24"
#define DEFAULT_TEXT_TIBETAN_ROMAN "../texts/tibetan/ACIP_KANGYUR_DUMP"
#define DEFAULT_TEXT_TIBETAN_UCHEN "../texts/tibetan/ACIP_KANGYUR_DUMP_TIB"
#define DEFAULT_TEXT_SANSKRIT_UNICODE "../texts/sanskrit/mahayana"
//#define DEFAULT_TEXT_SANSKRIT_DEVA "../texts/sanskrit/mahayana"

#define MAX_STRING_LENGTH 256

///////////////////////////

int main(int argc, char *argv[]) {

int language_choice=RESERVED;

int ngram_integer;

char * filenameout = "testout.txt";
char * out_dir1 = "../output";

//char * cmp_dir1 = "./taisho/T24";
char cmp_dir1[MAX_STRING_LENGTH];

// argv[0] is the name of the program
// argv[1] is the name of the language (chinese, tibetan_roman, tibetan_uchen, sanskrit_unicode, sanskrit_deva)
// argv[2] is the n-gram number
// argv[3] is the path to the input directory

int delimiter_flag=0;

if(argc>1)
	{
	if(strcmp(argv[1],"chinese")==0)
		{
		language_choice=CHINESE;
		}
	else if(strcmp(argv[1],"tibetan_roman")==0)
		{
		language_choice=TIBETAN_ROMAN;
		delimiter_flag=1;
		}
	else if(strcmp(argv[1],"tibetan_uchen")==0)
		{
		language_choice=TIBETAN_UCHEN;
		}
	else if(strcmp(argv[1],"sanskrit_unicode")==0)
		{
		language_choice=SANSKRIT_UNICODE;
		}
	else if(strcmp(argv[1],"sanskrit_deva")==0)
		{
		language_choice=SANSKRIT_DEVA;
		}
	else
		{
		printf("\nLanguage not recognized!");
		}
	}
else
	{
	language_choice=CHINESE;
	}

if(argc>2)
	{
	ngram_integer=atoi(argv[2]);
	}
else
	{
	ngram_integer=1;
	}

if(argc>3)
	{
	strncpy(cmp_dir1, argv[3], MAX_STRING_LENGTH-1);
	cmp_dir1[MAX_STRING_LENGTH-1] = '\0';
	}
	else
	{
	strncpy(cmp_dir1, DEFAULT_TEXT_CHINESE, MAX_STRING_LENGTH-1);
	cmp_dir1[MAX_STRING_LENGTH-1] = '\0';
	}

printf("Language set to %d\n", language_choice);

/// SETUP
///////////////
time_t start, end, rawtime;
double first, second;

struct tm * timeinfo;


FILE * fin;
FILE * fout_ngram[20];

wchar_t c;
wchar_t prev_char=0;

// THESE LOCALE SETTINGS MAY NEED TO BE MODIFIED FOR SOME SYSTEMS
// IF YOU SEE 'GARBAGE' CHARACTERS IN THE OUTPUT, YOU NEED TO MAKE
// SURE YOUR LOCALE SETTINGS ARE USING UTF-8
//setlocale(LC_ALL, "en_US.utf8");
setlocale(LC_ALL, "");

////////////////

/// SETUP DIRECTORIES
////////////////
DIR* FD1;
DIR* FD2;

    struct dirent* in_file1;
    struct dirent* in_file2;
    FILE    *common_file;
    FILE    *entry_file;
    char    buffer[BUFSIZ];
    
////////////////

////////////////
char *charstream;
int x=0;
int s=0;
int textcount=0;
int file_length=0;
unsigned long long total_strings=0;


char filetoopen[MAX_STRING_LENGTH];
char filetowrite[20][MAX_STRING_LENGTH];
char filetowrite_ngram[20][MAX_STRING_LENGTH];
char ngram_as_string[20][MAX_STRING_LENGTH];

/////////////////

   Akshara * curr, * head;
   //FileInfo * filecurrent, * filehead;
    
   int i;
   head = NULL;

/////////////////

//// INITIALIZE CLOCK

// Save user and CPU start time
    time(&start);
    first = CPU_TIME;

///* Opening common file for writing */
//    common_file = fopen("testcommon.txt", "w");
//    if (common_file == NULL)
//    {
//        fprintf(stderr, "Error : Failed to open common_file - %s\n", //strerror(errno));
//        return 1;
//    }
    
//////////////////
// READ FIRST DIRECTORY
//////////////////

/////// OPEN DIRECTORIES

    /* Scanning the input directories */
    
    if (NULL == (FD1 = opendir (cmp_dir1))) 
    {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        fclose(common_file);
        return 1;
    }

while ((in_file1 = readdir(FD1))) 
    {
    
/// figure out the name of the next file, ignore special files    
    
 if (!strcmp (in_file1->d_name, "."))
            continue;
        if (!strcmp (in_file1->d_name, ".."))    
            continue;
        if (!strcmp (in_file1->d_name, ".DS_Store"))    
            continue;    
        
        strcpy(filetoopen, cmp_dir1);
        strcat(filetoopen, "/");
        strcat(filetoopen, in_file1->d_name);

///////////////////////////


pid_t pid = fork();
if (pid == 0) {
    //funcToCallInChild(argument);
    //printf("[%d] [%d]\n", getppid(), getpid());
    
    //////////
    //printf("\n[%d][%d] opens %s\n", getppid(), getpid(), filetoopen);
    //printf("\nProcess #%d opens %s", getpid(), filetoopen);
    //////////
        
    ///////////////////////////////////////////////////////////////////
    /////// Read and output from child takes place here only //////////
    ///////////////////////////////////////////////////////////////////
    
    ///////////////////////////////////////////////////////////////////
    

       //////////////////////    
    
    int specialx; // controls n-gram filenames for n==1 max
    
    for(specialx=0; specialx<ngram_integer; specialx++)
    {
        strcpy(filetowrite_ngram[specialx], out_dir1);
        strcat(filetowrite_ngram[specialx], "/");
        strcat(filetowrite_ngram[specialx], in_file1->d_name);
        snprintf(ngram_as_string[specialx], sizeof ngram_as_string[specialx], ".%d", specialx+1);
        strcat(filetowrite_ngram[specialx], ngram_as_string[specialx]);
        strcat(filetowrite_ngram[specialx], ".ngram");
       
        fout_ngram[specialx] = fopen(filetowrite_ngram[specialx], "w");
    } 
       
       //////////////////////
       
        fin = fopen(filetoopen, "rw");
        
        //fout = fopen(filetowrite, "w");
     
        if (fin == NULL)
        {
            fprintf(stderr, "Error : Failed to open entry file - %s\n", strerror(errno));
            //printf("%s", in_file1->d_name);
           
           printf("error");
           // fclose(common_file);

            return 1;
        }

	///////////////////////////////////////////////////////////////////
	

	bytes_read=0;
	
while((c = fgetwc(fin)) != WEOF)
{
prev_type=type_check;
type_check=char_check(c, language_choice);

switch(language_choice)
	{
	case RESERVED:
	break;
	////////////////////////
	case CHINESE:
	////////////////////////
	if(prev_used==FIRSTCHAR) // this must be the first character read, so begin
		{ 
		glyph_reset(&ca);
		glyph_add(&ca, c);
		prev_used=type_check;
		}
	else
		{	
		if(char_check(c, language_choice)==CHIN_STDCHAR)
			{
			head=insert_akshara(ca.component, curr, head);
			glyph_reset(&ca);
			glyph_add(&ca, c);
			}
	
		prev_used=type_check;
		prev_char=c;
		}
	break;
	////////////////////////
	case TIBETAN_ROMAN:
		if(prev_used==FIRSTCHAR) // must be the first character read, so begin
			{ 
			glyph_reset(&ca);
			glyph_add(&ca, c);
			if(type_check!=TIB_ROMAN_SPECIAL)
				{
				prev_used=type_check;
				}	
			}
		else if(prev_used!=TIB_ROMAN_SPECIAL && type_check==TIB_ROMAN_SPECIAL)
				{
				head=insert_akshara(ca.component, curr, head);
				glyph_reset(&ca);
				prev_used=type_check;
				prev_char=c;
				}
		else if(type_check!=TIB_ROMAN_SPECIAL)
				{
				glyph_add(&ca, c);
				prev_used=type_check;
				prev_char=c;
				}
		break;
	////////////////////////
	case TIBETAN_UCHEN:
		if(prev_used==FIRSTCHAR) // first character read: begin
			{ 
			glyph_reset(&ca);
			glyph_add(&ca, c);

			if(type_check==TIB_UCHEN_STDCHAR)
				{
				prev_used=type_check;
				}
			}
		else if(type_check==TIB_UCHEN_FINAL) // last character: end glyph
				{
				glyph_add(&ca, c);
				head=insert_akshara(ca.component, curr, head);
				glyph_reset(&ca);
				prev_used=type_check;
				prev_char=c;
				}
		else if(prev_used==TIB_UCHEN_STDCHAR && (type_check==TIB_UCHEN_FINAL2))
				{
				glyph_add(&ca, L'་');
				head=insert_akshara(ca.component, curr, head);
				glyph_reset(&ca);
				prev_used=type_check;
				prev_char=c;
				}
		else if(type_check==TIB_UCHEN_STDCHAR)
				{
				glyph_add(&ca, c);
				prev_used=type_check;
				prev_char=c;
				}
		break;
	////////////////////////
	case SANSKRIT_UNICODE:

if(prev_used==FIRSTCHAR) // this must be the first character read, so begin
{ 
glyph_reset(&ca);
glyph_add(&ca, c);
	if(type_check==SKT_CONSONANT || type_check==SKT_VOWEL)
	{
	prev_used=type_check;
	}
}
else if(type_check==SKT_FINAL)
{
glyph_add(&ca, c);
head=insert_akshara(ca.component, curr, head);
glyph_reset(&ca);
prev_used=type_check;
prev_char=c;
}
else if(prev_used==SKT_VOWEL && (type_check==SKT_CONSONANT || type_check==SKT_VOWEL || type_check==SKT_NUMBER))
{
	if(prev_char=='a' && (c=='i' || c=='u'))
		{
		glyph_add(&ca, c);
		prev_used=type_check;
		prev_char=c;
		}
	else if(c=='a' || c==L'ṛ')
		{
		head=insert_akshara(ca.component, curr, head);
		//akshara_print(&ca);
		//collection_add(&ak_collection, &ca);
		glyph_reset(&ca);
		glyph_add(&ca, c);
		prev_used=type_check;
		prev_char=c;
		}
	else if(type_check==SKT_VOWEL || type_check==SKT_CONSONANT)
		{	
		head=insert_akshara(ca.component, curr, head);
		//akshara_print(&ca);
		//collection_add(&ak_collection, &ca);
		glyph_reset(&ca);
		glyph_add(&ca, c);
		prev_used=type_check;
		prev_char=c;
		}
}
else if(type_check==SKT_VOWEL || type_check==SKT_CONSONANT)
{
glyph_add(&ca, c);
prev_used=type_check;
prev_char=c;
}

break;

////////////////////////
	case SANSKRIT_DEVA: break;
	////////////////////////
default: break;
}

bytes_read++;
}
	///////////////////////////////////////////////////////////////////

time(&end);        // completion time (user)
second = CPU_TIME; // completion time (CPU)

printf("Finished %s in %d seconds (user), %.2f seconds (CPU)\n", filetoopen, (int)(end - start), (second - first));	
fclose(fin);

	//fprintf(common_file, "\nAnalysis of %s:", filetoopen);
	//fprintf(common_file, "\n\n");
	///////////////////////////////////////////////////////////////////
	

	curr = head;
   while(curr) {
      wchar_t *ngram=malloc(128);
      
      for(specialx=0; specialx<ngram_integer; specialx++)
      {
      //ngram=write_ngram(curr, ngram_integer);
      ngram=write_ngram(curr, specialx+1, delimiter_flag);
      fprintf(fout_ngram[specialx], "%ls\n", ngram);
      }
         
      curr = curr->next ;
           
     // this works, except that 
     // b) the function does not check to see if part of n-gram is NULL
     // c) if NULL in b, function should exit with error
        }

	///////////////////////////////////////////////////////////////////

	//printf("[%d] [%d] closes %s\n", getppid(), getpid(), filetoopen);

	head = NULL;
	curr = NULL;
	
	reset_list(head, curr);

for(specialx=0; specialx<ngram_integer; specialx++)
    {
	fclose(fout_ngram[specialx]);
	}

	fclose(fin);
    
    ///////////////////////////////////////////////////////////////////
    
    exit(0);
}
else
{
//printf("from parent: [%d] [%d]\n", getppid(), getpid());
// above line tests parent, not needed
// put any parent-specific functions in here (e.g., wait, write to common file)

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////PARENT-SPECIFIC OPERATIONS//////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

}

} // end of for loop

//////////////////
// CLOSING PROGRAM MAINTENANCE
/////////////////////////////

//fclose(common_file);


////////
///CHECK CLOCK
// Save end time
//    time(&end);
//    second = CPU_TIME;
//
//    printf("\ncpu  : %.2f secs", second - first); 
//    printf("\nuser : %d secs\n", (int)(end - start));

//////////////////

//printf("\nparent is now waiting...");
//wait();

return 0;   
}

int char_check(wchar_t x, int language_choice)
{
switch(language_choice)
	{
	case RESERVED: break;
	
	//////////////// Special formatting rules for Chinese Unicode /////////////
	case CHINESE:
		if(x==' ' || x==L'。' || x=='.' || x=='\n' || x=='-' || x==L',' || x=='/' || x=='(' || x==')' || x=='\\' || x==L'║' || x=='=' || x==':' || x=='#' || x=='_' || x=='\n' || x=='\r' || x==L'【' || x==L'】' || x==',' || x==L'》' || x==L'，' || x==L'　' || x==L'、' || x==L'：' || x==L'「' || x==L'」' || x==L'？' || x==L'！' || x==L'；' || x==L'『' || x==L'』' || x==L'）' || x==L'（' || x==L'．' || x==L'…' || x==L'○' || x==L']' || x==L'[' || x==L'*' || x==L'《' || x==L'+' || x==L'／' || x==L'〉' || x==L'〈' || x==L'◇')
	{
	return CHIN_SPECIAL;
	}
	else if(x=='A' || x=='B' || x=='C' || x=='D' || x=='E' || x=='F' || x=='G' || x=='H' || x=='I' || x=='J' || x=='K' || x=='L' || x=='M' || x=='N' || x=='O' || x=='P' || x=='Q' || x=='R' || x=='S' || x=='T' || x=='U' || x=='V' || x=='W' || x=='X' || x=='Y' || x=='Z' || x=='a' || x=='b' || x=='c' || x=='d' || x=='e' || x=='f' || x=='g' || x=='h' || x=='i' || x=='j' || x=='l' || x=='m' || x=='n' || x=='o' || x=='p' || x=='q' || x=='r' || x=='s' || x=='t' || x=='u' || x=='v' || x=='w' || x=='x' || x=='y' || x=='z' || x=='0' || x=='1' || x=='2' || x=='3' || x=='4' || x=='5' || x=='6' || x=='7' || x=='8' || x=='9')
	{
	return CHIN_SPECIAL;
	}
	else
	{
	return CHIN_STDCHAR;
	}
	break;
	//////////////// End formatting rules for Chinese Unicode /////////////
	
	//////////////// Special formatting rules for Tibetan roman /////////////
	case TIBETAN_ROMAN:
			if(x==L'B' || x==L'C' || x==L'D' || x==L'F' || x==L'G' || x==L'H' || x==L'J' || x==L'K' || x==L'L' || x==L'M' || x==L'N' || x==L'P' || x==L'Q' || x==L'R' || x==L'S' || x==L'T' || x==L'V' || x==L'W' || x==L'X' || x==L'Y' || x==L'Z' || x==L'b' || x==L'c' || x==L'd' || x==L'f' || x==L'g' || x==L'h' || x==L'j' || x==L'k' || x==L'l' || x==L'm' || x==L'n' || x==L'p' || x==L'q' || x==L'r' || x==L's' || x==L't' || x==L'v' || x==L'w' || x==L'x' || x==L'y' || x==L'z')
{
return TIB_ROMAN_CONSONANT;
}
else if(x==L'A' || x==L'E' || x==L'I' || x==L'O' || x==L'U' || x==L'a' || x==L'e' || x==L'i' || x==L'o' || x==L'u' || x==L'\'')
{
return TIB_ROMAN_VOWEL;
}
else if(x==' '|| x==L'|' || x=='\n' || x=='-' || x==L',' || x==L'@' || x=='0' || x=='1' || x=='2' || x=='3' || x=='4' || x=='5' || x=='6' || x=='7' || x=='8' || x=='9')
{
return TIB_ROMAN_SPECIAL;
}
else return TIB_ROMAN_SPECIAL;
break;
	//////////////// End formatting rules for Tibetan roman /////////////
	
	//////// Special formatting rules for Tibetan Uchen Unicode /////////////
	case TIBETAN_UCHEN:
			if(x==L'་' || x==L'༌') // two kinds of tsheg
	{
	return TIB_UCHEN_FINAL;
	}
	else if(x==L'།' || x==L'༑') // if we get a shad
	{
	return TIB_UCHEN_FINAL2; // note this to convert it to a tsheg
	}
	else if(x==' ' || x==L'།' || x=='.' || x=='\n' || x=='-' || x==L',' || x=='/' || x=='(' || x==')' || x=='\\' || x==L'║' || x=='=' || x==':' || x=='#' || x=='_' || x=='\n' || x=='\r' || x==L'【' || x==L'】' || x==L'[' || x==L']' || x=='[' || x==']' || x==',' || x==L'》')
	{
	return TIB_UCHEN_SPECIAL;
	}
	else if(x=='A' || x=='B' || x=='C' || x=='D' || x=='E' || x=='F' || x=='G' || x=='H' || x=='I' || x=='J' || x=='K' || x=='L' || x=='M' || x=='N' || x=='O' || x=='P' || x=='Q' || x=='R' || x=='S' || x=='T' || x=='U' || x=='V' || x=='W' || x=='X' || x=='Y' || x=='Z' || x=='a' || x=='b' || x=='c' || x=='d' || x=='e' || x=='f' || x=='g' || x=='h' || x=='i' || x=='j' || x=='l' || x=='m' || x=='n' || x=='o' || x=='p' || x=='q' || x=='r' || x=='s' || x=='t' || x=='u' || x=='v' || x=='w' || x=='x' || x=='y' || x=='z' || x=='0' || x=='1' || x=='2' || x=='3' || x=='4' || x=='5' || x=='6' || x=='7' || x=='8' || x=='9')
	{
	return TIB_UCHEN_SPECIAL;
	}
	else
	{
	return TIB_UCHEN_STDCHAR;
	}
	break;
	//////// End formatting rules for Tibetan Uchen Unicode /////////////
	
	//////// Special formatting rules for Sanskrit Unicode /////////////
	case SANSKRIT_UNICODE:
			if(x==L'k' || x==L'h' || x==L'g' || x==L'ṅ' || x==L'c' || x==L'j' || x==L'ñ' || x==L'ṭ' || x==L'ḍ' || x==L'ṇ' || x==L't' || x==L'd' || x==L'n' || x==L'p' || x==L'b' || x==L'm' || x==L'y' || x==L'r' || x==L'l' || x==L'v' || x==L'ś' || x==L'ṣ' || x==L's')
{
return SKT_CONSONANT;
}
else if(x==L'a' || x==L'i' || x==L'u' || x==L'e' || x==L'o' || x==L'ā' || x==L'ī' || x==L'ū' || x==L'ṛ' || x==L'ṝ' || x==L'ḷ' || x==L'ḹ')
{
return SKT_VOWEL;
}
else if(x==L'ṁ' || x==L'ṃ' || x==L'ḥ')
{
return SKT_FINAL;
}
else if(x==' '|| x==L'|' || x=='\n' || x=='-')
{
return SKT_SPECIAL;
}
else if(x=='0' || x=='1' || x=='2' || x=='3' || x=='4' || x=='5' || x=='6' || x=='7' || x=='8' || x=='9')
{
return SKT_NUMBER;
}
else return SKT_SPECIAL;
break;
	//////// End formatting rules for Sanskrit Unicode /////////////
	
	//////// Special formatting rules for Sanskrit Devanagari /////////////
	case SANSKRIT_DEVA:
	break;
	// NOT YET IMPLEMENTED
	
	//////// End formatting rules for Sanskrit Devanagari /////////////
	
	default: printf ("\nerror! language unknown!"); break;
	}	
//////////////////////////////////////////////


/////////////////////////////////////////////////
}

