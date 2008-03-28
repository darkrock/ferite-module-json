#include "jsonparser_header.h"

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
			default:
				ferite_buffer_add_char(script, new_data, current);
		}
	}
	real_data = ferite_buffer_to_str( script, new_data );
	ferite_buffer_delete( script, new_data );
	return real_data;
}
FeriteString *Ferite_JSON_Parse_StringToFeriteString( FeriteScript *script, FeriteJSONParser *parser ) {
	char current, next;
	FeriteBuffer *result = ferite_buffer_new(script, 0);
	FeriteString *rresult = NULL;
	
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
				case 'u':  ferite_buffer_add_str(script, result, "\\u"); break;
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
	FeriteVariable *object = ferite_create_uarray_variable(script, "object", 0, FE_STATIC);

	parser->depth++;
	ADVANCE_CHAR( script, parser );
	EAT_WHITESPACE( script, parser );
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
		
		ferite_uarray_add( script, VAUA(object), value, key->data, FE_ARRAY_ADD_AT_END );
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