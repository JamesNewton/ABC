// ABC: A human readable byte-code language
// http://t...content-available-to-author-only...d.org/techref/idea/minimalcontroller.htm

#include <stdio.h>
//yeah I know. But I think these are cool, so:
#define TRUE  (1==1)
#define FALSE (!TRUE)
//registers are made up of bytes, so 1 is the default size, this is really the
//spacing /between/ registers, so the maximum size each register can process.
//A size of 4 means 32 bit operations will be possible. 8 would be 64.
//This doesn't matter until the op width can be set.
#define REG_SIZE 4
//size of the program memory
#define MEM_SIZE 1000
//amount of memory to include after 'z' register. Stack grows here
#define STACK_SIZE 100
int num;
char dst, src; 
unsigned char op;
char flag;
#define REG_SIZE_CHARS (1+'z'-'a')*REG_SIZE+STACK_SIZE
#define REG_SIZE_INTS REG_SIZE_CHARS / sizeof(int)
union reg_t { char c[REG_SIZE_CHARS]; int i[REG_SIZE_INTS]; } reg;
//1 indexed array of registers. 0 is "no register found"
//Code memory for programs. Could be FLASH or EEPROM
char mem[MEM_SIZE];
char *radix; //radix. "r" in the register array
int *pc; //program counter pointer. "p" in the register array
int next; //pointer to free location in memory for the next instruction. 
char c; //hold the current character being processed

int is_white_space(char c) {
	if (' '==c) return TRUE; //spaces have no effect
	if ('\r'==c) return TRUE; //\n ends a line \r is extra, need to skip it
	return FALSE;
}
char get_inst() { //get the next byte code instruction
	if (*pc) { //if the pc isn't null
		return c = mem[*pc++]; //get pre-loaded code from RAM
		//TODO when out of data from ram, switch to external input
		}
	//wait for the console. 
	while (is_white_space(c = getchar()));//printf("got %d\n", c);
	if (c == EOF) c = FALSE;
	//TODO add external input to end of RAM for backtracking?
	return c; 
}
void skip_line() {
	printf(" skip ");
	get_inst(); 
	while( c != 0 //End of File
		&& c != '\n' //End of Line
		&& c != '!' //"else"
		) { 
		get_inst(); 
		}
	printf(" past %d\n", c);
	get_inst(); 
}
int if_number(char c) {
	if ((c>='0' && c<='9') || (c>='A' && c<='F')) return TRUE;
	return FALSE;
}
void number() {
	while (if_number(c) ) {
		char digit = c-'0';
		if (digit>9) digit -= 'A'-'9';
		if (digit > *radix) {
			printf("\nERR: #%d > r%d\n",digit,*radix);
			return;
		}
		num *= *radix;
		num += digit;
		printf("#%d.",num);
		get_inst(); //if you used it, replace it.
	}
}
int is_register(char c) {
	if (c>='a' && c<='z') return TRUE;
	return FALSE;
}
int is_src_dst_reg(char sd) {
	if (sd >= 1 && sd <=26) return TRUE;
	return FALSE;
}
int registers() {
	char r = FALSE;
	if (is_register(c)) {//register
		r = 1 + c-'a'; //1 to 26
		r = r * REG_SIZE; //allocate some space between each register
		r += num; //if a number was given, offset by that amount. 4a is a+4
		//TODO: 'a' will end up as zero, which flags no destination?
		num = 0;
		get_inst(); //if you used it, replace it.
		printf(" r:%d.",r);
	}
	return r;

}
#define FIRST_DEVICE 128
int devices() {
	char d = FALSE;
	if (c>='A' && c<='Z') {//device
		d = c - 'A';
		d += FIRST_DEVICE;
		get_inst(); //if you used it, replace it.
		printf(" D:%d.",d);
	}
	return d;
}
void destination() {
	dst = registers();
	if (!dst) dst = devices();
	printf(" dst:%d.", dst);
}
void operation() {
	if ((c>' ' && c<'0') //after ctrl codes, but before numbers
	 || (c>'9' && c<'A') //after numbers, but before uppercase letters
	 || (c>'Z' && c<'a') //after uppercase, but below lowercase letters
	 || (c>'a') //after lowercase letters.
	 ) { //we have an operation
	 	op = c;
		get_inst(); //if you used it, replace it.
		if (c == op) { op+=128; get_inst();} //duplicate? set high bit
		if ('=' == c && '<' == op) { op = 'L'; get_inst();} //less than or equal
		if ('=' == c && '>' == op) { op = 'G'; get_inst();} //greater than or equal
		if ('\"' == op) { printf("\nString:\"");//quote
			if (is_src_dst_reg(dst)) { //move source to destination until EOF or ending "
				reg.i[(int)dst] = next; //point register to memory
				while(('\"' != c) && (EOF != c)) { printf("%c", c); //TODO also check *pc
					mem[next++] = c;
					get_inst();
				} 
			get_inst(); //move past closing quote
			printf("\"\n");
			}
		}
		printf(" op:%d.c:%d",op,c);
	 }
}
void source() {
	src = registers();
	if (!src) src = devices();
	printf(" src:%d.", src);
}
int read_device(int src) {
	//TODO read devices. 
	return 0;
}
void do_it() {
	//convert src and dst into the values at thier addresses for width
	//TODO support widths
	int src_val = 0;
	if (src <= sizeof(reg.c)) { //register
		src_val = reg.c[(int)src] + num; printf("src:%d %c val:%d. ", src, src/REG_SIZE, src_val);
	} else { //otherwise it's a device
		src_val = read_device(src);
	}
	int dst_val = reg.c[(int)dst]; 
	//use num if no source
	switch(op) {
		case ':': dst_val = src_val; break;
		case '+': dst_val += src_val; break;
		case '-': dst_val -= src_val; break;
		case '*': dst_val *= src_val; break;
		case '/': dst_val /= src_val; break;
		case '|': dst_val |= src_val; break;
		case '&': dst_val &= src_val; break;
		case '>'+128: dst_val >>= src_val; break;
		case '<'+128: dst_val <<= src_val; break;
		case '=': flag = (dst_val == src_val); break;
		case '>': flag = (dst_val > src_val); break;
		case '<': flag = (dst_val < src_val); break;
		case 'G': flag = (dst_val >= src_val); break;
		case 'L': flag = (dst_val <= src_val); break;
		case '(': break; //parameter list (register indirection)
		case ')': break; //call
	}
	//TODO support widths
	if (dst <= sizeof(reg.c)) { //register
		reg.i[(int)dst] = dst_val;
	} else { //otherwise it's a device
		switch(dst-FIRST_DEVICE+'A') {
			case 'P': //Port/pin
				break;
		}
	}
	printf("dst:%d %c val:%d.\n", dst, dst/REG_SIZE, dst_val);
	//look ahead for conditionals //TODO should this be under operation?
	switch(c) {
		case '?': 
			if (flag) get_inst(); //continue with next
			else skip_line(); //fail
			dst = op = src = num = 0;
			break;
		case '!':
			skip_line();
			break;
		case '.': c=0; return;//TODO up stack to top. For now, just return
	}
}

int done() { 
	if (dst && op &&  (src || num)) {
		printf("\nDO: %c+%d %c= %c %d +#: %d. c:%c %d\n", dst/REG_SIZE+'`', dst % REG_SIZE, op, src/REG_SIZE+'`', src % REG_SIZE, num, c, c);
		do_it();
		op = src = 0;
	}
	if ('\n' == c) { printf(" line end\n");
		get_inst(); //if you used it, replace it.
		num = dst = src = op = 0;
		return TRUE; 
	}
	if (0 == c) { printf(" file end\n");
		return TRUE; 
	}
	if (0 == dst) { printf(" lost dst ");
		return TRUE;
	}
	return FALSE; 
	
}

//register letter to address in reg array. 1 based so 0 is false.
#define REG(x) ((1+x-'a')*REG_SIZE)

int main(void) {
	radix = &reg.c[REG('r')]; //register 'r' stores the radix
	*radix = 10;
	printf("radix %d (%d)\n", *radix, reg.c[REG('r')]);
	pc = (int *)&reg.c[REG('p')]; //questionable. TODO Better way?
	*pc = 0; //start accepting bytcodes from console, not memory
	printf("program counter %d (%d)\n", *pc, reg.c[REG('p')]);
	//next = (int *)&reg.c[REG('n')]; //TODO Better way?
	next = 0;
	dst = src = op = 0;
	flag = FALSE;
	get_inst(); //prime the pump
	//return 0;
	while(c) {
		number(); //prefix numbers add to reg address. e.g. 4a is b, 2P is 2nd port
		destination(); //register or device to send data to.
		do {
			operation(); //operation to perform on source for dest
			number(); //offset for source
			source(); //source value
		} while(!done()); //do we have everything? do it! repeat op and source
	}
	printf("mem: %s.",mem);
	return 0;
}
