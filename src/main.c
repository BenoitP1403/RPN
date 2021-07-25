/*
This is a simple RPN for casio CG-50. It also builds for 35+EII
Benoît Poreaux
*/

#include <gint/std/stdio.h>
#include <gint/std/stdlib.h>
#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/gint.h>
#include <gint/clock.h>
#include <gint/bfile.h>
#include <math.h>

#define False 0
#define True 1
#define KEY_ENTER KEY_EXE // La flemme de remplacer KEY_ENTER par KEY_EXE...
#define tailleBuffer 50 //The size of all the buffers that will store strings. 

#ifdef FXCG50 // If building for 90 +E
#define HEIGHT 224
#define WIDTH 396
#define taillePile 16 // The number of numbrs that can be displayed on the screen
extern font_t font;  // The CG50 needs another font for the text to be readable.
#else // If building for 35+EII
#define HEIGHT 64
#define WIDTH 128
#define taillePile 7
#endif

//Here are declared more keycodes for keys that are accessed using shift.
#define	KEY_SQRT 1 // x^0.5
#define	KEY_ROOTN 2 // x^(1/n)
#define	KEY_10PX 3  //10^x
#define	KEY_EXPO 4 // exp(x)
#define	KEY_ASIN 5 // asin(x)
#define	KEY_ACOS 6 // acos(x)
// Skip 7 because KEY_AC==7
#define	KEY_ATAN 8 // atan(x)
#define	KEY_ROOT3 9 // x^(1/3)
#define	KEY_INV 10 // 1/x
#define	KEY_PI 11 // Pi, 3.14159265359...


char ev2nb(key_event_t o); // Returns the number written on the key_pressed. Should only be used for a event corresponding to a number key...
double str2double(char * str,int len); //Returns the double that is represented in a string
void long2buffer(unsigned long long nb, char * b); // Puts a long int into a string
void rotateDown(double * pile,char t); // Rotates the pile, used when parenthesis pressed
void rotateUp(double * pile,char t);
void PushUp(double * pile);
void resetP(double * pile);
void addNumber(double i,double * pile);
char PileSwitchRotations(double * pile,int op,unsigned char lastDisplayed);
void double2buffer(double nb, char * buffer);
char numberPressed(key_event_t o);
void doOp(double nb1,double nb2, unsigned char op,double * pile, char NEntered,unsigned char * lastDisplayed);
void dispPile(double * pile,char lastDisplayed, char signeInput, char * bufferInput);
uint findKey(key_event_t event);
void resetBuffer(char * buffer);
void writePileInFS(double * pile, unsigned char lastDisplayed);
void readPileInFS(double * pile, unsigned char * lastDisplayed);
void mainLoop();
int main();

char ev2nb(key_event_t o){ //event of a number key to this number
        char e=o.key;
        if (e==KEY_0)
                return 0;
        if (e==KEY_1)
                return 1;
        if (e==KEY_2)
                return 2;
        if (e==KEY_3)
                return 3;
        if (e==KEY_4)
                return 4;
        if (e==KEY_5)
                return 5;
        if (e==KEY_6)
                return 6;
        if (e==KEY_7)
                return 7;
        if (e==KEY_8)
                return 8;
        return 9;
}

double str2double(char * str,int len){
        double o=0;
        char ent=True;
        char exp=0;
        for (int i=0;i<len;i++){
                if (str[i]=='\0')
                        break;
                if (str[i]=='.'){
                        ent=False;
                        continue;
                }
                else
                {
                        if (ent){
                                o*=10;
                                o+=str[i]-'0';
                        }
                        else{
                                exp--;
                                o+=(str[i]-'0')*pow(10,exp);

                        }
                }
        }
        return o;
}



void long2buffer(unsigned long long nb, char * b){
	if (nb==0){b[0]='0';b[1]='\0';return;}
        unsigned char r=0;
        while(nb>0){
                b[r]='0'+nb%10;
                nb=nb/10;
                r++;
        }
        char tmp;
        for (int i=0;i<(r)/2;i++){
                tmp=b[i];
                b[i]=b[r-1-i];
                b[r-1-i]=tmp;
	}
        b[r]='\0';

}


void rotateDown(double * pile,char t){ // Called to transform [a,b,c,d] into [a,c,d,b]
	double tmp=pile[1];
	for (int i=1;i<t-1;i++){
		pile[i]=pile[i+1];
	}
	pile[t-1]=tmp;
}

void rotateUp(double * pile,char t){ 
	double tmp=pile[t-1];
	for (int i=t-1;i>1;i--){
		pile[i]=pile[i-1];
	}
	pile[1]=tmp;
}


void PushUp(double * pile){ 
	char t=taillePile;
	double tmp=pile[t-1];
	for (int i=t-1;i>1;i--){
		pile[i]=pile[i-1];
	}
	pile[1]=tmp;
}


void resetP(double * pile){
	for (int i=0;i<taillePile;i++){
		pile[i]=0.0;
	}
}

void addNumber(double i,double * pile){ // add i in the pile
	for (unsigned char j=taillePile-1;j>0;j--){
		pile[j+1]=pile[j];
	}
	pile[1]=i;
}

char PileSwitchRotations(double * pile,int op,unsigned char lastDisplayed){
	if(op==KEY_LEFTP){
		rotateDown(pile,lastDisplayed);
		return 1;
	}
	else if(op==KEY_RIGHTP){
		rotateUp(pile,lastDisplayed);
		return 1;
	}
	else if (op==KEY_FRAC && lastDisplayed>2){
		double tmp;
		tmp=pile[1];
		pile[1]=pile[2];
		pile[2]=tmp;
		return 1;
	}
	return 0;
}

void double2buffer(double nb, char * buffer){ // Put nb into the string buffer. I use that because it seems that sprintf isn't working for floats and doubles... 
        char sign=nb/fabs(nb);
        nb=fabs(nb);

        int exp=0;

        if (nb>=1000000000){
                while (nb>= 10 && exp<255){
                        nb/=10;
                        exp++;
                }
                if (nb>= 10){
                        buffer[0]='O';buffer[1]='V';buffer[2]='E';buffer[3]='R';buffer[4]='F';buffer[5]='L';buffer[6]='O';buffer[7]='W';buffer[8]='\0';
                        return;
                }
        }
        if(nb<1. && nb>0){
                while(nb<1 && exp>-255){
                        nb*=10;
                        exp--;
                }
                if (nb<1){
                        nb=0;
                        exp=0;
                }
        }
        unsigned long long ent = (long)(nb);
        unsigned long long dec=(unsigned long long)round(((1+nb-(double)ent)*pow(10,8)));
        char * buffer1=malloc(tailleBuffer*sizeof(char));
        char * buffer2=malloc(tailleBuffer*sizeof(char));
        long2buffer(dec,buffer2);
        if (buffer2[0]=='2')
                ent+=1;
        long2buffer(ent,buffer1);
        buffer2[0]='.';
    sprintf(buffer,"+%s%sE%d",buffer1,buffer2,exp);
    if (sign<0)
        buffer[0]='-';
   free(buffer1);
    buffer1=NULL;
   free(buffer2);
    buffer2=NULL;
}




char numberPressed(key_event_t o){ // Check whether the touch pressed is a number.
	char e=o.key;
	if ((e==KEY_1) || (e==KEY_2) || (e==KEY_3) || (e==KEY_4) || (e==KEY_5) ||(e==KEY_6) || (e==KEY_7) || (e==KEY_8) || (e==KEY_9) || (e==KEY_0))
		return True;
	return False;
}

void doOp(double nb1,double nb2, unsigned char op,double * pile, char NEntered,unsigned char * lastDisplayed){
	char m=1;
	if (op==KEY_ADD  && (NEntered || (*lastDisplayed)>2))
		pile[1]=nb2+nb1;
	else if (op==KEY_SUB && (NEntered || (*lastDisplayed)>2))
		pile[1]=nb2-nb1;
	else if (op==KEY_MUL && (NEntered || (*lastDisplayed)>2))
		pile[1]=nb2*nb1;
	else if (op==KEY_DIV && (NEntered || (*lastDisplayed)>2))
		if (nb1!=0.0)
			pile[1]=nb2/nb1;
		else
			pile[1]=NAN;
	else if (op==KEY_POWER && (NEntered || (*lastDisplayed)>2))
		pile[1]=pow(nb2,nb1);
	else if (op==KEY_EXP && (NEntered || (*lastDisplayed)>2))
		{pile[1]=nb2*pow(10,nb1);
		}
	else if (op==KEY_ROOTN && (NEntered || (*lastDisplayed)>2))
		if (nb2>=0)
			pile[1]=pow(nb2,1./nb1);
		else {
			pile[1]=-pow(-nb2,1./nb1);
			if ((pow(pile[1],nb1)*nb2)<0)
				pile[1]=NAN;
		}
	else if (op==KEY_SIN)
		{if (NEntered)PushUp(pile);
		pile[1]=sin(nb1);
		m=0;}
	else if (op==KEY_COS)
		{
		if (NEntered)PushUp(pile);
		pile[1]=cos(nb1);
		m=0;}
	else if (op==KEY_TAN)
		{
		if (NEntered)PushUp(pile);
		pile[1]=tan(nb1);
		m=0;}
	else if (op==KEY_SQUARE)
		{
		if (NEntered)PushUp(pile);
		pile[1]=nb1*nb1;
		m=0;}
	else if (op==KEY_LOG)
		{
		if (NEntered)PushUp(pile);
		pile[1]=log10(nb1);
		
		m=0;}
	else if (op==KEY_LN)
		{
		if (NEntered)PushUp(pile);
		pile[1]=log(nb1);
		m=0;}
	else if (op==KEY_SQRT)
		{
		if (NEntered)PushUp(pile);
		pile[1]=pow(nb1,0.5);
		
		m=0;}
	else if (op==KEY_10PX)
		{
		if (NEntered)PushUp(pile);
		pile[1]=pow(10,nb1);
		m=0;}
	else if (op==KEY_EXPO)
		{
		if (NEntered)PushUp(pile);
		pile[1]=pow(2.71828182846,nb1);
		m=0;}
	else if (op==KEY_ASIN)
		{if (NEntered)PushUp(pile);
		pile[1]=asin(nb1);
		m=0;}
	else if (op==KEY_ACOS)
		{if (NEntered)PushUp(pile);
		pile[1]=acos(nb1);
		m=0;}
	else if (op==KEY_ATAN)
		{
		if (NEntered)PushUp(pile);
		pile[1]=atan(nb1);
		m=0;}
	else if (op==KEY_ROOT3)
		{if (NEntered)PushUp(pile);
			pile[1]=pow(fabs(nb1),1.0/3);
			if (nb1<0)
				pile[1]*=-1;
		m=0;}
	else if (op==KEY_INV)
		{if (NEntered)PushUp(pile);
		pile[1]=1./nb1;
		m=0;}
	else{
		m=0;
	}
	if (m && (!NEntered)){
		for (int j=2; j<taillePile-1;j++)
			pile[j]=pile[j+1];}
	if (m==0 && NEntered){(*lastDisplayed)++;}
	else if ((!NEntered) && m && (*(lastDisplayed)>2)){(*lastDisplayed)--;}

}

void dispPile(double * pile,char lastDisplayed, char signeInput, char * bufferInput){ // Put the numbers on the screen
	char * buffer2=malloc(tailleBuffer*sizeof(char));
	dclear(C_WHITE);
	if (signeInput==-1)
		sprintf(buffer2,"-%s",bufferInput);
	else
		sprintf(buffer2,"%s",bufferInput);	
	dtext(WIDTH/40,HEIGHT-(int)((HEIGHT/(taillePile)+1)*(1)),C_BLACK,buffer2);
	char buffer[tailleBuffer];
	for (int i=1;i<taillePile;i++){
		if (i<lastDisplayed){
			
			if (pile[i]==pile[i]){
				double2buffer(pile[i],buffer);
			}else{
				buffer[0]='N';buffer[1]='A';buffer[2]='N';buffer[3]='\0';
			}
			dtext(WIDTH/40,HEIGHT-(int)((HEIGHT/(taillePile))*(i+1)),C_BLACK,buffer);
		}
	}
	dupdate();
	free(buffer2);
	buffer2=NULL;
}


uint findKey(key_event_t event){ // Returns the code for the key pressed.
	uint o=event.key;
	if (event.shift){
		if (o==KEY_SQUARE)
			o=KEY_SQRT;
		if (o==KEY_POWER)
			o=KEY_ROOTN;
		if (o==KEY_LOG)
			o=KEY_10PX;
		if (o==KEY_LN)
			o=KEY_EXPO;
		if (o==KEY_SIN)
			o=KEY_ASIN;
		if (o==KEY_COS)
			o=KEY_ACOS;
		if (o==KEY_TAN)
			o=KEY_ATAN;
		if (o==KEY_LEFTP)
			o=KEY_ROOT3;
		if (o==KEY_RIGHTP)
			o=KEY_INV;
		if (o==KEY_EXP)
			o=KEY_PI;
	}
	return o;

}




void resetBuffer(char * buffer){
	for (int i=0;i<tailleBuffer;i++){
		buffer[i]='\0';
	}
}

void writePileInFS(double * pile, unsigned char lastDisplayed){
	static const uint16_t *path = u"\\\\fls0\\.RPN.sav";
	int fd= BFile_Open(path , BFile_WriteOnly);
	BFile_Write(fd,pile,sizeof(double)*taillePile);
	BFile_Write(fd,&lastDisplayed,sizeof(char)+1);
	BFile_Close(fd);
}

void readPileInFS(double * pile, unsigned char * lastDisplayed){
	struct BFile_FileInfo fileInfo;
	int handle;
	uint16_t foundpath[30];
	int size = sizeof(double)*taillePile+sizeof(char)+1;
	int fd;
	static const uint16_t *filepath = u"\\\\fls0\\.RPN.sav";
	char checkfile =
	BFile_FindFirst(filepath, &handle, foundpath, &fileInfo);
	BFile_FindClose(handle); 
	if (checkfile == -1)
		BFile_Create(filepath, BFile_File, &size);
	else {
		fd = BFile_Open(filepath, BFile_ReadOnly);
		BFile_Read(fd, pile, sizeof(double)*taillePile, 0);
		BFile_Read(fd, lastDisplayed, sizeof(char), sizeof(double)*taillePile);
		BFile_Close(fd);
	}
}

void mainLoop(){
	double * pile=malloc(taillePile*sizeof(double));
	char * bufferInput=malloc(tailleBuffer*sizeof(char));
	for (int i=0;i<taillePile;i++){
		pile[i]=0;
	}
	unsigned char op=0; //A number coding for the operation. 0: nothing 
	char NEntered=False; // Has a number been entered since the last calculation or EXE?
	char signeInput=1; // Signe of the input. 1 for plus and -1 for minus.
	unsigned char itInput=0;
	resetBuffer(bufferInput);
	unsigned char lastDisplayed=1;
	readPileInFS(pile, &lastDisplayed);
	dispPile(pile,lastDisplayed,1, bufferInput); // It displays the numbers before the user press a key.
	while(1){
		op=0;
		key_event_t o;
		o=getkey_opt(215,NULL);// The event of the key pressed goes into o.
		if (numberPressed(o)){
			bufferInput[itInput]=ev2nb(o)+'0';
			bufferInput[itInput+1]='\0';
			itInput++;

		}
		else if (o.key==KEY_DOT){
			bufferInput[itInput]='.';
			bufferInput[itInput+1]='\0';
			itInput++;
		}
		else if (o.key==KEY_DEL){
			if (itInput>0){
				itInput--;
				bufferInput[itInput]='\0';}
			else if (lastDisplayed>1){
				for (int c=1;c<=lastDisplayed;c++){
					pile[c]=pile[c+1];
				}
				pile[lastDisplayed]=0;
				lastDisplayed--;
			}
		}
		else if (o.key==KEY_NEG){
			signeInput=-signeInput;
			op=0;
		}
		else{
			op=findKey(o);  //op is the value of the operation
		}
		pile[0]=str2double(bufferInput,itInput);
		pile[0]*=signeInput;
		if (itInput>0)
			NEntered=1;
		else
			NEntered=0;
		if (op==KEY_EXIT || op==KEY_MENU){
			//gint_osmenu();
			writePileInFS(pile, lastDisplayed);
			break;
			free(bufferInput);
		}
		if ((PileSwitchRotations(pile,op,lastDisplayed))){}
		else if (op==KEY_PI){
			addNumber(3.14159265358979323846264338,pile);
			lastDisplayed++;
		}
		else if ((op==KEY_ENTER) && (bufferInput[0]!='\0')){
			addNumber(pile[0],pile);
			pile[0]=0;
			NEntered=False;
			signeInput=1;
			if (lastDisplayed<taillePile)
				lastDisplayed++;
			resetBuffer(bufferInput);
			itInput=0;
		}
		else if ((lastDisplayed>=2)  &&  op==KEY_ENTER  && (bufferInput[0]=='\0')){
			addNumber(pile[1],pile);
			pile[0]=0;
			NEntered=False;
			signeInput=1;
			if (lastDisplayed<taillePile)
				lastDisplayed++;
			resetBuffer(bufferInput);
			itInput=0;
		}
		else if(op==KEY_ACON){
			resetP(pile);
			lastDisplayed=1;
			resetBuffer(bufferInput);
			signeInput=1;
			itInput=0;
		}
		else if ((lastDisplayed>=2) && op!=0  && (bufferInput[0]=='\0')){
			doOp(pile[1],pile[2],op,pile,NEntered,&lastDisplayed);
			op=0;
			NEntered=False;
			signeInput=1;
			resetBuffer(bufferInput);
			itInput=0;
		}
		else if ((lastDisplayed>=1) && op!=0 && (bufferInput[0]!='\0')){
			doOp(pile[0],pile[1],op,pile,NEntered,&lastDisplayed);
			op=0;
			NEntered=False;
			pile[0]=0;
			signeInput=1;
			resetBuffer(bufferInput);
			itInput=0;
		}
		dispPile(pile,lastDisplayed,signeInput,bufferInput);

	}
	free(pile);
	pile=NULL;
}

int main(){
	#ifdef FXCG50 //load the font for CG-50
	dfont(&font);
	#endif
	mainLoop();
	return EXIT_SUCCESS; 
}