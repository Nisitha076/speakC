#include "../include/token.h"

const char *token_type_name(TokenType type) {
    switch (type) {
        // Keywords
        case TOKEN_SET:           return "TOKEN_SET";
        case TOKEN_TO:            return "TOKEN_TO";
        case TOKEN_AS:            return "TOKEN_AS";
        case TOKEN_DEFINE:        return "TOKEN_DEFINE";
        case TOKEN_FUNCTION:      return "TOKEN_FUNCTION";
        case TOKEN_RETURNS:       return "TOKEN_RETURNS";
        case TOKEN_STRUCTURE:     return "TOKEN_STRUCTURE";
        case TOKEN_OF:            return "TOKEN_OF";
        case TOKEN_DECLARE:       return "TOKEN_DECLARE";
        case TOKEN_POINTER:       return "TOKEN_POINTER";
        case TOKEN_ADDRESS:       return "TOKEN_ADDRESS";
        case TOKEN_VALUE:         return "TOKEN_VALUE";
        case TOKEN_AT:            return "TOKEN_AT";
        case TOKEN_ARRAY:         return "TOKEN_ARRAY";
        case TOKEN_IF:            return "TOKEN_IF";
        case TOKEN_ELSE:          return "TOKEN_ELSE";
        case TOKEN_WHILE:         return "TOKEN_WHILE";
        case TOKEN_FOR:           return "TOKEN_FOR";
        case TOKEN_DO:            return "TOKEN_DO";
        case TOKEN_SWITCH:        return "TOKEN_SWITCH";
        case TOKEN_ON:            return "TOKEN_ON";
        case TOKEN_CASE:          return "TOKEN_CASE";
        case TOKEN_DEFAULT:       return "TOKEN_DEFAULT";
        case TOKEN_BREAK:         return "TOKEN_BREAK";
        case TOKEN_RETURN:        return "TOKEN_RETURN";
        case TOKEN_INCLUDE:       return "TOKEN_INCLUDE";
        case TOKEN_EXPECTING:     return "TOKEN_EXPECTING";
        case TOKEN_NOTHING:       return "TOKEN_NOTHING";
        case TOKEN_INCREMENT:     return "TOKEN_INCREMENT";
        case TOKEN_DECREMENT:     return "TOKEN_DECREMENT";
        case TOKEN_ADD:           return "TOKEN_ADD";
        case TOKEN_SUBTRACT:      return "TOKEN_SUBTRACT";
        case TOKEN_MULTIPLY:      return "TOKEN_MULTIPLY";
        case TOKEN_DIVIDE:        return "TOKEN_DIVIDE";
        case TOKEN_BY:            return "TOKEN_BY";
        case TOKEN_FROM:          return "TOKEN_FROM";
        case TOKEN_CONSTANT:      return "TOKEN_CONSTANT";
        case TOKEN_SIZE:          return "TOKEN_SIZE";
        case TOKEN_CAST:          return "TOKEN_CAST";
        case TOKEN_STRING_KW:     return "TOKEN_STRING_KW";
        case TOKEN_CONTINUE:      return "TOKEN_CONTINUE";
        case TOKEN_GOTO:          return "TOKEN_GOTO";
        case TOKEN_ENUMERATION:   return "TOKEN_ENUMERATION";
        case TOKEN_ALIAS:         return "TOKEN_ALIAS";
        case TOKEN_UNION_KW:      return "TOKEN_UNION_KW";
        case TOKEN_STATIC_KW:     return "TOKEN_STATIC_KW";
        case TOKEN_EXTERN_KW:     return "TOKEN_EXTERN_KW";
        case TOKEN_VOLATILE_KW:   return "TOKEN_VOLATILE_KW";
        case TOKEN_REGISTER_KW:   return "TOKEN_REGISTER_KW";
        case TOKEN_INLINE_KW:     return "TOKEN_INLINE_KW";

        // Types
        case TOKEN_INT:           return "TOKEN_INT";
        case TOKEN_FLOAT:         return "TOKEN_FLOAT";
        case TOKEN_DOUBLE:        return "TOKEN_DOUBLE";
        case TOKEN_CHAR:          return "TOKEN_CHAR";
        case TOKEN_VOID:          return "TOKEN_VOID";
        case TOKEN_LONG:          return "TOKEN_LONG";
        case TOKEN_SHORT:         return "TOKEN_SHORT";
        case TOKEN_UNSIGNED:      return "TOKEN_UNSIGNED";
        case TOKEN_SIGNED:        return "TOKEN_SIGNED";
        case TOKEN_BOOL:          return "TOKEN_BOOL";
        case TOKEN_TRUE:          return "TOKEN_TRUE";
        case TOKEN_FALSE:         return "TOKEN_FALSE";

        // Literals
        case TOKEN_NUMBER:        return "TOKEN_NUMBER";
        case TOKEN_STRING_LIT:    return "TOKEN_STRING_LIT";
        case TOKEN_CHAR_LIT:      return "TOKEN_CHAR_LIT";
        case TOKEN_IDENTIFIER:    return "TOKEN_IDENTIFIER";

        // Operators
        case TOKEN_ASSIGN:        return "TOKEN_ASSIGN";
        case TOKEN_PLUS:          return "TOKEN_PLUS";
        case TOKEN_MINUS:         return "TOKEN_MINUS";
        case TOKEN_STAR:          return "TOKEN_STAR";
        case TOKEN_SLASH:         return "TOKEN_SLASH";
        case TOKEN_PERCENT:       return "TOKEN_PERCENT";
        case TOKEN_EQUAL:         return "TOKEN_EQUAL";
        case TOKEN_NOT_EQUAL:     return "TOKEN_NOT_EQUAL";
        case TOKEN_LESS:          return "TOKEN_LESS";
        case TOKEN_GREATER:       return "TOKEN_GREATER";
        case TOKEN_LESS_EQUAL:    return "TOKEN_LESS_EQUAL";
        case TOKEN_GREATER_EQUAL: return "TOKEN_GREATER_EQUAL";
        case TOKEN_AND:           return "TOKEN_AND";
        case TOKEN_OR:            return "TOKEN_OR";
        case TOKEN_NOT:           return "TOKEN_NOT";
        case TOKEN_AMPERSAND:     return "TOKEN_AMPERSAND";
        case TOKEN_PIPE:          return "TOKEN_PIPE";
        case TOKEN_TILDE:         return "TOKEN_TILDE";
        case TOKEN_CARET:         return "TOKEN_CARET";
        case TOKEN_LSHIFT:        return "TOKEN_LSHIFT";
        case TOKEN_RSHIFT:        return "TOKEN_RSHIFT";
        case TOKEN_QUESTION:      return "TOKEN_QUESTION";

        // Compound Assignment Operators
        case TOKEN_PLUS_ASSIGN:    return "TOKEN_PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN:   return "TOKEN_MINUS_ASSIGN";
        case TOKEN_STAR_ASSIGN:    return "TOKEN_STAR_ASSIGN";
        case TOKEN_SLASH_ASSIGN:   return "TOKEN_SLASH_ASSIGN";
        case TOKEN_PERCENT_ASSIGN: return "TOKEN_PERCENT_ASSIGN";
        case TOKEN_AMP_ASSIGN:     return "TOKEN_AMP_ASSIGN";
        case TOKEN_PIPE_ASSIGN:    return "TOKEN_PIPE_ASSIGN";
        case TOKEN_CARET_ASSIGN:   return "TOKEN_CARET_ASSIGN";
        case TOKEN_LSHIFT_ASSIGN:  return "TOKEN_LSHIFT_ASSIGN";
        case TOKEN_RSHIFT_ASSIGN:  return "TOKEN_RSHIFT_ASSIGN";

        // Punctuation
        case TOKEN_LPAREN:        return "TOKEN_LPAREN";
        case TOKEN_RPAREN:        return "TOKEN_RPAREN";
        case TOKEN_LBRACKET:      return "TOKEN_LBRACKET";
        case TOKEN_RBRACKET:      return "TOKEN_RBRACKET";
        case TOKEN_LBRACE:        return "TOKEN_LBRACE";
        case TOKEN_RBRACE:        return "TOKEN_RBRACE";
        case TOKEN_COMMA:         return "TOKEN_COMMA";
        case TOKEN_COLON:         return "TOKEN_COLON";
        case TOKEN_SEMICOLON:     return "TOKEN_SEMICOLON";
        case TOKEN_DOT:           return "TOKEN_DOT";
        case TOKEN_ARROW:         return "TOKEN_ARROW";
        case TOKEN_HASH:          return "TOKEN_HASH";
        case TOKEN_ELLIPSIS:      return "TOKEN_ELLIPSIS";

        // Indentation
        case TOKEN_INDENT:        return "TOKEN_INDENT";
        case TOKEN_DEDENT:        return "TOKEN_DEDENT";
        case TOKEN_NEWLINE:       return "TOKEN_NEWLINE";

        // C17-specific operator tokens
        case TOKEN_PLUS_PLUS:     return "TOKEN_PLUS_PLUS";
        case TOKEN_MINUS_MINUS:   return "TOKEN_MINUS_MINUS";

        // C17 keyword tokens
        case TOKEN_C_STRUCT:      return "TOKEN_C_STRUCT";
        case TOKEN_C_CONST:       return "TOKEN_C_CONST";
        case TOKEN_C_SIZEOF:      return "TOKEN_C_SIZEOF";
        case TOKEN_C_NULL:        return "TOKEN_C_NULL";
        case TOKEN_C_TYPEDEF:     return "TOKEN_C_TYPEDEF";
        case TOKEN_C_ENUM:        return "TOKEN_C_ENUM";
        case TOKEN_C_UNION:       return "TOKEN_C_UNION";
        case TOKEN_C_STATIC:      return "TOKEN_C_STATIC";
        case TOKEN_C_EXTERN:      return "TOKEN_C_EXTERN";
        case TOKEN_C_VOLATILE:    return "TOKEN_C_VOLATILE";
        case TOKEN_C_REGISTER:    return "TOKEN_C_REGISTER";
        case TOKEN_C_INLINE:      return "TOKEN_C_INLINE";

        // Special
        case TOKEN_EOF:           return "TOKEN_EOF";
        case TOKEN_ERROR:         return "TOKEN_ERROR";

        default:                  return "TOKEN_UNKNOWN";
    }
}
