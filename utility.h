typedef struct __ferite_json_parser {
	FeriteHash *named_objects;
	char *data;
	size_t size;
	size_t pos;
	size_t depth;
} FeriteJSONParser;

/* Parser MACROS */
#define CURRENT_CHAR( SCRIPT, DS ) (DS)->data[(DS)->pos]
#define ADVANCE_CHAR( SCRIPT, DS ) (DS)->pos++; if(((DS)->size < (DS)->pos)) { return NULL; }
#define REWIND_CHAR( SCRIPT, DS )  (DS)->pos--
#define GET_CHAR( SCRIPT, DS )     CURRENT_CHAR(SCRIPT,DS); ADVANCE_CHAR(SCRIPT,DS)
#define EAT_WHITESPACE( CONTEXT, DS ) \
	do { \
		int ate_some_characters = FE_FALSE; \
		do { \
			ate_some_characters = FE_FALSE; \
			while( (DS)->size > (DS)->pos && \
			       ((DS)->data[(DS)->pos] == ' '  || \
			        (DS)->data[(DS)->pos] == '\n' || \
			        (DS)->data[(DS)->pos] == '\r' || \
			        (DS)->data[(DS)->pos] == '\t') ) { \
				ate_some_characters = FE_TRUE; \
				ADVANCE_CHAR( CONTEXT, DS ); \
			} \
			if( CURRENT_CHAR(CONTEXT,DS) == '/' ) { \
				ate_some_characters = FE_TRUE; \
				ADVANCE_CHAR(CONTEXT,DS); \
				if( CURRENT_CHAR(CONTEXT,DS) == '/' ) { \
					while( CURRENT_CHAR(CONTEXT,DS) != '\n' ) { \
						ADVANCE_CHAR(CONTEXT,DS); \
					} \
				} else if( CURRENT_CHAR(CONTEXT,DS) == '*' ) { \
					char previous_character = 0; \
					ADVANCE_CHAR(CONTEXT,DS); \
					while( CURRENT_CHAR(CONTEXT,DS) != '/' && previous_character != '*' ) { \
						previous_character = GET_CHAR(CONTEXT,DS); \
					} \
				} \
				ADVANCE_CHAR(CONTEXT,DS); \
			} \
		} while( ate_some_characters ); \
	} while(0);
#define CHECK_AND_PUSH( SCRIPT, DS, BUFFER, CHECK ) \
	if( CURRENT_CHAR(SCRIPT,DS) == CHECK ) { \
		ferite_buffer_add_char(SCRIPT, BUFFER, CURRENT_CHAR(SCRIPT,DS)); \
		ADVANCE_CHAR(SCRIPT,DS); \
	}
#define SPIN_RANGE( SCRIPT, DS, BUFFER, LOWER, UPPER ) \
	while( CURRENT_CHAR(SCRIPT,DS) >= LOWER && CURRENT_CHAR(SCRIPT,DS) <= UPPER && ((DS)->size > (DS)->pos) ) { \
		ferite_buffer_add_char(SCRIPT, BUFFER, CURRENT_CHAR(SCRIPT,DS)); \
		ADVANCE_CHAR(SCRIPT,DS); \
	}
#define CHECK_TOKEN( SCRIPT, DS, TOKEN ) \
	{ \
		int loc = 0; \
		for( loc = 0; loc < strlen(TOKEN); loc++ ) { \
			if( CURRENT_CHAR(SCRIPT,DS) == TOKEN[loc] ) { \
				ADVANCE_CHAR(SCRIPT,DS); \
			} else { \
				ferite_error(SCRIPT, 0, "Unable to match complete token!"); \
			} \
		} \
	}

FeriteString *Ferite_JSON_EscapeString( FeriteScript *script, FeriteString *data );
FeriteString *Ferite_JSON_Parse_StringToFeriteString( FeriteScript *script, FeriteJSONParser *parser );
FeriteVariable *Ferite_JSON_Parse_Number( FeriteScript *script, FeriteJSONParser *parser );
FeriteVariable *Ferite_JSON_Parse_String( FeriteScript *script, FeriteJSONParser *parser );
FeriteVariable *Ferite_JSON_Parse_Array( FeriteScript *script, FeriteJSONParser *parser );
FeriteVariable *Ferite_JSON_Parse_Object( FeriteScript *script, FeriteJSONParser *parser );
FeriteString *Ferite_JSON_Parse_ObjectReference( FeriteScript *script, FeriteJSONParser *parser );
FeriteVariable *Ferite_JSON_Parse_Value( FeriteScript *script, FeriteJSONParser *parser );
