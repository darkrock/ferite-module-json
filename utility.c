#include "JSON_header.h"

FeriteClass *JSONArrayHandler = NULL;

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
static char _codePointValue[128];
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
					i += length - 1;
				} else if( codepoint >= 32 ){
					ferite_buffer_add_char(script, new_data, current);
				}
			}
		}
	}
	real_data = ferite_buffer_to_str( script, new_data );
	ferite_buffer_delete( script, new_data );
	return real_data;
}
FeriteString *Ferite_JSON_Parse_StringToFeriteString( FeriteScript *script, FeriteJSONParser *parser, int *valid_ferite_identifier ) {
	char current, next;
	FeriteBuffer *result = ferite_buffer_new(script, 0);
	FeriteString *rresult = NULL;
	
	if( valid_ferite_identifier )
		*valid_ferite_identifier = FE_TRUE;
	
	if( CURRENT_CHAR(script,parser) != '"' )
		return NULL;

	ADVANCE_CHAR(script,parser);
	while( (current = CURRENT_CHAR(script,parser)) != '"' ) {
		if( current == '\\' ) {
			int success = FE_TRUE;
			
			if( valid_ferite_identifier )
				*valid_ferite_identifier = FE_FALSE;
			
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
					
					if( CURRENT_CHAR(script,parser) != '"' ) {
						buffer[0] = CURRENT_CHAR(script, parser);
						ADVANCE_CHAR(script, parser);
					} else {
						success = FE_FALSE;
					}
					if( CURRENT_CHAR(script,parser) != '"' ) {
						buffer[1] = CURRENT_CHAR(script, parser);
						ADVANCE_CHAR(script, parser);
					} else {
						success = FE_FALSE;
					}
					if( CURRENT_CHAR(script,parser) != '"' ) {
						buffer[2] = CURRENT_CHAR(script, parser);
						ADVANCE_CHAR(script, parser);
					} else {
						success = FE_FALSE;
					}
					if( CURRENT_CHAR(script,parser) != '"' ) {
						buffer[3] = CURRENT_CHAR(script, parser);
					} else {
						success = FE_FALSE;
					}

#ifdef DEBUG_JSON
					printf("%c [%d]\n", buffer[0], buffer[0]);
					printf("%c [%d]\n", buffer[1], buffer[1]);
					printf("%c [%d]\n", buffer[2], buffer[2]);
					printf("%c [%d]\n", buffer[3], buffer[3]);
#endif

					if( success ) {
#ifdef DEBUG_JSON
						printf("Success\n");
#endif
						value = strtol( buffer, NULL, 16 );
						ferite_buffer_add_str(script, result, Ferite_UTF8_CodePointCharacter(value));
					}
					break;
				}
			}
			if( !success ) {
				ferite_error(script, 0, "Error parsing string!");
				break;
			}
		} else {
#ifdef DEBUG_JSON
			printf("%c [%d]\n", current, current);
#endif
			if( valid_ferite_identifier && *valid_ferite_identifier && !(
				(current >= 'a' && current <= 'z') ||
				(current >= 'A' && current <= 'Z') ||
				(current >= '0' && current <= '9') ||
				(current == '_')
			) ) {
				*valid_ferite_identifier = FE_FALSE;
			}
			ferite_buffer_add_char(script, result, current);
		}
		ADVANCE_CHAR(script, parser);
	}
	ADVANCE_CHAR(script, parser);
	rresult = ferite_buffer_to_str( script, result );
	ferite_buffer_delete( script, result );
#ifdef DEBUG_JSON
	printf("Got string: '%s'\n", rresult->data);
#endif
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
		CHECK_AND_PUSH( script, parser, number, '-' );
		CHECK_AND_PUSH( script, parser, number, '+' );
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
	FeriteString *rstr = Ferite_JSON_Parse_StringToFeriteString( script, parser, NULL );
	FeriteVariable *vstr = ferite_create_string_variable( script, "string", rstr, FE_STATIC );
	ferite_str_destroy( script, rstr );
	return vstr;
}
FeriteVariable *Ferite_JSON_Parse_Array( FeriteScript *script, FeriteJSONParser *parser, FeriteObject *root ) {
	int position = 0;
	FeriteVariable *array = ferite_create_uarray_variable(script, "array", 0, FE_STATIC);
	int check_for_attribute_function = (root != NULL ? FE_TRUE : FE_FALSE);
	FeriteClass *root_class = NULL;
	long count = 0;
	
	if( root && root->klass == JSONArrayHandler ) {
		FeriteVariable *_root_class = ferite_object_get_var(script, root, "root_class");
		if( _root_class ) {
			root_class = VAC(_root_class);
		}
	}
	
	ADVANCE_CHAR( script, parser );
	EAT_WHITESPACE( script, parser );
	while( CURRENT_CHAR( script, parser ) != ']' ) {
		FeriteVariable *value = NULL;

		FeriteObject *new_root = NULL;
		
		if( root ) {
			if( root_class == NULL ) {
				FeriteVariable **parameter_list = (check_for_attribute_function ? ferite_create_parameter_list_from_data( script, "l", count, NULL ) : NULL);
				FeriteFunction  *objectForArrayElementFunction = (check_for_attribute_function ? ferite_object_get_function_for_params( script, root, "objectForArrayElement", parameter_list ) : NULL);
		
				if( objectForArrayElementFunction ) {
					FeriteVariable *return_value = ferite_call_function( script, root, NULL, objectForArrayElementFunction, parameter_list );
					if( return_value ) {
						if( return_value->type == F_VAR_OBJ ) {
							new_root = VAO(return_value);
							if( new_root ) {
								FINCREF(new_root);
							}
						}
						ferite_variable_destroy( script, return_value );
					}
				} else {
					check_for_attribute_function = FE_FALSE;
				}

				if( parameter_list ) {
					ferite_delete_parameter_list( script, parameter_list );
				}
			} else {
				FeriteVariable *_new_root = ferite_new_object(script, root_class, NULL);
				if( _new_root ) {
					if( VAO(_new_root) ) {
						new_root = VAO(_new_root);
						FINCREF(new_root);
					}
					ferite_variable_destroy( script, _new_root );
				}
			}
		}
		
		EAT_WHITESPACE( script, parser );
		value = Ferite_JSON_Parse_Value( script, parser, new_root );
		CHECK_ERROR(script);

		ferite_uarray_add( script, VAUA(array), value, NULL, FE_ARRAY_ADD_AT_END );

		if( new_root ) {
			FDECREF(new_root);
		}

		EAT_WHITESPACE( script, parser );
		if( CURRENT_CHAR(script,parser) == ',') {
			ADVANCE_CHAR( current, parser );
		}
		EAT_WHITESPACE( script, parser );
		count++;
	}
	ADVANCE_CHAR(script, parser);
	return array;
}
FeriteVariable *Ferite_JSON_Parse_Object( FeriteScript *script, FeriteJSONParser *parser, FeriteObject *root ) {
	FeriteVariable *object = (root ? 
		ferite_create_object_variable_with_data( script, "JSON Object", root, FE_STATIC ) :
		ferite_new_object(script, ferite_find_namespace_element_contents( script, script->mainns, "JSON.Object", FENS_CLS), NULL) 
	);
	FeriteVariable *ovalues = ferite_object_get_var( script, VAO(object), "_JSONVariables" );
	FeriteString *name = NULL;
	int seen_reference = FE_FALSE;
	int check_for_attribute_function = FE_TRUE;

	if( root ) {
		MARK_VARIABLE_AS_DISPOSABLE(object);
	}
	
	parser->depth++;
	ADVANCE_CHAR( script, parser );
	EAT_WHITESPACE( script, parser );
	while( CURRENT_CHAR( script, parser ) != '}' ) {
		int valid_ferite_identifier = FE_TRUE;
		FeriteString *key = Ferite_JSON_Parse_StringToFeriteString( script, parser, &valid_ferite_identifier );
		FeriteVariable *value = NULL;

		CHECK_ERROR(script);

		EAT_WHITESPACE( script, parser );
		if( CURRENT_CHAR( script, parser ) !=  ':' ) {
			ferite_error( script, 0, "Expecting :" );
			return NULL;
		}
		ADVANCE_CHAR( script, parser );

		EAT_WHITESPACE( script, parser );
		
		if( !seen_reference && strcmp(key->data, ":reference") == 0 ) {
			FeriteString *object_id = Ferite_JSON_Parse_StringToFeriteString( script, parser, NULL );
			FeriteObject *target = ferite_hash_get( script, parser->named_objects, object_id->data );
			
			if( target ) {
				FDECREFI(VAO(object), object->refcount);
				VAO(object) = target;
				FINCREFI(VAO(object), object->refcount);
			} else {
				FeriteVariable *oname = ferite_object_get_var( script, VAO(object), "_JSONName" );
				if( oname ) {
					ferite_str_cpy( script, VAS(oname), object_id );
				}
				ferite_hash_add( script, parser->named_objects, object_id->data, VAO(object) );
			}
			ferite_str_destroy( script, object_id );
		} else {
			FeriteObject    *new_root = NULL;
			FeriteVariable **parameter_list = (check_for_attribute_function ? ferite_create_parameter_list_from_data( script, "c", key->data, NULL ) : NULL);
			FeriteFunction  *objectForAttributeFunction = (check_for_attribute_function ? ferite_object_get_function_for_params( script, root, "objectForAttribute", parameter_list ) : NULL);
			FeriteVariable  *existing_attribute = ferite_object_get_var( script, VAO(object), key->data );
			
			if( objectForAttributeFunction ) {
				FeriteVariable *return_value = ferite_call_function( script, root, NULL, objectForAttributeFunction, parameter_list );
				if( return_value ) {
					if( return_value->type == F_VAR_OBJ ) {
						new_root = VAO(return_value);
						if( new_root ) {
							FINCREF(new_root);
						}
					}
					ferite_variable_destroy( script, return_value );
				}
			} else {
				check_for_attribute_function = FE_FALSE;
			}

			if( parameter_list ) {
				ferite_delete_parameter_list( script, parameter_list );
			}
			
			value = Ferite_JSON_Parse_Value( script, parser, new_root );
			CHECK_ERROR(script);
			
			if( existing_attribute && root != NULL ) {
				if( !ferite_variable_fast_assign( script, existing_attribute, value ) ) {
					ferite_error( script, 0, "Incompatible types between variable and JSON value for variable %s\n", key->data );
				}
				ferite_variable_destroy( script, value );
			} else if( valid_ferite_identifier && root != NULL ) {
				ferite_object_set_var( script, VAO(object), key->data, value );
			} else if( ovalues ) {			
				ferite_uarray_add( script, VAUA(ovalues), value, key->data, FE_ARRAY_ADD_AT_END );
			} else {
				ferite_variable_destroy( script, value );
			}
			
			if( new_root ) {
				FDECREF(new_root);
			}

			CHECK_ERROR(script);
		}
		seen_reference = FE_TRUE;

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
FeriteVariable *Ferite_JSON_Parse_Value( FeriteScript *script, FeriteJSONParser *parser, FeriteObject *root ) {
	FeriteVariable *value = NULL;
	EAT_WHITESPACE( script, parser );
	switch( CURRENT_CHAR( script, parser ) ) {
		case '"':
			value = Ferite_JSON_Parse_String( script, parser );
			break;
		case '{':
			value = Ferite_JSON_Parse_Object( script, parser, root );
			break;
		case '[':
			value = Ferite_JSON_Parse_Array( script, parser, root );
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
	CHECK_ERROR(script);
	return value;
}