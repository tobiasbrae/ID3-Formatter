#include <stdio.h>
#include <stdint.h>
//#include <conio.h>
#include <windows.h>

/*
* MP3-Tag (last 128 Bytes)
* Identifier,  3 Bytes
*      Title, 30 Bytes
*     Artist, 30 Bytes
*      Album, 30 Bytes
*       Year,  4 Bytes
*    Comment, 30 Bytes
*      Genre,  1 Byte
*/

typedef struct
{
	char identifier[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	char genre[1];
}tag;

#define MAX_DEPTH 10

void scanDirectory(char *path, char *relPath, uint8_t depth);
void editMP3(char *path, char *relPath, char *fileName);
uint8_t checkName(char *path, char *relPath, char *fileName, char *fullPath);
void checkTags(char *fileName, char *fullPath);
char getAnswer(char *question, uint8_t allPointer);

#define NUM_ALLS 5
#define DUMMY 0
#define SEARCH_DIR 1
#define RENAME 2
#define CREATE_TAGS 3
#define CHANGE_TAGS 4
int8_t doAll[NUM_ALLS];

#define PATH_BUF 500
#define NAME_BUF 150
#define QUEST_BUF 200

char questBuffer[QUEST_BUF];

uint8_t verbose;
#define ERRORS 0
#define WARNINGS 0
#define FIRST_QUESTION 2
#define FINISH 1
#define NAME_CHANGES 1
#define TAG_CHANGES 2
#define INFORMATION 3

uint8_t firstQuest;

int main(void)
{
	firstQuest = 1;
	verbose = TAG_CHANGES;
	
	printf("Willkommen im ID3-V1 Formatierer!\n");
	printf("Dieses Programm sucht im angegebenen Verzeichnis alle *.mp3-Dateien.\n");
	printf("Danach werden aus dem jeweiligen Dateinamen Interpret und Titel entnommen.\n");
	printf("Diese Informationen werden dann in die ID3-V1-Tags aufgenommen.\n");
	printf("Die *.mp3-Dateien muessen dabei das Format \"<Interpret> - <Titel>.mp3\" haben.\n");
	printf("Mit \"scan <Pfad>\" kannst du den angegebenen Ordner scannen.\n");
	printf("Mit \"v <Level>\" passt du den Verbose-Level an.\n");
	printf("Mit \"exit\" beendest du das Programm.\n");

	char inputString[PATH_BUF];
	
	while(1)
	{
		printf("Gib einen Befehl ein:\n");
		uint8_t cnt = 0;
		do
		{
			scanf("%c", inputString + cnt);
			cnt++;
		}
		while(inputString[cnt-1] != 10);
		inputString[--cnt] = 0;
		if(!strncmp(inputString, "scan ", 5))
		{
			if(inputString[cnt - 1] != '\\')
			{
				if(verbose >= ERROR)
					printf("Der Pfad muss ein Ordner sein!\n");
			}
			else
			{
				for(int i = 0; i < NUM_ALLS; i++)
					doAll[i] = 0;
				char null = 0;
				scanDirectory(inputString + 5, &null, 1);
				if(verbose >= FINISH)
					printf("Suche abgeschlossen.\n");
			}
		}
		else if(!strncmp(inputString, "v ", 2))
		{
			int inputVal;
			if(sscanf(inputString + 2, "%d", &inputVal))
			{
				if(inputVal >= 0)
				{
					verbose = inputVal;
					printf("0 - Fehler, Warnungen\n");
					printf("1 - Fertigstellung, Namens-Aenderungen\n");
					printf("2 - Tag-Aenderungen\n");
					printf("3 - Informationen\n");
				}
				else if(verbose >= ERROR)
					printf("Der Verbose-Level muss >= 0 sein.\n");
			}
			else if(verbose >= ERROR)
				printf("Der eingegebene Wert muss eine Zahl sein.\n");
		}
		else if(!strncmp(inputString, "exit", 4))
		{
			if(verbose >= INFORMATION)
				printf("Programm wird beendet...\n");
			break;
		}
		else if(verbose >= ERROR)
		{
			printf("Unbekannter Befehl!\n");
		}
	}
	return 1;
}

// Windows-specific ========================================================
void scanDirectory(char *path, char *relPath, uint8_t depth)
{
	if(depth <= MAX_DEPTH)
	{
		char searchPath[PATH_BUF];
		sprintf(searchPath, "%s\\%s*.*", path, relPath);
		HANDLE hFind;
		WIN32_FIND_DATA findData;
		hFind = FindFirstFile(searchPath, &findData);
		if(hFind == INVALID_HANDLE_VALUE)
		{
			if(verbose >= ERROR)
				printf("Der angegebene Ordner existiert nicht!\n");
		}
		else
		{
			uint8_t dirEmpty = 1;
			uint8_t noMP3 = 1;
			do
			{
				if(findData.cFileName[0] != '.')
				{
					dirEmpty = 0;
					if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // directory found
					{
						char newRelPath[PATH_BUF];
						sprintf(newRelPath, "%s%s\\", relPath, findData.cFileName);
						sprintf(questBuffer, "Unterordner \"%s\" mit in die Suche einbeziehen?", newRelPath);
						if(getAnswer(questBuffer, SEARCH_DIR) == 'y')
						{
							if(verbose >= INFORMATION)
								printf("Unterordner \"%s\" wird durchsucht...\n", newRelPath);
							scanDirectory(path, newRelPath, depth + 1);
						}
						else if(verbose >= INFORMATION)
						{
							printf("Unterordner \"%s\" wird ausgelassen...\n", newRelPath);
						}
					}
					else // file found
					{
						char *searchDot = strrchr(findData.cFileName, '.');
						if(searchDot && !strcmp(searchDot, ".mp3")) // .mp3 found
						{
							noMP3 = 0;
							editMP3(path, relPath, findData.cFileName);
						}
						else if(searchDot && !strcmp(searchDot, ".MP3")) // change to lower case
						{
							char newFileName[NAME_BUF], fullPath[PATH_BUF], newFullPath[PATH_BUF];
							sprintf(newFileName, "%s", findData.cFileName);
							char *lastM = strrchr(newFileName, 'M');
							*lastM = 'm';
							*(lastM+1) = 'p';
							sprintf(fullPath, "%s%s%s", path, relPath, findData.cFileName);
							sprintf(newFullPath, "%s%s%s", path, relPath, newFileName);
							rename(fullPath, newFullPath);
							if(verbose >= NAME_CHANGES)
								printf("MP3: \"%s\"\n---> \"%s\"\n", findData.cFileName, newFileName);
							noMP3 = 0;
							editMP3(path, relPath, newFileName);
						}
					}
				}
			}
			while(FindNextFile(hFind, &findData));

			if(dirEmpty && verbose >= INFORMATION)
				printf("Der Ordner ist leer!\n");

			if(noMP3 && verbose >= INFORMATION)
				printf("Keine *.mp3-Dateien gefunden!\n");
		}
		FindClose(hFind);
	}
	else if(verbose >= WARNINGS)
		printf("Maximale Suchtiefe erreicht!\n");
}

void editMP3(char *path, char *relPath, char *fileName)
{
	char fullPath[PATH_BUF];
	sprintf(fullPath, "%s%s%s", path, relPath, fileName);
	
	FILE *mp3File = fopen(fullPath, "r");
	if(mp3File) // File found
	{
		fclose(mp3File);
		if(checkName(path, relPath, fileName, fullPath))
		{
			checkTags(fileName, fullPath);
		}
		
	}
	else
	{
		fclose(mp3File);
		if(verbose >= WARNINGS)
			printf("Datei \"%s%s\" wurde nicht gefunden...\n", relPath, fileName);
	}
}

uint8_t checkName(char *path, char *relPath, char *fileName, char *fullPath)
{
	uint8_t splitFound = 0;
	uint8_t cnt = 0;
	while(fileName[cnt+2])
	{
		if(!strncmp(fileName + cnt, " - ", 3))
		{
			splitFound = 1;
			break;
		}
		cnt++;
	}
	if(splitFound) // " - " found
	{
		char newFileName[NAME_BUF];
		uint8_t pointer = 0;
		while(fileName[pointer]) // check filename for unwanted cases
		{
			if((pointer == 0 || fileName[pointer-1] == ' ') && fileName[pointer] >= 'a' && fileName[pointer] <= 'z') // Upper case
			{
				newFileName[pointer] = fileName[pointer] - 32;
			}
			else // everything is fine
			{
				newFileName[pointer] = fileName[pointer];
			}
			pointer++;
		}
		newFileName[pointer] = 0;
		if(strcmp(fileName, newFileName)) // compare "new" filename to original
		{
			sprintf(questBuffer, "\"%s\" umbenennen in \n\"%s\"?", fileName, newFileName);
			if(getAnswer(questBuffer, RENAME) == 'y')
			{
				if(verbose >= NAME_CHANGES)
					printf("Name: \"%s\"\n----> \"%s\"\n", fileName, newFileName);
				char newFullPath[150];
				sprintf(newFullPath, "%s%s%s", path, relPath, newFileName);
				rename(fullPath, newFullPath);
				sprintf(fullPath, "%s", newFullPath);
				sprintf(fileName, "%s", newFileName);
			}
		}
		return 1;
	}
	else if(verbose >= WARNINGS)
	{
		printf("Kein \" - \" in \"%s%s\" gefunden...\n", relPath, fileName);
	}
	return 0;
}

void checkTags(char *fileName, char *fullPath)
{
	FILE *mp3File = fopen(fullPath, "r");
	if(mp3File)
	{
		tag oldTag, newTag;
		newTag.identifier[0] = 'T';
		newTag.identifier[1] = 'A';
		newTag.identifier[2] = 'G';
		char *pOldTag = (char*) &oldTag;
		char *pNewTag = (char*) &newTag;
		fseek(mp3File, -128, SEEK_END);
		for(int i=0; i < 128; i++) // load the last 128 byte into the old tag
			fscanf(mp3File, "%c", pOldTag+i);
		fclose(mp3File);
		
		uint8_t endFound = 0;
		uint8_t read = 0;
		uint8_t write = 0;
		while(read < 30) // load artist from filename into new tag
		{
			if(fileName[read] == ' ' && fileName[read+1] == '-')
				endFound = 1;
			if(endFound)
				newTag.artist[read] = 0;
			else
				newTag.artist[read] = fileName[read];
			read++;
		}
		
		read = 0;
		endFound = 0;
		while(strncmp(fileName + read - 2, " - ", 3)) // find beginning of title in filename
			read++;
		read++;
		while(write < 30) // load title from filename into new tag
		{
			if(!strcmp(fileName + read, ".mp3"))
				endFound = 1;
			if(endFound)
				newTag.title[write] = 0;
			else
				newTag.title[write] = fileName[read++];
			write++;
		}
		
		if(strncmp(oldTag.identifier, "TAG", 3)) // no tag found
		{
			sprintf(questBuffer, "Kein Tag in \"%s\" gefunden. Erstellen?", fileName);
			if(getAnswer(questBuffer, CREATE_TAGS) == 'y')
			{
				for(int i = 62; i < 128; i++)
					pNewTag[i] = 0;
				
				mp3File = fopen(fullPath, "a");
				if(mp3File)
				{
					for(int i = 0; i < 128; i++)
						fputc(pNewTag[i], mp3File);
					if(verbose >= TAG_CHANGES)
						printf("Tag fuer \"%s\" erstellt.\n", fileName);
					fclose(mp3File);
				}
			}
		}
		else // check tag
		{
			if(strcmp(oldTag.artist, newTag.artist) || strcmp(oldTag.title, newTag.title)) // tags aren't the same
			{
				sprintf(questBuffer, "Die Tags in \"%s\" weichen ab. Anpassen?", fileName);
				if(getAnswer(questBuffer, CHANGE_TAGS) == 'y')
				{
					mp3File = fopen(fullPath, "r+");
					if(mp3File)
					{
						for(int i = 62; i < 128; i++)
							pNewTag[i] = pOldTag[i]; // get the other data from file
						
						fseek(mp3File, -128, SEEK_END);
						for(int i = 0; i < 128; i++)
							fputc(pNewTag[i], mp3File);
						if(verbose >= TAG_CHANGES){}
							//printf("Tag: \"%s; %s\"\n---> \"%s; %s\"\n", artist, title, newArtist, newTitle);
					}
					else if(verbose >= ERRORS)
					{
						printf("Die Datei \"%s\" ist schreibgeschuetzt.\n", fileName);
					}
				}
			}
			else if(verbose >= INFORMATION)
			{
				printf("Tag in \"%s\" ist korrekt.\n", fileName);
			}
			fclose(mp3File);
		}
	}
}

char getAnswer(char *question, uint8_t allPointer)
{
	if(firstQuest && verbose >= FIRST_QUESTION)
	{
		firstQuest = 0;
		printf("Hinweis: Auf Fragen kannst du mit \"[a]y\" oder \"[a]n\" antworten.\n");
		char dummyBuffer[150];
		sprintf(dummyBuffer, "Das [a] bedeutet \"Antwort merken\". Hast du das verstanden?");
		if(getAnswer(dummyBuffer, DUMMY) == 'n')
			printf("Schade. Vielleicht bekommst du es ja trotzdem hin.\n");
	}
	char answer;
	if(doAll[allPointer] == 1)
		answer = 'y';
	else if(doAll[allPointer] == -1)
		answer = 'n';
	else
	{
		printf("%s", question);
		do
		{
			answer = getchar();
			//while(!kbhit()); // Windows-specific
			//answer = getch();
		}
		while(!(answer == 'a' || answer == 'y' || answer == 'n'));
		//getchar();
		if(answer == 'a')
		{
			do
			{
				answer = getchar();
				//while(!kbhit())
					//Sleep(10);
				//answer = getch();
			}
			while(!(answer == 'y' || answer == 'n'));
			getchar();
			if(answer == 'y')
				doAll[allPointer] = 1;
			else if(answer == 'n')
				doAll[allPointer] = -1;
		}
		printf("\n");
	}
	return answer;
}
