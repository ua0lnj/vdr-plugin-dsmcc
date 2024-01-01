#ifndef DECODE_H
#define DECODE_H

/* Current known ASN1 -> MHEG5 mappings */

/* Basic Types */
#define APPLICATION		0x00
#define SCENE			0x01
#define STDID			0x02
#define STDVERSION		0x03
#define OBJECTINFO		0x04

#define ITEMS			0x08

/* External program tags ? */

#define RESIDENT_PROGRAM	0x09

/* Variables */

#define BOOLEAN_VARIABLE	0x0F
#define INTEGER_VARIABLE	0x10
#define STRING_VARIABLE		0x11
#define OBJREF_VARIABLE		0x12
#define CONTENTREF_VARIABLE	0x13


/* Item Types */
#define LINK			0x14
#define STREAM			0x15
#define BITMAP			0x16


#define RECTANGLE		0x19
#define LABEL			0x1D

/* Attributes */

#define BUTTON_REF_COLOUR	0x30
#define HIGHLIGHT_REF_COLOUR	0x31

#define INPUT_EVENT_REGISTER	0x33
#define SCENE_COORDINATE_SYSTEM	0x34
#define ASPECT_RATIO		0x35
#define MOVING_CURSOR		0x36
#define NEXT_SCENES		0x37
#define INITALLY_ACTIVE		0x38
#define CONTENT_HOOK		0x39
#define ORIGINAL_CONTENT	0x3A
#define SHARED			0x3B

/* Link Attributes */
#define LINK_CONDITION		0x3E
#define LINK_EFFECT		0x3F

/* ?? */

#define	GET_DATA_CONTENT	0x43
#define GET_TEXT_CONTENT	0x44

/* TOken Groups */
#define TOKEN_GROUP		0x46
#define TOKEN_GROUP_ITEMS	0x47
#define NO_TOKEN_ACTION_SLOTS	0x48

/* graphic attributes */
#define	ORIGINAL_BOX_SIZE	0x4C
#define ORIGINAL_POSITION	0x4D
#define ORIGINAL_PALETTE_REF	0x4E

#define ORIG_REF_FILL_COLOUR	0x55
#define ORIGINAL_FONT		0x56
#define HORIZONTAL_JUST		0x57
#define VERTICAL_JUST		0x58
#define LINE_ORIENTATION	0x59
#define START_CORNER		0x5A
#define	TEXT_WRAPPING		0x5B

/* stream info */
#define	MULTIPLEX		0x5C
#define	STORAGE			0x5D
#define	LOOPING			0x5E /* ??? */
#define	AUDIO			0x5F /* ??? */
#define VIDEO			0x60

/* video attributes */

#define ORIGINAL_VOLUME		0x62

/* ?? */

#define ENGINE_RESP		0x65

/* Slider attributes */
#define ORIENTATION		0x66
#define MAX_VALUE		0x67
#define MIN_VALUE		0x68
#define INITAL_VALUE		0x69
#define INITAL_PORTION		0x6A
#define STEP_SIZE		0x6B
#define SLIDER_STYLE		0x6C

/* Input attributes */

#define INPUT_TYPE		0x6D
#define CHAR_LIST		0x6E
#define OBSCURED_INPUT		0x6F
#define MAX_LENGTH		0x70

/* Button attributes ?? */
#define ORIGINAL_LABEL		0x71
#define BUTTON_STYLE		0x72

/* ??
#define ACTIVATE		0x73 /* ?? */
#define ADD			0x74 /* ?? */
#define SETDATA			0x76
#define CALL			0x78
#define DEACTIVATE		0x7D /* ?? */

#define GETENGINESUPPORT	0x8E

#define PLAY			0xAD /* ?? */

#define PERSISTENT		0xB1 /* Store/get */
#define PERSISTENT		0xD9

#define FREEZE			0xB9	/* ?? */

#define SETLABEL		0xC1

#define SETDATA			0xD4	/* ?? */
#define BRING_TO_FRONT		0xD8
#define TRANSITION_TO		0xDE

#define GINTEGER		0xE2
#define GSTRING			0xE3
#define GCONTENT		0xE4

#define INDIRECT_REF		0xEC

/* UK MHEG 1.06 May 2003 */

#define SET_BACKGROUND_COLOUR	0xED
#define SET_CELL_POSITION	0xEE
#define SET__INPUT_REGISTER 	0xEF

#endif
