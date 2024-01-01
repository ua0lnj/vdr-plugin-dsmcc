#include <stdio.h>
#include <stdlib.h>

#include "decode.h"

#define UNIVERSAL 0
#define CONTEXT   1

#define DEFINITE   0
#define INDEFINITE 1

#define PRIMITIVE   0
#define CONSTRUCTED 1

#define NONE 0
#define TAG 1
#define LENGTH 2
#define LENGTH_OCTET 3
#define CONTENT 4

#define BOOL 1
#define INTEGER 2
#define STRING 4
#define ENUM 10
#define SEQUENCE 16

#define ENDCONTENT 0

struct tag_str {
	unsigned char class;
	unsigned char encoding;
	unsigned long type;
	unsigned long type_len;
	unsigned long length;
	unsigned long length_len;
	unsigned long length_form;
	unsigned long length_octets;
	union {
		char *string;
		unsigned int integer;
		unsigned char bool;
		unsigned char enumer;
	} val;
	struct tag_str *next, *prev;
	int count;
};

static int tabstops = 0;
static int seqtabs = 0;

struct tag_str *ProcessTag(struct tag_str *tag);

void pushtab();
void poptab();
void tabs();

void pushseq();
void popseq ();
void tabseqs();

void pushtab() {
	tabstops++;
}

void poptab() {
	tabstops--;
}

void tabs() {
	int i;

	for(i=0;i<tabstops;i++) 
		printf("\t");
}

void pushseq() {
	seqtabs++;
}

void popseq() {
	seqtabs--;
}

void tabseqs() {
	int i;

	if(seqtabs>0)
		printf("\n");

	for(i=0;i<seqtabs;i++) 
		printf("\t");
}

int main(int argc, char *argv[]) {
	FILE *fd;
	unsigned int ch, ch2;
	struct tag_str tag;
	struct tag_str *tagstack, *last, *newtag;
	unsigned char state = NONE;

	

	tagstack = last = newtag = NULL;
	tag.val.string = NULL;
	tag.val.integer = tag.val.bool = 0;
	tag.type_len = 0;

	if((fd = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Failed to open file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	while((ch = fgetc(fd)) != EOF) {

				
		if(state == NONE) {
			if((ch & 0xC0) == 0x80) {
				tag.class = CONTEXT;
			} else if((ch & 0xC0) == 0) {
				tag.class = UNIVERSAL;
			} else {
				printf("Unknown tag\n");
			}
			if((ch & 0x20) == 0) {
				tag.encoding = PRIMITIVE;
			} else {
				tag.encoding = CONSTRUCTED;
			}
			if((ch & 0x1F) == 0x1F) {
				state = TAG;
			} else {
				tag.type = ch & 0x1F;
				state = LENGTH;
			}
			tag.val.string = NULL;
			tag.val.integer = tag.val.bool = 0;
			tag.length_form = DEFINITE;
		} else if(state == TAG) {
			if((ch & 0x80) == 0x80) {
				/* copy last 7 bits */
				tag.type_len += 7;
				tag.type = ((ch & 0x7F) << (32-tag.type_len)); 
			} else {
				/* copy last 7 bits and shift all down */
				tag.type_len += 7;
				tag.type|=((ch&0x7F) << (32-tag.type_len));
				tag.type = (tag.type >> (32-tag.type_len));
				state = LENGTH;
				tag.type_len = 0;
			}
		} else if(state == LENGTH) {
			if(ch & 0x80) {
				/* copy last 7 bits and shift all down */
				tag.length_octets = (ch & 0x7F);
				tag.length = tag.length_len = 0;
				state = LENGTH_OCTET;
			} else {
				/* copy last 7 bits */
				tag.length = ch & 0x7F;
				if(tag.length == 0) {
					tag.length_form = INDEFINITE;
				}
				if((tag.class==UNIVERSAL)&&(tag.type==STRING)) {
					if(tag.length > 0) {
					  tag.val.string = malloc(tag.length+1);
					  tag.count = 0;
					  state = CONTENT;
					} else {
					  tag.val.string = NULL;
					  ProcessTag(&tag);
					  state = NONE;
					}
				} else {
					tag.count = 0;
					state = CONTENT;
				}
			}
		} else if(state == LENGTH_OCTET) {
			tag.length_len+=8;
			tag.length |= (ch << (32 - tag.length_len));
			tag.length_octets--;
			if(tag.length_octets == 0) {
				tag.length =(tag.length>>(32-tag.length_len));
				tag.count = 0;
				state = CONTENT;
				if((tag.class==UNIVERSAL)&&(tag.type==STRING)) {
					tag.val.string = malloc(tag.length+1);
				}
			} else {
				state = LENGTH_OCTET;
			}
		} else if(state == CONTENT) {
			if(tag.length_form == INDEFINITE) {
			  if((ch == ENDCONTENT) && ((ch2=fgetc(fd))==ENDCONTENT)){
			    ProcessTag(&tag);
			    if(last->prev != NULL) {
				last->prev->next = NULL;
				free(last);
			    } else {
				free(last);
				tagstack = NULL;
			    }
			    state = NONE;
			  }
			} else {

			  switch(tag.type) {
			
			    case BOOL:
				tag.val.bool = ch;
			  	ProcessTag(&tag);
				state = NONE;
				break;
			    case ENUM:
				tag.val.enumer = ch;
			  	ProcessTag(&tag);
				state = NONE;
				break;
			    case INTEGER:
				tag.val.integer|=(ch<<((tag.length-(1+tag.count++))*8));
				if(tag.count == tag.length) {
				  	ProcessTag(&tag);
					state = NONE;
				}
				break;
			    case STRING:
				*(tag.val.string+tag.count++) = ch;
				if(tag.count == tag.length) {
					*(tag.val.string+tag.count) = '\0';
				  	ProcessTag(&tag);
					free(tag.val.string);
					state = NONE;
				}
				break;
			    case SEQUENCE:
				/* Handled below */
				break;
			    default:
				break;
			  }
			}
		}

		if(state == CONTENT)  {
		  if(tag.class == CONTEXT) {
			struct tag_str *nexttag;
			/* Add tag to stack */
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = CONTEXT;
			newtag->type = tag.type;
			newtag->length = tag.length;
			newtag->next = newtag->prev = NULL;
			newtag->count = -1;
			if(tagstack == NULL) {
			  tagstack = newtag;
			} else {
			  for(last=tagstack;last->next!=NULL;last=last->next){;}
			  last->next = newtag;
			  newtag->prev = last;
			}
			nexttag = ProcessTag(newtag);
			switch(newtag->type) {
			case 0x00:
			case 0x01:
			case 0x08:
			case 0x09:
			case 0x10:
			case 0x11:
			case 0x12:
			case 0x13:
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x19:
			case 0x1D:
				pushtab();
				break;
			}
			if(nexttag != NULL) {
				tag.class = nexttag->class;
				tag.type = nexttag->type;
				tag.length = nexttag->length;
				tag.val = nexttag->val;
				state = CONTENT;
			} else {
				state = NONE;
			}
		  } else if((tag.class == UNIVERSAL)&&(tag.type == SEQUENCE)) {
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = tag.type;
			newtag->length = tag.length;
			newtag->next = newtag->prev = NULL;
			newtag->count = -1;
			if(tagstack == NULL) {
		  	  tagstack = newtag;
			} else {
		  	  for(last=tagstack;last->next!=NULL;last=last->next){;}
		  	  last->next = newtag;
		  	  newtag->prev = last;
			}
			for(;last!=NULL;last=last->prev) {
				if((last->class==UNIVERSAL) && (last->type==SEQUENCE)) 
					pushseq();
					break;
			}
			ProcessTag(newtag);
			state = NONE;
		  }
		}

		/* Add 1 byte to all in tagstack */
		if(tagstack != NULL) 
			for(last=tagstack;last->next!=NULL;last=last->next) {;}
		else
			last = NULL;

		while(last!=NULL) {
			struct tag_str *ntag;

			last->count++;
			if(last->length == last->count) {
				/* Output end-bracket and pop tag */
				if((last->class == UNIVERSAL) && (last->type == SEQUENCE)) {
					if(last->prev && last->prev->class == UNIVERSAL && last->prev->type == SEQUENCE) {
						popseq();
						tabseqs();
						printf(") ");
					 } else 
						printf(") ");
				} else if(last->class == CONTEXT) {
					switch(last->type) {
					case 0x00:
					case 0x0F:
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13:
					case 0x14:
					case 0x16:
					case 0x19:
					case 0x1D:
					case 0x21:
					case 0x47:
						poptab();
						tabs();
						printf("}\n");
						break;
					case 0x08:
						tabs();
						printf(")\n");
						break;
					case 0x01:
					case 0x02:
					case 0x03:
					case 0x04:
					case 0x27:
					case 0x29:
					case 0x2B:
					case 0x3A:
					case 0x3B:
					case 0x39:
					case 0x43:
					case 0x4C:
					case 0x4D:
					case 0x52:
					case 0x54:
					case 0x55:
					case 0x57:
					case 0xC1:
					case 0xCF:
					case 0xD8:
						printf("\n");
						break;
					default:
						break;
					}
				} 

				if(last->prev != NULL) {
					if(last->next != NULL)  {
						last->prev->next = last->next;
						last->next->prev = last->prev;
					} else {
						last->prev->next = NULL;
					}
					
				} else {
					if(last->next != NULL)  {
						tagstack = last->next;
						last->next->prev = NULL;
					}
				}
				ntag = last->prev;
				free(last);
				last = ntag;
				continue;
			}
			last = last->prev;
		}

	}

}

struct tag_str *ProcessTag(struct tag_str *tag) {
	struct tag_str *newtag = NULL;

	fflush(stdout);

	if(tag->class == CONTEXT) {
		switch(tag->type) {
			
		case 0x00:
			tabs();
			printf("{ ");
			printf(":Application ");
			break;
		case 0x01:
			tabs();
			printf(":Scene ");
			break;
		case 0x02:
			tabs();
			break;
		case 0x03:
			tabs();
			printf(":StdID ");
			break;
		case 0x04:
			tabs();
			printf(":StdVersion ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = STRING;
			newtag->length = tag->length;
			newtag->next = newtag->prev = NULL;
			newtag->val.string = malloc(newtag->length+1);
			newtag->count = -1;
			break;
		case 0x05:
			tabs();
			printf(":ObjectInfo ");
			break;
		case 0x06:
			tabs();
			printf(":Unknown(0x06) ");
			break;
		case 0x07:
			tabs();
			printf(":Unknown(0x07) ");
			break;
		case 0x08:
			tabs();
			printf(":Items ( \n");
			break;
		case 0x09:
			tabs();
			printf(":ResidentProgram ");
			break;
		case 0x0A:
			tabs();
			printf(":Unknown(0x0A) ");
			break;
		case 0x0B:
			tabs();
			printf(":Unknown(0x0B) ");
			break;
		case 0x0C:
			tabs();
			printf(":Unknown(0x0C) ");
			break;
		case 0x0D:
			tabs();
			printf(":Unknown(0x0D) ");
			break;
		case 0x0E:
			tabs();
			printf(":Unknown(0x0E) ");
			break;
		case 0x0F:
			tabs();
			printf("{ :BooleanVariable ");
			break;
		case 0x10:
			tabs();
			printf("{ :IntegerVariable ");
			break;
		case 0x11:
			tabs();
			printf("{ :OctetStringVariable ");
			break;
		case 0x12:
			tabs();
			printf("{ :ObjRefVariable ");
			break;
		case 0x13:
			tabs();
			printf("{ :ContentRefVariable ");
			break;
		case 0x14:
			tabs();
			printf("{ :Link ");
			break;
		case 0x15:
			tabs();
			printf(":Stream ");
			break;
		case 0x16:
			tabs();
			printf("{ :Bitmap ");
			break;
		case 0x19:
			tabs();
			printf("{ :Rectangle ");
			break;
		case 0x1D:
			tabs();
			printf("{ :Label ");
			break;
		case 0x21:
			tabs();
			printf("{ :0x21 ");
			break;
		case 0x27:
			tabs();
			printf(":Background-Colour ");
			break;
		case 0x29:
			tabs();
			printf(":Text-Colour ");
			break;
		case 0x2B:
			tabs();
			printf(":Font-Attributes ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = STRING;
			newtag->length = tag->length;
			newtag->val.string = malloc(tag->length+1);
			newtag->next = newtag->prev = NULL;
			newtag->count = -1;
			break;
		case 0x38:
			tabs();
			printf(":Initially-Active ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = BOOL;
			newtag->length = tag->length;
			newtag->next = newtag->prev = NULL;
			newtag->count = -1;
			break;
		case 0x3A:
			tabs();
			printf(":Original-Content ");
			break;
		case 0x3B:
			tabs();
			printf(":Shared ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = BOOL;
			newtag->length = tag->length;
			newtag->next = newtag->prev = NULL;
			newtag->count = -1;
			break;
		case 0x39:
			tabs();
			printf(":Content-Hook ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = BOOL;
			newtag->length = tag->length;
			newtag->next = newtag->prev = NULL;
			newtag->count = -1;
			break;
		case LINK_CONDITION:
			tabs();
			printf(":Link-Condition ");
			break;
		case LINK_EFFECT:
			tabs();
			printf(":Link-Effect");
			break;
		case 0x43:
			tabs();
			printf(":?GetDataContent? ");
			break;
		case 0x47:
			tabs();
			printf("{ :?Token-Group-Items? ");
			break;
		case 0x4C:
			tabs();
			printf(":Original-Box-Size ");
			break;
		case 0x4D:
			tabs();
			printf(":Original-Position ");
			break;
		case 0x52:
			tabs();
			printf(":0x52 ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = BOOL;
			newtag->length = tag->length;
			newtag->next = newtag->prev = NULL;
			break;
		case 0x54:
			tabs();
			printf(":0x54 ");
			break;
		case 0x55:
			tabs();
			printf(":Orig-Ref-FillColor ");
			break;
		case 0x57:
			tabs();
			printf(":Horizontal-Justification ");
			newtag = malloc(sizeof(struct tag_str));
			newtag->class = UNIVERSAL;
			newtag->type = INTEGER;
			newtag->length = tag->length;
			newtag->next = newtag->prev = NULL;
			break;
		case 0xC1:
			tabs();
			printf(":SetLabel ");
			break;
		case 0xCF:
			tabs();
			printf(":0xCF ");
			break;
		case 0xD8:
			tabs();
			printf(":?BringToFront? ");
			break;
		default:
			tabs();
			printf(":Unknown ");
			break;
		}
	} else if(tag->class == UNIVERSAL) {
		int i;

		switch(tag->type) {
		
		case BOOL:
			printf("(%X) ", tag->val.bool);
			break;
		case ENUM:
			printf("(%X) ", tag->val.enumer);
			break;
		case INTEGER:
			printf("%d ", tag->val.integer);
			break;
		case STRING:
			if(tag->val.string != NULL)
				printf("\"%s\" ", tag->val.string);
			else 
				printf("\"\" ");
/*
			printf("\"");
			for(i=0;i<tag->length;i++) 
				printf("%X", tag->val.string[i]);
			printf("\" ");
*/
			break;
		case SEQUENCE:
			tabseqs();
			printf("( ");
			break;
		default:
			printf("asntype() ");
			break;
		}
		
	}

	return newtag;
}
	
