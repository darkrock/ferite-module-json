#include "JSON_header.h"

int Ferite_UTF8_CharacterLength( FeriteString *data, int offset ) {
	if( data->length ) {
		int length = data->length;
		int i = offset;
		unsigned char b1 = data->data[i];
		if( (b1 & 0xC0) == 0xC0 ) {
			unsigned char b2 = ((i + 1) < length ? data->data[i+1] : 0);
			unsigned char b3 = ((i + 2) < length ? data->data[i+2] : 0);
			unsigned char b4 = ((i + 3) < length ? data->data[i+3] : 0);

			// 4 byte
			if( (b1 & 0xF0) == 0xF0 &&
				(b2 & 0x80) == 0x80 &&
				(b3 & 0x80) == 0x80 &&
				(b4 & 0x80) == 0x80 ) {
				return 4;
			}

			// 3 byte
			if( (b1 & 0xE0) == 0xE0 &&
				(b2 & 0x80) == 0x80 &&
				(b3 & 0x80) == 0x80 ) {
				return 3;
			}

			// We do a proper check
			// 2 byte
			if( (b1 & 0xC0) == 0xC0 &&
				(b2 & 0x80) == 0x80 ) {
				return 2;
			}
		}
		return 1;
	}
	return 0;
}
int Ferite_UTF8_CharacterCodePoint( FeriteString *data, int offset ) {
	int characterSize = Ferite_UTF8_CharacterLength( data, offset );
	int characterValue = 0;
	char *character = data->data;
	
	printf("Character size: %d\n", characterSize);
	switch( characterSize ) {
		case 1:
			characterValue =   character[offset + 0];
			break;
		case 2:
			characterValue = ((character[offset + 0] & 0x1F) << 6) + 
							  (character[offset + 1] & 0x3F);
			break;
		case 3:
			characterValue = ((character[offset + 0] & 0xF) << 12) + 
							 ((character[offset + 1] & 0x3F) << 6) + 
							  (character[offset + 2] & 0x3F);
			break;
		case 4:
			characterValue = ((character[offset + 0] & 0x7) << 18) + 
							 ((character[offset + 1] & 0x3F) << 12) + 
							 ((character[offset + 2] & 0x3F) << 6) + 
							  (character[offset + 3] & 0x3F);
			break;
	}
	return characterValue;
}
char _codePointValue[128];
char *Ferite_UTF8_CodePointCharacter( int characterValue ) {
	memset(_codePointValue, 0, 128);
	
	// Position 0 - 127 is equal to percent-encoding with an ASCII character encoding:
	if (characterValue < 128) {
		_codePointValue[0] = characterValue;
	}

	// Position 128 - 2047: two bytes for UTF-8 character encoding.
	if (characterValue > 127 && characterValue < 2048) {
		// First UTF byte: Mask the first five bits of characterValue with binary 110X.XXXX:
		_codePointValue[0] = ((characterValue >> 6) | 0xC0);
		// Second UTF byte: Get last six bits of characterValue and mask them with binary 10XX.XXXX:
		_codePointValue[1] = ((characterValue & 0x3F) | 0x80);
	}

	// Position 2048 - 65535: three bytes for UTF-8 character encoding.
	if (characterValue > 2047 && characterValue < 65536) {
		// First UTF byte: Mask the first four bits of characterValue with binary 1110.XXXX:
		_codePointValue[0] = ((characterValue >> 12) | 0xE0);
		// Second UTF byte: Get the next six bits of characterValue and mask them binary 10XX.XXXX:
		_codePointValue[1] = (((characterValue >> 6) & 0x3F) | 0x80);
		// Third UTF byte: Get the last six bits of characterValue and mask them binary 10XX.XXXX:
		_codePointValue[2] = ((characterValue & 0x3F) | 0x80);
	}

	// Position 65536 - : four bytes for UTF-8 character encoding.
	if (characterValue > 65535) {
		// First UTF byte: Mask the first three bits of characterValue with binary 1111.0XXX:
		_codePointValue[0] = ((characterValue >> 18) | 0xF0);
		// Second UTF byte: Get the next six bits of characterValue and mask them binary 10XX.XXXX:
		_codePointValue[1] = (((characterValue >> 12) & 0x3F) | 0x80);
		// Third UTF byte: Get the last six bits of characterValue and mask them binary 10XX.XXXX:
		_codePointValue[2] = (((characterValue >> 6) & 0x3F) | 0x80);
		// Fourth UTF byte: Get the last six bits of characterValue and mask them binary 10XX.XXXX:
		_codePointValue[3] = ((characterValue & 0x3F) | 0x80);
	}
	return _codePointValue;
}

char _hexValue[128];
char *Ferite_HexValue( int value ) {
	memset(_hexValue, 0, 128);
	sprintf(_hexValue, "\\u%04X", value );
	printf("Hexvalue: %s for %d [%d]\n", _hexValue, value, strlen(_hexValue));
	return _hexValue;
}

FeriteString *Ferite_JSON_EscapeString( FeriteScript *script, FeriteString *data ) {
	size_t length = data->length, i = 0;
	FeriteBuffer *new_data = ferite_buffer_new( script, length * 2 );
	FeriteString *real_data = NULL;
	
	for( i = 0; i < length; i++ ) {
		char current = data->data[i];
		switch( current ) {
			case '"':  ferite_buffer_add_str(script, new_data, "\\\""); break;
			case '\\': ferite_buffer_add_str(script, new_data, "\\\\"); break;
			case '/':  ferite_buffer_add_str(script, new_data, "\\/"); break;
			case '\b': ferite_buffer_add_str(script, new_data, "\\b"); break;
			case '\n': ferite_buffer_add_str(script, new_data, "\\n"); break;
			case '\f': ferite_buffer_add_str(script, new_data, "\\f"); break;
			case '\r': ferite_buffer_add_str(script, new_data, "\\r"); break;
			case '\t': ferite_buffer_add_str(script, new_data, "\\t"); break;
			default: {
				int length = Ferite_UTF8_CharacterLength( data, i );
				int codepoint = Ferite_UTF8_CharacterCodePoint( data, i );
				if( codepoint > 127 ) {
					ferite_buffer_add_str( script, new_data, Ferite_HexValue(codepoint) );
					printf("Length: %d\n", length);
					i += length - 1;
				} else {
					ferite_buffer_add_char(script, new_data, current);
				}
			}
		}
	}
	real_data = ferite_buffer_to_str( script, new_data );
	ferite_buffer_delete( script, new_data );
	printf("String: '%s' [%d, %d]\n", real_data->data, real_data->length, strlen(real_data->data));
	return real_data;
}
FeriteString *Ferite_JSON_Parse_StringToFeriteString( FeriteScript *script, FeriteJSONParser *parser ) {
	char current, next;
	FeriteBuffer *result = ferite_buffer_new(script, 0);
	FeriteString *rresult = NULL;
	
	if( CURRENT_CHAR(script,parser) != '"' )
		return NULL;

	ADVANCE_CHAR(script,parser);
	while( (current = CURRENT_CHAR(script,parser)) != '"' ) {
		if( current == '\\' ) {
			ADVANCE_CHAR(script, parser);
			next = CURRENT_CHAR(script, parser);
			switch( next ) {
				case '"':  ferite_buffer_add_char(script, result, '"'); break;
				case '\\': ferite_buffer_add_char(script, result, '\\'); break;
				case '/':  ferite_buffer_add_char(script, result, '/'); break;
				case 'b':  ferite_buffer_add_char(script, result, '\b'); break;
				case 'n':  ferite_buffer_add_char(script, result, '\n'); break;
				case 'f':  ferite_buffer_add_char(script, result, '\f'); break;
				case 'r':  ferite_buffer_add_char(script, result, '\r'); break;
				case 't':  ferite_buffer_add_char(script, result, '\t'); break;
				case 'u': {
					char buffer[5];
					int value = 0;
					memset( buffer, 0, 5 );
					
					ADVANCE_CHAR(script, parser);
					buffer[0] = CURRENT_CHAR(script, parser);
					ADVANCE_CHAR(script, parser);
					buffer[1] = CURRENT_CHAR(script, parser);
					ADVANCE_CHAR(script, parser);
					buffer[2] = CURRENT_CHAR(script, parser);
					ADVANCE_CHAR(script, parser);
					buffer[3] = CURRENT_CHAR(script, parser);
					
					value = strtol( buffer, NULL, 16 );
					
					ferite_buffer_add_str(script, result, Ferite_UTF8_CodePointCharacter(value));
					break;
				}
			}
		} else {
			ferite_buffer_add_char(script, result, current);
		}
		ADVANCE_CHAR(script, parser);
	}
	ADVANCE_CHAR(script, parser);
	rresult = ferite_buffer_to_str( script, result );
	ferite_buffer_delete( script, result );
	return rresult;
}

FeriteVariable *Ferite_JSON_Parse_Number( FeriteScript *script, FeriteJSONParser *parser ) {
	FeriteBuffer *number = ferite_buffer_new(script, 0);
	FeriteVariable *fnumber = NULL;
	int integer = 0;
	double floatp = 0;
	int isInteger = 1;
	FeriteString *buf = NULL;

	/* Sign */
	CHECK_AND_PUSH( script, parser, number, '-' );
	/* Head */
	if( CURRENT_CHAR(script,parser) != '0' ) {
		SPIN_RANGE( script, parser, number, '1', '9' );
		SPIN_RANGE( script, parser, number, '0', '9' );
	} else {
		CHECK_AND_PUSH( script, parser, number, '0' );
	}
	/* Decimal point and tail */
	if( CURRENT_CHAR(script,parser) == '.' ) {
		isInteger = 0;
		CHECK_AND_PUSH( script, parser, number, '.');
		SPIN_RANGE( script, parser, number, '0', '9' );
	}
	/* Exponent */
	if( CURRENT_CHAR(script,parser) == 'e' || CURRENT_CHAR(script,parser) == 'E' ) {
		ferite_buffer_add_char(script, number, 'E');
		SPIN_RANGE( script, parser, number, '0', '9' );
	}

	/* Convert to a number object */
	buf = ferite_buffer_to_str( script, number );
	if( isInteger ) {
		integer = atol( buf->data );
		fnumber = ferite_create_number_long_variable( script, "number", integer, FE_STATIC );
	} else {
		floatp = atof( buf->data );
		fnumber = ferite_create_number_double_variable( script, "number", floatp, FE_STATIC );
	}
	ferite_buffer_delete( script, number );
	ferite_str_destroy( script, buf );
	return fnumber;
}
FeriteVariable *Ferite_JSON_Parse_String( FeriteScript *script, FeriteJSONParser *parser ) {
	FeriteString *rstr = Ferite_JSON_Parse_StringToFeriteString( script, parser );
	FeriteVariable *vstr = ferite_create_string_variable( script, "string", rstr, FE_STATIC );
	ferite_str_destroy( script, rstr );
	return vstr;
}
FeriteVariable *Ferite_JSON_Parse_Array( FeriteScript *script, FeriteJSONParser *parser ) {
	int position = 0;
	FeriteVariable *array = ferite_create_uarray_variable(script, "array", 0, FE_STATIC);

	ADVANCE_CHAR( script, parser );
	EAT_WHITESPACE( script, parser );
	while( CURRENT_CHAR( script, parser ) != ']' ) {
		FeriteVariable *value = NULL;

		EAT_WHITESPACE( script, parser );
		value = Ferite_JSON_Parse_Value( script, parser );
		ferite_uarray_add( script, VAUA(array), value, NULL, FE_ARRAY_ADD_AT_END );

		EAT_WHITESPACE( script, parser );
		if( CURRENT_CHAR(script,parser) == ',') {
			ADVANCE_CHAR( current, parser );
		}
		EAT_WHITESPACE( script, parser );
	}
	ADVANCE_CHAR(script, parser);
	return array;
}
FeriteVariable *Ferite_JSON_Parse_Object( FeriteScript *script, FeriteJSONParser *parser ) {
	FeriteVariable *object = ferite_new_object(script, ferite_find_namespace_element_contents( script, script->mainns, "JSON.JSONObject", FENS_CLS), NULL);
	FeriteVariable *ovalues = ferite_object_get_var( script, VAO(object), "variables" );
	FeriteString *name = NULL;
	
	parser->depth++;
	ADVANCE_CHAR( script, parser );
	EAT_WHITESPACE( script, parser );
	if( CURRENT_CHAR( script, parser ) == '<' ) {
		name = Ferite_JSON_Parse_ObjectReference( script, parser );
		if( name ) {
			FeriteVariable *oname = ferite_object_get_var( script, VAO(object), "name" );
/*			printf("Found named object %s\n", name->data); */
			ferite_hash_add( script, parser->named_objects, name->data, VAO(object) );
			ferite_str_cpy( script, VAS(oname), name );
			ferite_str_destroy( script, name );
		}
		EAT_WHITESPACE( script, parser );
	}
	while( CURRENT_CHAR( script, parser ) != '}' ) {
		FeriteString *key = Ferite_JSON_Parse_StringToFeriteString( script, parser );
		FeriteVariable *value = NULL;

		EAT_WHITESPACE( script, parser );
		if( CURRENT_CHAR( script, parser ) !=  ':' ) {
			ferite_error( script, 0, "Expecting :" );
			return NULL;
		}
		ADVANCE_CHAR( script, parser );

		EAT_WHITESPACE( script, parser );
		value = Ferite_JSON_Parse_Value( script, parser );
		
		ferite_uarray_add( script, VAUA(ovalues), value, key->data, FE_ARRAY_ADD_AT_END );
		ferite_str_destroy( script, key );

		EAT_WHITESPACE( script, parser );
		if( CURRENT_CHAR(script,parser) == ',') {
			ADVANCE_CHAR( current, parser );
		}
		EAT_WHITESPACE( script, parser );
	}
	ADVANCE_CHAR(script, parser);
	parser->depth--;
	return object;
}
FeriteString *Ferite_JSON_Parse_ObjectReference( FeriteScript *script, FeriteJSONParser *parser ) {
	char current, next;
	FeriteBuffer *result = ferite_buffer_new(script, 0);
	FeriteString *rresult = NULL;
	
	ADVANCE_CHAR(script,parser);
	while( (current = CURRENT_CHAR(script,parser)) != '>' ) {
		ferite_buffer_add_char(script, result, current);
		ADVANCE_CHAR(script, parser);
	}
	ADVANCE_CHAR(script, parser);
	rresult = ferite_buffer_to_str( script, result );
	ferite_buffer_delete( script, result );
	return rresult;
}
FeriteVariable *Ferite_JSON_Parse_Value( FeriteScript *script, FeriteJSONParser *parser ) {
	FeriteVariable *value = NULL;
	EAT_WHITESPACE( script, parser );
	switch( CURRENT_CHAR( script, parser ) ) {
		case '"':
			value = Ferite_JSON_Parse_String( script, parser );
			break;
		case '{':
			value = Ferite_JSON_Parse_Object( script, parser );
			break;
		case '<': {
			FeriteString *name = Ferite_JSON_Parse_ObjectReference( script, parser );
/*			printf("Got referenced object %s\n", name->data); */
			value = ferite_create_object_variable_with_data( script, "object", ferite_hash_get( script, parser->named_objects, name->data ), FE_STATIC );
			ferite_str_destroy( script, name );
			break;
		}
		case '[':
			value = Ferite_JSON_Parse_Array( script, parser );
			break;
		case 't':
			CHECK_TOKEN( script, parser, "true" );
			value = ferite_create_boolean_variable(script, "boolean", FE_TRUE, FE_STATIC);
			break;
		case 'f':
			CHECK_TOKEN( script, parser, "false" );
			value = ferite_create_boolean_variable(script, "boolean", FE_FALSE, FE_STATIC);
			break;
		case 'n':
			CHECK_TOKEN( script, parser, "null" );
			value = ferite_create_object_variable(script, "null", FE_STATIC);
			break;
		default:
			value = Ferite_JSON_Parse_Number( script, parser );
			break;
	}
	return value;
}