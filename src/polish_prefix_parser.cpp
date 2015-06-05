#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <tgmath.h>
#include <ctype.h>

// c with templates stack
#define MAX_STACK_SIZE 64

template <class T> struct PPStack{
  size_t size;
  T items[MAX_STACK_SIZE];
};

template <class T> inline void push(PPStack<T> * ps, T x)
{
  if (ps->size >= MAX_STACK_SIZE) {
    fputs("Error: stack overflow\n", stderr);
    abort();
  } else
    ps->items[ps->size++] = x;
}

template <class T> inline T pop(PPStack<T> *ps)
{
  if (ps->size == 0){
    fputs("Error: stack underflow\n", stderr);
    abort();
  } else
    return ps->items[--ps->size];
}

// function library
typedef void (* pp_func)( PPStack<float> * pp_val_stack, float * val, int offset);
 
inline void pp_func_add(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, pop(pp_val_stack) + pop(pp_val_stack));
}
inline void pp_func_sub(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, pop(pp_val_stack) - pop(pp_val_stack));
}
inline void pp_func_mul(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, pop(pp_val_stack) * pop(pp_val_stack));
}
inline void pp_func_div(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, pop(pp_val_stack) / pop(pp_val_stack));
}
inline void pp_func_exp(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)pow(pop(pp_val_stack), pop(pp_val_stack)));
}
inline void pp_func_abs(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)fabs(pop(pp_val_stack) ));
}
inline void pp_func_cos(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)cos(pop(pp_val_stack) ));
}
inline void pp_func_sin(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)sin(pop(pp_val_stack) ));
}
inline void pp_func_tan(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, (float)tan(pop(pp_val_stack) ));
}
inline void pp_func_push(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, *val);
}

inline void pp_func_push_offset(PPStack<float> * pp_val_stack, float * val, int offset){
  push(pp_val_stack, *(val + offset));
}

pp_func get_pp_func(char id){
  switch(id){
  case '+':
    return pp_func_add;
    break;
  case '-':
    return pp_func_sub;
    break;
  case '*':
    return pp_func_mul;
    break;
  case '/':
    return pp_func_div;
    break;
  case 'a':
    return pp_func_abs;
    break;
  case '^':
    return pp_func_exp;
    break;
  case 's':
    return pp_func_sin;
    break;
  case 'c':
    return pp_func_cos;
    break;
  case 't':
    return pp_func_tan;
    break;
  default:
    return NULL;
    break;
  }
  return NULL;
}
int get_pp_func_len(char id){
  switch(id){
  case '+':
    return 1;
    break;
  case '-':
    return 1;
    break;
  case '*':
    return 1;
    break;
  case '/':
    return 1;
    break;
  case 'a':
    return 0;
    break;
  case '^':
    return 1;
    break;
  case 's':
    return 0;
    break;
  case 'c':
    return 0;
    break;
  case 't':
    return 0;
    break;
  default:
    return 0;
    break;
  }
  return 0;
}

int is_pp_func(char id){
  return ( (id == '+') || (id == '-') ||
	   (id == '*') || (id == '/') ||
	   (id == '^') || (id == 'a') ||
	   (id == 's') || (id == 'c') ||
	   (id == 't'));
}

//////////////////////////////////////
//string parsing

//from stack overflow  Alexey Frunze
char** pp_str_split(char* a_str, const char a_delim)
{
  char** result    = 0;
  size_t count     = 0;
  char* tmp        = a_str;
  char* last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp) {
    if (a_delim == *tmp) {
      count++;
      last_comma = tmp;
      }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen(a_str) - 1);
  /* Add space for terminating null string so caller
     knows where the list of returned strings ends. */
  count++;
  result = (char**)malloc(sizeof(char*) * count);
  if (result) {
      size_t idx  = 0;
      char* token = strtok(a_str, delim);

      while (token) {
	  assert(idx < count);
	  *(result + idx++) = strdup(token);
	  token = strtok(0, delim);
        }
      assert(idx == count - 1); //this usually means that two spaces are in a row
      *(result + idx) = 0;
    }
  return result;
}


int is_numeric (const char * s)
{
  if (s == NULL || *s == '\0' || isspace(*s))
    return 0;
  char * p;
  strtod (s, &p);
  return *p == '\0';
}


struct PPToken{
  pp_func func;
  int arg_num;
  float val;
  float * val_addr;
};


//////////////////////////////////////
//list of functions
//get number of arguments pushed / pulled
void tokenize_equation_or_die(const char * eq, PPStack<PPToken> * out_stack){
  out_stack->size = 0;

  //check for weird (ok not so fucking weird, shut up) edge cases
  //check that it is not empty
  int eqlen = strlen(eq);
  if ( eqlen == 0){
    fprintf(stderr, "empty equation '%s'\n", eq);
    exit(1);
  }  
  //check if first character is space
  if ( isspace(eq[0]) ){
    fprintf(stderr, "equation begins with whitespace '%s'\n", eq);
    exit(1);
  }

  //check if last character is space
  if ( isspace(eq[eqlen-1]) ){
    fprintf(stderr, "equation ends with whitespace '%s'\n", eq);
    exit(1);
  }
  
  //check if there are two spaces in a row
  char twospace[] = "  ";
  if (strstr (eq, twospace) != NULL){
    fprintf(stderr, "two spaces found in '%s'\n", eq);
    exit(1);
  }

  char * eq_copy;
  eq_copy = (char *) malloc((eqlen + 1) * sizeof(char));
  strncpy(eq_copy, eq, eqlen + 1 );



  //tokenize equation
  char ** split_eq = pp_str_split(eq_copy, ' ');
  if (split_eq){
    for (int i = 0; *(split_eq + i); i++){
      //printf("Tokenizing:  %s\n",  *(split_eq + i));
      //check if len 1 and func
      char * eqt = *(split_eq + i);
      PPToken tok;
      tok.val_addr = NULL;
      if (strlen(eqt) == 1 && is_pp_func(eqt[0])){
	tok.func = get_pp_func(eqt[0]);
	tok.arg_num = get_pp_func_len(eqt[0]);
	tok.val = 0;
      } else if ( is_numeric( eqt )){
	tok.arg_num = -1;
	tok.val = atof(eqt);
	tok.func = pp_func_push;
      } else{
	fprintf(stderr, "Token '%s' is not recognized\n", eqt);
	exit(1);
      }
      push(out_stack, tok);
      free( *(split_eq + i));
    }
    free(split_eq);
  }else{
    fprintf(stderr, "pp_str_split failed miserably on '%s'\n", eq);
    exit(1);
  }
  //check that it's a valid equation
  int check_val = 0;
  for (int i = out_stack->size-1; i >= 0; i--){
    check_val += out_stack->items[i].arg_num;
    //printf("%d\n",check_val);
    if (check_val > 0){
      fprintf(stderr, "equation is not valid, too many operators before variables '%s'\n", eq);
      exit(1);
    }
  }
  //check that we end with only one value on the stack
  if (check_val != -1){
    fprintf(stderr, "equation does not have a definite result '%s'\n", eq);
    exit(1);
  }
  free(eq_copy);
}

float evaluate_tokenized_equation_or_die(PPStack<PPToken> * token_stack, int offset){
  PPStack<float> eval_stack;
  eval_stack.size = 0;
  for (int i = token_stack->size-1; i >= 0; i--){
    token_stack->items[i].func(&eval_stack, token_stack->items[i].val_addr == NULL ? &(token_stack->items[i].val) : token_stack->items[i].val_addr, offset );
  }
  return eval_stack.items[0];
}





int main(int argn, char * argc[]){
  //char test[] = "* + 3 3 + 2 2";
  PPStack<PPToken> pp_tokens;
  if (argn < 2) exit(1);
  printf("evaluating %s\n", argc[1]);

  tokenize_equation_or_die( argc[1], &pp_tokens );
  for (int i = 0; i < 10000000; i++){
    float val = evaluate_tokenized_equation_or_die(&pp_tokens,0);
  }

  return 0;
}
