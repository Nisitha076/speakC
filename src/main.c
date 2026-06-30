#include "../include/util/strbuf.h"
#include "../include/util/vec.h"
#include "../include/util/arena.h"
#include "../include/lexer.h"
#include "../include/c_lexer.h"
#include "../include/token.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/emitter.h"
#include "../include/util/file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(void) {
    printf("Usage: speakc <command> <filename>\n");
    printf("Commands:\n");
    printf("  build    Transpile SpeakC file to C17\n");
    printf("  tokens   Print tokens of SpeakC file\n");
    printf("  ctokens  Print tokens of C17 file\n");
    printf("  ast      Print AST of SpeakC file\n");
    printf("  reverse  Reverse transpile C to SpeakC\n");
}

static int cmd_build(const char *input_path) {
    // 1. Read the source file
    char *source = read_file(input_path);
    if (!source) return 1;

    // 2. Set up memory
    Arena arena;
    arena_init(&arena);

    // 3. Lex
    Lexer lexer;
    Vec tokens;
    vec_init(&tokens);
    lexer_init(&lexer, source, &arena);
    lexer_tokenize(&lexer, &tokens);

    // 4. Parse
    Parser parser;
    parser_init(&parser, &tokens, &arena, source, input_path);
    ASTNode *ast = parser_parse(&parser);

    // 5. Emit
    Emitter emitter;
    emitter_init(&emitter);
    emitter_emit(&emitter, ast);
    char *output = emitter_get_output(&emitter);

    // 6. Write output file (change extension to .c safely)
    char output_path[256];
    const char *dot = strrchr(input_path, '.');
    size_t stem_len = dot ? (size_t)(dot - input_path) : strlen(input_path);
    int written = snprintf(output_path, sizeof(output_path), "%.*s.c", (int)stem_len, input_path);

    if (written < 0 || (size_t)written >= sizeof(output_path)) {
        fprintf(stderr, "Error: output file path is too long\n");
        // Cleanup
        free(output);
        emitter_free(&emitter);
        vec_free(&tokens);
        arena_free(&arena);
        free(source);
        return 1;
    }

    if (write_file(output_path, output) != 0) {
        fprintf(stderr, "Failed to write output\n");
        // Cleanup
        free(output);
        emitter_free(&emitter);
        vec_free(&tokens);
        arena_free(&arena);
        free(source);
        return 1;
    }

    printf("✓ Transpiled: %s → %s\n", input_path, output_path);

    // 7. Cleanup
    free(output);
    emitter_free(&emitter);
    vec_free(&tokens);
    arena_free(&arena);
    free(source);

    return 0;
}


static int cmd_tokens(const char *input_path) {
    char *source = read_file(input_path);
    if (!source) return 1;

    Arena arena;
    arena_init(&arena);
    Lexer lexer;
    Vec tokens;
    vec_init(&tokens);
    lexer_init(&lexer, source, &arena);
    lexer_tokenize(&lexer, &tokens);

    // Print all tokens
    for (int i = 0; i < tokens.count; i++) {
        Token *tok = (Token *)vec_get(&tokens, i);
        printf("%-20s %-15s  %d:%d\n",
               token_type_name(tok->type),
               tok->value,
               tok->line,
               tok->column);
    }

    vec_free(&tokens);
    arena_free(&arena);
    free(source);
    return 0;
}

static int cmd_ctokens(const char *input_path) {
    char *source = read_file(input_path);
    if (!source) return 1;

    Arena arena;
    arena_init(&arena);
    CLexer lexer;
    Vec tokens;
    vec_init(&tokens);
    c_lexer_init(&lexer, source, &arena);
    c_lexer_tokenize(&lexer, &tokens);

    printf("--- C17 Tokens from %s ---\n", input_path);
    for (int i = 0; i < tokens.count; i++) {
        Token *tok = (Token *)vec_get(&tokens, i);
        printf("%-20s %-15s  %d:%d\n",
               token_type_name(tok->type),
               tok->value,
               tok->line,
               tok->column);
    }
    printf("Total tokens: %d\n", tokens.count);

    vec_free(&tokens);
    arena_free(&arena);
    free(source);
    return 0;
}

static int cmd_ast(const char *input_path) {
    char *source = read_file(input_path);
    if (!source) return 1;

    Arena arena;
    arena_init(&arena);
    Lexer lexer;
    Vec tokens;
    vec_init(&tokens);
    lexer_init(&lexer, source, &arena);
    lexer_tokenize(&lexer, &tokens);

    Parser parser;
    parser_init(&parser, &tokens, &arena, source, input_path);
    ASTNode *ast = parser_parse(&parser);

    ast_print(ast, 0);

    vec_free(&tokens);
    arena_free(&arena);
    free(source);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("speakc version 0.1.0\n");
        return 0;
    }

    if (argc < 3) {
        print_usage();
        return 1;
    }

    const char *command = argv[1];
    const char *filename = argv[2];

    if (strcmp(command, "build") == 0)   return cmd_build(filename);
    if (strcmp(command, "tokens") == 0)  return cmd_tokens(filename);
    if (strcmp(command, "ctokens") == 0) return cmd_ctokens(filename);
    if (strcmp(command, "ast") == 0)     return cmd_ast(filename);
    if (strcmp(command, "reverse") == 0) {
        printf("TODO: Reverse transpilation (Phase 7)\n");
        return 0;
    }

    printf("Unknown command: %s\n", command);
    print_usage();
    return 1;
}